// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTFlagRunGameState.h"
#include "UTHUDWidget_FlagRunStatus.h"
#include "UTBlitzFlag.h"
#include "UTBlitzFlagSpawner.h"
#include "UTBlitzDeliveryPoint.h"

UUTHUDWidget_FlagRunStatus::UUTHUDWidget_FlagRunStatus(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	YouHaveFlagText = NSLOCTEXT("CTFScore", "YouHaveFlagText", "You have the flag!");
	YouHaveFlagTextAlt = NSLOCTEXT("CTFScore", "YouHaveFlagTextAlt", "You have the flag, take it to the enemy base!");
	EnemyHasFlagText = NSLOCTEXT("CTFScore", "EnemyHasFlagText", "The enemy has your flag, recover it!");
	BothFlagsText = NSLOCTEXT("CTFScore", "BothFlagsText", "You have the enemy flag - hold it until your flag is returned!");

	ScreenPosition = FVector2D(0.5f, 0.0f);
	Size = FVector2D(0.f, 0.f);
	Origin = FVector2D(0.5f, 0.5f);
	AnimationAlpha = 0.0f;
	StatusScale = 1.f;
	InWorldAlpha = 0.8f;
	MaxIconScale = 0.75f;

	bSuppressMessage = false;

	OldFlagState[0] = CarriedObjectState::Home;
	OldFlagState[1] = CarriedObjectState::Home;
	StatusChangedScale = 10.f;
	CurrentStatusScale[0] = 1.f;
	CurrentStatusScale[1] = 1.f;
	ScaleDownTime = 0.5f;

	TopEdgePadding = 0.1f;
	BottomEdgePadding = 0.18f;
	LeftEdgePadding = 0.025f;
	RightEdgePadding = 0.025f;
	NormalLineBrightness = 0.025f;
	LineGlow = 0.4f;
	PulseLength = 0.7f;
	bAlwaysDrawFlagHolderName = false;
	DeliveryPointText = NSLOCTEXT("FlagRunStatus", "DeliveryPoint", "Delivery Point");
}

bool UUTHUDWidget_FlagRunStatus::ShouldDraw_Implementation(bool bShowScores)
{
	bool bResult = !bShowScores && UTGameState && UTGameState->HasMatchStarted() && !UTGameState->HasMatchEnded() && (UTGameState->GetMatchState() != MatchState::MatchIntermission);
	if (!bResult)
	{
		LastFlagStatusChange = GetWorld()->GetTimeSeconds();
	}
	return bResult;
}

void UUTHUDWidget_FlagRunStatus::Draw_Implementation(float DeltaTime)
{
	AUTFlagRunGameState* GameState = Cast<AUTFlagRunGameState>(UTGameState);
	if (GameState == NULL) return;

	if (bStatusDir)
	{
		StatusScale = FMath::Min(StatusScale + 2.f*DeltaTime, 2.0f);
		bStatusDir = (StatusScale < 1.8f);
	}
	else
	{
		StatusScale = FMath::Max(StatusScale - 2.f*DeltaTime, 1.f);
		bStatusDir = (StatusScale < 1.2f);
	}
	for (int32 i = 0; i < 2; i++)
	{
		CurrentStatusScale[i] = FMath::Max(1.f, CurrentStatusScale[i] - DeltaTime*(StatusChangedScale - 1.f) / ScaleDownTime);
	}

	FVector ViewPoint;
	FRotator ViewRotation;

	UTPlayerOwner->GetPlayerViewPoint(ViewPoint, ViewRotation);
	DrawIndicators(GameState, ViewPoint, ViewRotation, DeltaTime);
}

void UUTHUDWidget_FlagRunStatus::DrawIndicators(AUTFlagRunGameState* GameState, FVector PlayerViewPoint, FRotator PlayerViewRotation, float DeltaTime)
{
	if (GameState)
	{
		uint8 OwnerTeam = UTHUDOwner->UTPlayerOwner->GetTeamNum();
		uint8 OffensiveTeam = GameState->bRedToCap ? 0 : 1;
		uint8 DefensiveTeam = GameState->bRedToCap ? 1 : 0;

		if (GameState->FlagSpawner)
		{
			AUTBlitzFlag* Flag = Cast<AUTBlitzFlag>(GameState->FlagSpawner->GetCarriedObject());
			if (Flag && (Flag->ObjectState != CarriedObjectState::Delivered))
			{
				if (Flag->Holder && (Flag->Holder == UTHUDOwner->UTPlayerOwner->UTPlayerState))
				{
					FlagHolderNameTemplate.Text = YouHaveFlagText;
					RenderObj_Text(FlagHolderNameTemplate, FVector2D(0.0f, 50.0f));
				}
				DrawFlagWorld(GameState, PlayerViewPoint, PlayerViewRotation, OffensiveTeam, Flag, Flag->Holder);
			}
		}
		if (GameState->DeliveryPoint)
		{
			DrawFlagBaseWorld(GameState, PlayerViewPoint, PlayerViewRotation, DefensiveTeam, GameState->DeliveryPoint, nullptr, nullptr);
		}
	}
}

FVector UUTHUDWidget_FlagRunStatus::GetAdjustedScreenPosition(const FVector& WorldPosition, const FVector& ViewPoint, const FVector& ViewDir, float Dist, float IconSize, bool& bDrawEdgeArrow, int32 Team)
{
	FVector Cross = (ViewDir ^ FVector(0.f, 0.f, 1.f)).GetSafeNormal();
	FVector DrawScreenPosition = GetCanvas()->Project(WorldPosition);
	FVector FlagDir = (WorldPosition - ViewPoint).GetSafeNormal();
	if ((ViewDir | FlagDir) < 0.f)
	{
		bDrawEdgeArrow = true;
		FVector LeftDir = (ViewDir ^ FVector(0.f, 0.f, 1.f)).GetSafeNormal();
		bool bLeftOfScreen = (FlagDir | LeftDir) > 0.f;
		DrawScreenPosition.X = bLeftOfScreen ? -0.5f*GetCanvas()->ClipX * (ViewDir | FlagDir) : GetCanvas()->ClipX * (1.f + 0.5f*(ViewDir | FlagDir));
		DrawScreenPosition.Y = (1.f - BottomEdgePadding)*GetCanvas()->ClipY - IconSize;
		DrawScreenPosition.Z = 0.0f;
		DrawScreenPosition.X = FMath::Clamp(DrawScreenPosition.X, IconSize + LeftEdgePadding*GetCanvas()->ClipX, GetCanvas()->ClipX - IconSize - RightEdgePadding*GetCanvas()->ClipX);
		return DrawScreenPosition;
	}
	else if ((DrawScreenPosition.X < 0.f) || (DrawScreenPosition.X > GetCanvas()->ClipX))
	{
		bool bLeftOfScreen = (DrawScreenPosition.X < 0.f);
		float OffScreenDistance = bLeftOfScreen ? -1.f*DrawScreenPosition.X : DrawScreenPosition.X - GetCanvas()->ClipX;
		bDrawEdgeArrow = true;
		DrawScreenPosition.X = bLeftOfScreen ? IconSize + LeftEdgePadding*GetCanvas()->ClipX : GetCanvas()->ClipX - IconSize - RightEdgePadding*GetCanvas()->ClipX;
		//Y approaches 0.9*Canvas->ClipY as further off screen
		float MaxOffscreenDistance = GetCanvas()->ClipX;
		DrawScreenPosition.Y = (1.f - BottomEdgePadding)*GetCanvas()->ClipY + FMath::Clamp((MaxOffscreenDistance - OffScreenDistance) / MaxOffscreenDistance, 0.f, 1.f) * (DrawScreenPosition.Y - (1.f - BottomEdgePadding)*GetCanvas()->ClipY);
		if (Team == 1)
		{
			DrawScreenPosition.Y += 0.05f*GetCanvas()->ClipY;
		}
		DrawScreenPosition.Y = FMath::Clamp(DrawScreenPosition.Y, IconSize + TopEdgePadding*GetCanvas()->ClipY, GetCanvas()->ClipY - IconSize - BottomEdgePadding*GetCanvas()->ClipY);
	}
	else
	{
		DrawScreenPosition.X = FMath::Clamp(DrawScreenPosition.X, IconSize + LeftEdgePadding*GetCanvas()->ClipX, GetCanvas()->ClipX - IconSize - RightEdgePadding*GetCanvas()->ClipX);
		DrawScreenPosition.Y = FMath::Clamp(DrawScreenPosition.Y, IconSize + TopEdgePadding*GetCanvas()->ClipY, GetCanvas()->ClipY - IconSize - BottomEdgePadding*GetCanvas()->ClipY);
		DrawScreenPosition.Z = 0.0f;
	}
	return DrawScreenPosition;
}


void UUTHUDWidget_FlagRunStatus::DrawEdgeArrow(FVector InWorldPosition, FVector PlayerViewPoint, FRotator PlayerViewRotation, FVector InDrawScreenPosition, float CurrentWorldAlpha, float WorldRenderScale, int32 Team)
{
	ArrowTemplate.RenderScale = 1.1f * WorldRenderScale;
	ArrowTemplate.RenderOpacity = CurrentWorldAlpha;
	ArrowTemplate.RenderColor = (Team == 0) ? REDHUDCOLOR : BLUEHUDCOLOR;
	float RotYaw = FMath::Acos(PlayerViewRotation.Vector() | (InWorldPosition - PlayerViewPoint).GetSafeNormal()) * 180.f / PI;
	if (InDrawScreenPosition.X < 0.f)
	{
		RotYaw *= -1.f;
	}
	RenderObj_TextureAtWithRotation(ArrowTemplate, FVector2D(InDrawScreenPosition.X, InDrawScreenPosition.Y), RotYaw);
}

bool UUTHUDWidget_FlagRunStatus::ShouldDrawFlag(AUTFlag* Flag, bool bIsEnemyFlag)
{
	return (Flag->ObjectState == CarriedObjectState::Dropped) || (Flag->ObjectState == CarriedObjectState::Home) || Flag->bCurrentlyPinged || !bIsEnemyFlag;
}

FText UUTHUDWidget_FlagRunStatus::GetFlagReturnTime(AUTFlag* Flag)
{
	return Flag ? FText::AsNumber(Flag->FlagReturnTime) : FText::GetEmpty();
}

void UUTHUDWidget_FlagRunStatus::DrawFlagBaseWorld(AUTFlagRunGameState* GameState, FVector PlayerViewPoint, FRotator PlayerViewRotation, uint8 TeamNum, AUTGameObjective* FlagBase, AUTFlag* Flag, AUTPlayerState* FlagHolder)
{
	if (FlagBase)
	{
		if (Flag == nullptr)
		{
			AUTBlitzFlag* BaseFlag = Cast<AUTBlitzFlag>(FlagBase->GetCarriedObject());
		}

		bScaleByDesignedResolution = false;

		float Dist = (FlagBase->GetActorLocation() - PlayerViewPoint).Size();
		float WorldRenderScale = RenderScale * MaxIconScale;

		bool bSpectating = UTPlayerOwner->PlayerState && UTPlayerOwner->PlayerState->bOnlySpectator;
		bool bDrawEdgeArrow = false;
		FVector DrawScreenPosition(0.f);
		FVector WorldPosition = FlagBase->GetActorLocation() + FlagBase->GetActorRotation().RotateVector(Flag ? Flag->HomeBaseOffset : FVector(0.0f, 0.0f, 128.0f)) + FVector(0.f, 0.f, Flag ? Flag->Collision->GetUnscaledCapsuleHalfHeight() * 3.f : 0.0f);
		float CurrentWorldAlpha = InWorldAlpha;
		FVector ViewDir = PlayerViewRotation.Vector();
		float Edge = CircleTemplate.GetWidth()* WorldRenderScale;
		DrawScreenPosition = GetAdjustedScreenPosition(WorldPosition, PlayerViewPoint, ViewDir, Dist, Edge, bDrawEdgeArrow, TeamNum);

		float PctFromCenter = (DrawScreenPosition - FVector(0.5f*GetCanvas()->ClipX, 0.5f*GetCanvas()->ClipY, 0.f)).Size() / GetCanvas()->ClipX;
		CurrentWorldAlpha = InWorldAlpha * FMath::Min(8.f*PctFromCenter, 1.f);

		DrawScreenPosition.X -= RenderPosition.X;
		DrawScreenPosition.Y -= RenderPosition.Y;

		CircleBorderTemplate.RenderColor = (TeamNum == 0) ? REDHUDCOLOR : BLUEHUDCOLOR;
		CircleBorderTemplate.RenderOpacity = CurrentWorldAlpha;
		CircleTemplate.RenderOpacity = CurrentWorldAlpha;

		RenderObj_TextureAt(CircleTemplate, DrawScreenPosition.X, DrawScreenPosition.Y, CircleTemplate.GetWidth()* WorldRenderScale, CircleTemplate.GetHeight()* WorldRenderScale);
		RenderObj_TextureAt(CircleBorderTemplate, DrawScreenPosition.X, DrawScreenPosition.Y, CircleBorderTemplate.GetWidth()* WorldRenderScale, CircleBorderTemplate.GetHeight()* WorldRenderScale);

		if (TeamNum == 0)
		{
			RedTeamIconTemplate.RenderOpacity = CurrentWorldAlpha;
			RenderObj_TextureAt(RedTeamIconTemplate, DrawScreenPosition.X, DrawScreenPosition.Y, RedTeamIconTemplate.GetWidth()* WorldRenderScale, RedTeamIconTemplate.GetHeight()* WorldRenderScale);
		}
		else
		{
			BlueTeamIconTemplate.RenderOpacity = CurrentWorldAlpha;
			RenderObj_TextureAt(BlueTeamIconTemplate, DrawScreenPosition.X, DrawScreenPosition.Y, BlueTeamIconTemplate.GetWidth()* WorldRenderScale, BlueTeamIconTemplate.GetHeight()* WorldRenderScale);
		}
		DrawText(DeliveryPointText, DrawScreenPosition.X, DrawScreenPosition.Y - ((BlueTeamIconTemplate.GetHeight() + 8.f) * WorldRenderScale), AUTHUD::StaticClass()->GetDefaultObject<AUTHUD>()->TinyFont, true, FVector2D(1.f, 1.f), FLinearColor::Black, false, FLinearColor::Black, 1.5f*WorldRenderScale, 0.5f + 0.5f*CurrentWorldAlpha, FLinearColor::White, FLinearColor(0.f, 0.f, 0.f, 0.f), ETextHorzPos::Center, ETextVertPos::Center);

		if (bDrawEdgeArrow)
		{
			DrawEdgeArrow(WorldPosition, PlayerViewPoint, PlayerViewRotation, DrawScreenPosition, CurrentWorldAlpha, WorldRenderScale, TeamNum);
		}
		if (Flag && Flag->ObjectState != CarriedObjectState::Home)
		{
			RenderObj_TextureAt(FlagGoneIconTemplate, DrawScreenPosition.X, DrawScreenPosition.Y, FlagGoneIconTemplate.GetWidth()* WorldRenderScale, FlagGoneIconTemplate.GetHeight()* WorldRenderScale);
		}

		CircleTemplate.RenderOpacity = 1.f;
		CircleBorderTemplate.RenderOpacity = 1.f;
		bScaleByDesignedResolution = true;
	}
}

void UUTHUDWidget_FlagRunStatus::DrawFlagWorld(AUTFlagRunGameState* GameState, FVector PlayerViewPoint, FRotator PlayerViewRotation, uint8 TeamNum, AUTFlag* Flag, AUTPlayerState* FlagHolder)
{
	bool bSpectating = UTPlayerOwner->PlayerState && UTPlayerOwner->PlayerState->bOnlySpectator;
	bool bIsEnemyFlag = Flag && GameState && !GameState->OnSameTeam(Flag, UTPlayerOwner);
	bool bShouldDrawFlagIcon = ShouldDrawFlag(Flag, bIsEnemyFlag);
	if (Flag && GameState && (bSpectating || bShouldDrawFlagIcon) && (Flag->Holder != UTHUDOwner->GetScorerPlayerState()))
	{
		bScaleByDesignedResolution = false;
		FlagIconTemplate.RenderColor = (TeamNum == 0) ? REDHUDCOLOR : BLUEHUDCOLOR;

		// Draw the flag / flag base in the world
		float Dist = (Flag->GetActorLocation() - PlayerViewPoint).Size();
		float WorldRenderScale = RenderScale * MaxIconScale;
		bool bDrawEdgeArrow = false;
		float OldFlagAlpha = FlagIconTemplate.RenderOpacity;
		float CurrentWorldAlpha = InWorldAlpha;
		float CurrentNumberAlpha = InWorldAlpha;
		FVector ViewDir = PlayerViewRotation.Vector();
		float Edge = CircleTemplate.GetWidth()* WorldRenderScale;

		AUTCharacter* Holder = (Flag->ObjectState == CarriedObjectState::Held) ? Cast<AUTCharacter>(Flag->GetAttachmentReplication().AttachParent) : nullptr;
		if (!Holder && (Flag->ObjectState == CarriedObjectState::Held))
		{
			Holder = Flag->HoldingPawn;
		}
		FVector WorldPosition = (Holder != nullptr) ? Holder->GetMesh()->GetComponentLocation() + FVector(0.f, 0.f, Holder->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()) : Flag->GetActorLocation();
		FVector DrawScreenPosition = GetAdjustedScreenPosition(WorldPosition, PlayerViewPoint, ViewDir, Dist, Edge, bDrawEdgeArrow, TeamNum);
		
		// Look to see if we should be displaying the in-world indicator for the flag.
		float CurrentWorldTime = GameState->GetWorld()->GetTimeSeconds();
		if (bIsEnemyFlag)
		{
			if (!bEnemyFlagWasDrawn)
			{
				EnemyFlagStartDrawTime = CurrentWorldTime;
			}
			if (CurrentWorldTime - EnemyFlagStartDrawTime < 0.3f)
			{
				WorldRenderScale *= (1.f + 3.f * (0.3f - CurrentWorldTime + EnemyFlagStartDrawTime));
			}
			bEnemyFlagWasDrawn = true;
		}

		float CurrentCircleAlpha = CurrentWorldAlpha;
		float ViewDist = (PlayerViewPoint - WorldPosition).Size();

		UFont* TinyFont = AUTHUD::StaticClass()->GetDefaultObject<AUTHUD>()->TinyFont;
		float InWorldFlagScale = WorldRenderScale * (StatusScale - 0.5f*(StatusScale - 1.f));
		float InWorldNumberScale = InWorldFlagScale;
		FVector EdgeScreenPosition = DrawScreenPosition;
		float CircleScaling = 1.f;
		if (!bDrawEdgeArrow)
		{
			FVector FootPosition = Holder ? Holder->GetMesh()->GetComponentLocation() : Flag->GetActorLocation() + FVector(0.f,0.f, Flag->Collision->GetUnscaledCapsuleHalfHeight());
			bool bIgnore = false;
			EdgeScreenPosition = GetAdjustedScreenPosition(FootPosition, PlayerViewPoint, ViewDir, Dist, Edge, bIgnore, TeamNum);
			float RingScaleFactor = Holder ? 4.4f : 4.f;
			CircleScaling = FMath::Max(1.f, RingScaleFactor * (DrawScreenPosition - EdgeScreenPosition).Size() / FMath::Max(1.f, CircleBorderTemplate.GetWidth()));
			InWorldFlagScale *= CircleScaling;
			float DistFromCenter = (DrawScreenPosition - FVector(0.5f*GetCanvas()->ClipX, 0.5f*GetCanvas()->ClipY, 0.f)).Size();
			CurrentCircleAlpha = CurrentCircleAlpha * 1.f / CircleScaling;
			if ((CircleScaling >= 4.f) && !Holder)
			{
				CurrentCircleAlpha *= FMath::Max(0.2f, 1.f /(CircleScaling - 3.f));
			}
			if ((CircleScaling < 6.f) && (DistFromCenter < 0.55f*CircleScaling*CircleBorderTemplate.GetWidth()))
			{
				CurrentCircleAlpha *= FMath::Min(1.f, 1.f/(7.f - CircleScaling));
			}
		}
		FlagIconTemplate.RenderOpacity = CurrentWorldAlpha;
		CircleTemplate.RenderOpacity = 1.5f*CurrentWorldAlpha;
		CircleBorderTemplate.RenderOpacity = CurrentCircleAlpha;

		DrawScreenPosition.X -= RenderPosition.X;
		DrawScreenPosition.Y -= RenderPosition.Y;
		EdgeScreenPosition.X -= RenderPosition.X;
		EdgeScreenPosition.Y -= RenderPosition.Y;

		if (Holder || (CircleScaling > 2.f) || bDrawEdgeArrow)
		{
			CircleBorderTemplate.RenderColor = FLinearColor::Black;
			RenderObj_TextureAt(CircleBorderTemplate, DrawScreenPosition.X, DrawScreenPosition.Y, CircleBorderTemplate.GetWidth()* InWorldFlagScale, CircleBorderTemplate.GetHeight()* InWorldFlagScale);
			if (!bDrawEdgeArrow)
			{
				CircleBorderTemplate.RenderColor = FlagIconTemplate.RenderColor;
				RenderObj_TextureAt(CircleBorderTemplate, DrawScreenPosition.X, DrawScreenPosition.Y, 1.1f*CircleBorderTemplate.GetWidth()* InWorldFlagScale, 1.1f*CircleBorderTemplate.GetHeight()* InWorldFlagScale);
			}
		}
		CircleBorderTemplate.RenderColor = FLinearColor::Black;
		FVector DrawNumberPosition = EdgeScreenPosition;
		DrawNumberPosition.Y = FMath::Min(DrawScreenPosition.Y, DrawNumberPosition.Y + 0.25f*CircleTemplate.GetHeight()* InWorldNumberScale);
		if (bDrawEdgeArrow)
		{
			RenderObj_TextureAt(CircleTemplate, DrawScreenPosition.X, DrawScreenPosition.Y, CircleTemplate.GetWidth()* InWorldFlagScale, CircleTemplate.GetHeight()* InWorldFlagScale);
			DrawEdgeArrow(WorldPosition, PlayerViewPoint, PlayerViewRotation, DrawScreenPosition, CurrentWorldAlpha, InWorldFlagScale, TeamNum);
		}
		else if (Holder == nullptr)
		{
			if ((DrawNumberPosition + FVector(RenderPosition.X, RenderPosition.Y, 0.f) - FVector(0.5f*GetCanvas()->ClipX, 0.5f*GetCanvas()->ClipY, 0.f)).Size() < 1.2f*CircleTemplate.GetWidth()*WorldRenderScale*1.5f)
			{
				CurrentNumberAlpha *= 0.2f;
			}
			CircleBorderTemplate.RenderOpacity = CurrentNumberAlpha;
			CircleTemplate.RenderOpacity = CurrentNumberAlpha;
			FlagIconTemplate.RenderOpacity = CurrentNumberAlpha;
			RenderObj_TextureAt(CircleTemplate, DrawNumberPosition.X, DrawNumberPosition.Y, CircleTemplate.GetWidth()*InWorldNumberScale, CircleTemplate.GetHeight()* InWorldNumberScale);
			RenderObj_TextureAt(CircleBorderTemplate, DrawNumberPosition.X, DrawNumberPosition.Y, CircleTemplate.GetWidth()*InWorldNumberScale, CircleTemplate.GetHeight()* InWorldNumberScale);
			CircleBorderTemplate.RenderColor = FlagIconTemplate.RenderColor;
			RenderObj_TextureAt(CircleBorderTemplate, DrawNumberPosition.X, DrawNumberPosition.Y, 1.1f*CircleTemplate.GetWidth()*InWorldNumberScale, 1.1f*CircleTemplate.GetHeight()* InWorldNumberScale);
			RenderObj_TextureAt(FlagIconTemplate, DrawNumberPosition.X, DrawNumberPosition.Y, FlagIconTemplate.GetWidth()* InWorldNumberScale, FlagIconTemplate.GetHeight()* InWorldNumberScale);
		}
		FText FlagStatusMessage = Flag->GetHUDStatusMessage(UTHUDOwner);
		if (!FlagStatusMessage.IsEmpty())
		{
			DrawText(FlagStatusMessage, DrawScreenPosition.X, DrawScreenPosition.Y - ((CircleTemplate.GetHeight() + 40) * WorldRenderScale), AUTHUD::StaticClass()->GetDefaultObject<AUTHUD>()->TinyFont, true, FVector2D(1.f, 1.f), FLinearColor::Black, false, FLinearColor::Black, 1.5f*WorldRenderScale, 0.5f + 0.5f*CurrentWorldAlpha, FLinearColor::White, FLinearColor(0.f, 0.f, 0.f, 0.f), ETextHorzPos::Center, ETextVertPos::Center);
		}
		if (bDrawEdgeArrow && Flag && Flag->ObjectState == CarriedObjectState::Held)
		{
			TakenIconTemplate.RenderOpacity = CurrentWorldAlpha;
			TakenIconTemplate.RenderColor = FlagIconTemplate.RenderColor;
			RenderObj_TextureAt(TakenIconTemplate, DrawScreenPosition.X, DrawScreenPosition.Y, 1.1f * TakenIconTemplate.GetWidth()* InWorldFlagScale, 1.1f * TakenIconTemplate.GetHeight()* InWorldFlagScale);
			RenderObj_TextureAt(FlagIconTemplate, DrawScreenPosition.X - 0.25f * FlagIconTemplate.GetWidth()* InWorldFlagScale, DrawScreenPosition.Y - 0.25f * FlagIconTemplate.GetHeight()* InWorldFlagScale, FlagIconTemplate.GetWidth()* InWorldFlagScale, FlagIconTemplate.GetHeight()* InWorldFlagScale);
		}
		else
		{
			if (bDrawEdgeArrow)
			{
				RenderObj_TextureAt(FlagIconTemplate, DrawScreenPosition.X, DrawScreenPosition.Y, 1.25f*FlagIconTemplate.GetWidth()* InWorldFlagScale, 1.25f*FlagIconTemplate.GetHeight()* InWorldFlagScale);
			}

			if (Flag->ObjectState == CarriedObjectState::Dropped)
			{
				bool bCloseToFlag = !bIsEnemyFlag && Cast<AUTCharacter>(UTPlayerOwner->GetPawn()) && Flag->IsNearTeammate((AUTCharacter *)(UTPlayerOwner->GetPawn()));
				FLinearColor TimeColor = bCloseToFlag ? FLinearColor::Green : FLinearColor::White;
				DrawText(GetFlagReturnTime(Flag), DrawNumberPosition.X, DrawNumberPosition.Y, TinyFont, true, FVector2D(1.f, 1.f), FLinearColor::Black, false, FLinearColor::Black, 1.5f*InWorldNumberScale, 0.5f + 0.5f*CurrentNumberAlpha, TimeColor, FLinearColor(0.f, 0.f, 0.f, 0.f), ETextHorzPos::Center, ETextVertPos::Center);
			}
			else if (Flag->ObjectState == CarriedObjectState::Home)
			{
				if (GameState->RemainingPickupDelay > 0)
				{
					DrawText(FText::AsNumber(GameState->RemainingPickupDelay), DrawNumberPosition.X, DrawNumberPosition.Y, TinyFont, true, FVector2D(1.f, 1.f), FLinearColor::Black, false, FLinearColor::Black, 1.5f*InWorldNumberScale, 0.5f + 0.5f*CurrentNumberAlpha, FLinearColor::White, FLinearColor(0.f, 0.f, 0.f, 0.f), ETextHorzPos::Center, ETextVertPos::Center);
				}
			}
		}
		FlagIconTemplate.RenderOpacity = OldFlagAlpha;
		CircleTemplate.RenderOpacity = 1.f;
		CircleBorderTemplate.RenderOpacity = 1.f;

		if (LastFlagStatus != Flag->ObjectState)
		{
			LastFlagStatus = Flag->ObjectState;
			LastFlagStatusChange = GetWorld()->GetTimeSeconds();
		}

		// draw line from hud to this loc - can't used Canvas line drawing code because it doesn't support translucency
		FVector LineEndPoint(DrawScreenPosition.X+RenderPosition.X, DrawScreenPosition.Y+RenderPosition.Y, 0.f);
		FVector LineStartPoint(0.5f*Canvas->ClipX, 0.5f*Canvas->ClipY, 0.f);
		if ((LineEndPoint - LineStartPoint).Size() > 0.05f*Canvas->ClipX)
		{
			float LineBrightness = 0.f;
			float TimeSinceChange = GetWorld()->GetTimeSeconds() - FMath::Max(LastFlagStatusChange, (bIsEnemyFlag ? GameState->LastEnemyLocationReportTime : GameState->LastFriendlyLocationReportTime));
			if (TimeSinceChange < 0.1f)
			{
				LineBrightness = LineGlow * TimeSinceChange * 10.f;
			}
			else if (TimeSinceChange < 0.1f + PulseLength)
			{
				LineBrightness = NormalLineBrightness + LineGlow * FMath::Max(0.f, 1.f - (TimeSinceChange - 0.1f) / PulseLength);
			}
			if (LineBrightness > 0.f)
			{
				LineStartPoint += 0.025f*Canvas->ClipX*(LineEndPoint - LineStartPoint).GetSafeNormal();
				FLinearColor LineColor = FlagIconTemplate.RenderColor;
				LineColor.A = LineBrightness;
				FBatchedElements* BatchedElements = Canvas->Canvas->GetBatchedElements(FCanvas::ET_Line);
				FHitProxyId HitProxyId = Canvas->Canvas->GetHitProxyId();
				BatchedElements->AddTranslucentLine(LineEndPoint, LineStartPoint, LineColor, HitProxyId, 8.f);
			}
		}
	}
	else if (bIsEnemyFlag)
	{
		bEnemyFlagWasDrawn = false;
	}
}

