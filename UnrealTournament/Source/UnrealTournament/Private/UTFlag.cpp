// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTCarriedObject.h"
#include "UTFlag.h"
#include "UTCTFGameMessage.h"
#include "UTCTFGameState.h"
#include "UTCTFGameMode.h"
#include "UTCTFRewardMessage.h"
#include "UnrealNetwork.h"
#include "UTFlagRunGameState.h"
#include "UTBlitzDeliveryPoint.h"
#include "UTLift.h"

static FName NAME_Wipe(TEXT("Wipe"));

AUTFlag::AUTFlag(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Collision->InitCapsuleSize(110.f, 134.0f);
	Mesh = ObjectInitializer.CreateDefaultSubobject<USkeletalMeshComponent>(this, TEXT("CTFFlag"));
	GetMesh()->AlwaysLoadOnClient = true;
	GetMesh()->AlwaysLoadOnServer = true;
	GetMesh()->SetupAttachment(RootComponent);
	GetMesh()->SetAbsolute(false, false, true);

	MeshOffset = FVector(0, 0.f, -48.f);
	HeldOffset = FVector(0.f, 0.f, 0.f);
	HomeBaseOffset = FVector(0.f, 0.f, -8.f);
	HomeBaseRotOffset.Yaw = 0.0f;

	FlagWorldScale = 1.75f;
	FlagHeldScale = 1.f;
	GetMesh()->SetWorldScale3D(FVector(FlagWorldScale));
	GetMesh()->bEnablePhysicsOnDedicatedServer = false;
	MovementComponent->ProjectileGravityScale = 1.3f;
	MovementComponent->bKeepPhysicsVolumeWhenStopped = true;
	bAlwaysRelevant = true;
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	ClothBlendHome = 0.f;
	ClothBlendHeld = 0.5f;
	PingedDuration = 2.f;
	TargetPingedDuration = 0.5f;
	bShouldPingFlag = false;
	NearTeammateDist = 1200.f;
}

void AUTFlag::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (GetNetMode() == NM_DedicatedServer || GetCachedScalabilityCVars().DetailMode == 0)
	{
		GetMesh()->bDisableClothSimulation = true;
	}
	if (GetNetMode() != NM_DedicatedServer)
	{
		MeshMID = Mesh->CreateAndSetMaterialInstanceDynamic(0);
	}
}

void AUTFlag::PlayCaptureEffect()
{
}

void AUTFlag::UpdateOutline()
{
	//	UE_LOG(UT, Warning, TEXT("Update Outline Holdingpawn %d is outlined %d"), (HoldingPawn != nullptr), (HoldingPawn && HoldingPawn->IsOutlined()) ? 1 : 0);
	const bool bOutlined = (GetNetMode() != NM_DedicatedServer) && ((HoldingPawn != nullptr) ? HoldingPawn->IsOutlined() : bGradualAutoReturn);
	// 0 is a null value for the stencil so use team + 1
	// last bit in stencil is a bitflag so empty team uses 127
	uint8 NewStencilValue = (GetTeamNum() == 255) ? 127 : (GetTeamNum() + 1);
	if (HoldingPawn == NULL || HoldingPawn->GetOutlineWhenUnoccluded())
	{
		NewStencilValue |= 128;
	}
	if (bOutlined)
	{
		GetMesh()->MeshComponentUpdateFlag = EMeshComponentUpdateFlag::AlwaysTickPoseAndRefreshBones;
		if (CustomDepthMesh == NULL)
		{
			CustomDepthMesh = Cast<USkeletalMeshComponent>(CreateCustomDepthOutlineMesh(GetMesh(), this));
			CustomDepthMesh->SetWorldScale3D(FVector(HoldingPawn ? FlagHeldScale : FlagWorldScale));
		}
		if (CustomDepthMesh->CustomDepthStencilValue != NewStencilValue)
		{
			CustomDepthMesh->CustomDepthStencilValue = NewStencilValue;
			CustomDepthMesh->MarkRenderStateDirty();
		}
		if (!CustomDepthMesh->IsRegistered())
		{
			CustomDepthMesh->RegisterComponent();
			CustomDepthMesh->LastRenderTime = GetMesh()->LastRenderTime;
		}
	}
	else
	{
		GetMesh()->MeshComponentUpdateFlag = EMeshComponentUpdateFlag::OnlyTickPoseWhenRendered;
		if (CustomDepthMesh != NULL && CustomDepthMesh->IsRegistered())
		{
			CustomDepthMesh->UnregisterComponent();
		}
	}
}

void AUTFlag::ClientUpdateAttachment(bool bNowAttachedToPawn)
{
	if (GetMesh())
	{
		GetMesh()->SetAbsolute(false, false, true);
		if (bNowAttachedToPawn)
		{
			GetMesh()->SetWorldScale3D(FVector(FlagHeldScale));
			GetMesh()->SetRelativeLocation(HeldOffset);
			GetMesh()->SetRelativeRotation(FRotator(0.f, 0.f, 0.f));
			GetMesh()->ClothBlendWeight = ClothBlendHeld;
		}
		else
		{
			GetMesh()->SetWorldScale3D(FVector(FlagWorldScale));
			GetMesh()->SetRelativeLocation(MeshOffset);
		}
		if (CustomDepthMesh)
		{
			CustomDepthMesh->SetWorldScale3D(FVector(bNowAttachedToPawn ? FlagHeldScale : FlagWorldScale));
		}
	}
	Super::ClientUpdateAttachment(bNowAttachedToPawn);
}

void AUTFlag::OnObjectStateChanged()
{
	Super::OnObjectStateChanged();

	if (Role == ROLE_Authority)
	{
		if (ObjectState == CarriedObjectState::Dropped)
		{
			GetWorldTimerManager().SetTimer(SendHomeWithNotifyHandle, this, &AUTFlag::SendHomeWithNotify, AutoReturnTime, false);
			FlagReturnTime = FMath::Clamp(int32(AutoReturnTime + 1.f), 0, 255);
		}
		else
		{
			GetWorldTimerManager().ClearTimer(SendHomeWithNotifyHandle);
		}
	}
	GetMesh()->ClothBlendWeight = (ObjectState == CarriedObjectState::Held) ? ClothBlendHeld : ClothBlendHome;
}

bool AUTFlag::IsNearTeammate(AUTCharacter* TeamChar)
{
	// slightly smaller radius on client 
	float Approx = (Role == ROLE_Authority) ? 1.f : 0.92f;
	return TeamChar && TeamChar->GetController() && ((GetActorLocation() - TeamChar->GetActorLocation()).SizeSquared() < Approx*NearTeammateDist*NearTeammateDist) && (((GetActorLocation() - TeamChar->GetActorLocation()).SizeSquared() < Approx*160000.f) || TeamChar->GetController()->LineOfSightTo(this));
}

void AUTFlag::SendHomeWithNotify()
{
	SendGameMessage(1, NULL, NULL);
	SendHome();
}

void AUTFlag::MoveToHome()
{
	Super::MoveToHome();
	GetMesh()->SetRelativeLocation(MeshOffset);
	GetMesh()->ClothBlendWeight = ClothBlendHome;
}

void AUTFlag::SetHolder(AUTCharacter* NewHolder)
{
	Super::SetHolder(NewHolder);

	if (HoldingPawn != nullptr)
	{
		// force a re-touch on any flag bases the new holder is overlapping
		// this handles the case where the flag was dropped such that the next holder is touching the capture point already when it is picked up
		TArray<UPrimitiveComponent*> Overlaps;
		HoldingPawn->GetOverlappingComponents(Overlaps);
		for (UPrimitiveComponent* OtherComp : Overlaps)
		{
			AUTCTFFlagBase* FlagBase = Cast<AUTCTFFlagBase>(OtherComp->GetOwner());
			if (FlagBase != nullptr)
			{
				FlagBase->OnOverlapBegin(FlagBase->Capsule, HoldingPawn, HoldingPawn->GetCapsuleComponent(), 0, false, FHitResult());
			}
		}
	}
}

void AUTFlag::DelayedDropMessage()
{
	if ((LastGameMessageTime < FlagDropTime) && (ObjectState == CarriedObjectState::Dropped))
	{
		SendGameMessage(3, LastHolder, NULL);
		LastDroppedMessageTime = GetWorld()->GetTimeSeconds();
	}
}

void AUTFlag::PlayReturnedEffects()
{
	if (GetNetMode() != NM_DedicatedServer && ReturningMesh == NULL) // don't play a second set if first is still playing (guards against redundant calls to SendHome(), etc)
	{
		if (ReturnParamCurve != NULL)
		{
			GetMesh()->bDisableClothSimulation = true;
			GetMesh()->ClothBlendWeight = ClothBlendHome;
			ReturningMesh = DuplicateObject<USkeletalMeshComponent>(Mesh, this);
			if (GetCachedScalabilityCVars().DetailMode != 0)
			{
				GetMesh()->bDisableClothSimulation = false;
			}
			ReturningMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
			ReturningMeshMID = ReturningMesh->CreateAndSetMaterialInstanceDynamicFromMaterial(0, GetClass()->GetDefaultObject<AUTFlag>()->Mesh->GetMaterial(0));
			ReturningMeshMID->SetScalarParameterValue(NAME_Wipe, ReturnParamCurve->GetFloatValue(0.0f));
			ReturningMesh->RegisterComponent();
		}
		UGameplayStatics::SpawnEmitterAtLocation(this, ReturnSrcEffect, GetActorLocation() + GetRootComponent()->ComponentToWorld.TransformVectorNoScale(GetMesh()->RelativeLocation), GetActorRotation());
		if (HomeBase != NULL)
		{
			UGameplayStatics::SpawnEmitterAtLocation(this, ReturnDestEffect, GetHomeLocation() + FRotationMatrix(GetHomeRotation()).TransformVector(GetMesh()->RelativeLocation), GetHomeRotation());
		}
	}
}

void AUTFlag::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// tick return material effect
	if (ReturningMeshMID != NULL)
	{
		ReturnEffectTime += DeltaTime;
		float MinTime, MaxTime;
		ReturnParamCurve->GetTimeRange(MinTime, MaxTime);
		if (ReturnEffectTime >= MaxTime)
		{
			ReturningMesh->DestroyComponent(false);
			ReturningMesh = NULL;
			ReturningMeshMID = NULL;
			MeshMID->SetScalarParameterValue(NAME_Wipe, 0.0f);
			ReturnEffectTime = 0.0f;
		}
		else
		{
			const float Value = ReturnParamCurve->GetFloatValue(ReturnEffectTime);
			ReturningMeshMID->SetScalarParameterValue(NAME_Wipe, Value);
			MeshMID->SetScalarParameterValue(NAME_Wipe, 1.0f - Value);
		}
	}
	if (Role == ROLE_Authority)
	{
		bool bWasPinged = bCurrentlyPinged;
		bCurrentlyPinged = false;
		AUTGameVolume* GV = HoldingPawn && HoldingPawn->UTCharacterMovement ? Cast<AUTGameVolume>(HoldingPawn->UTCharacterMovement->GetPhysicsVolume()) : nullptr;
		if (Holder)
		{
			//Update currently pinged
			if (bShouldPingFlag)
			{
				bCurrentlyPinged = (GetWorld()->GetTimeSeconds() - LastPingedTime < PingedDuration);
				if (!bCurrentlyPinged && GV && GV->bIsDefenderBase)
				{
					if (GetWorld()->GetTimeSeconds() - EnteredEnemyBaseTime < PingedDuration)
					{
						bCurrentlyPinged = true;
					}
					else if (HoldingPawn->GetController())
					{
						// ping if has LOS to flag base
						// FIXMESTEVE move to blitz flag
						AUTGameObjective* OtherBase = nullptr;
						AUTCTFGameState* CTFGameState = GetWorld()->GetGameState<AUTCTFGameState>();
						if (CTFGameState)
						{
							OtherBase = CTFGameState->FlagBases[1 - GetTeamNum()];
						}
						else
						{
							AUTFlagRunGameState* BlitzGameState = GetWorld()->GetGameState<AUTFlagRunGameState>();
							if (BlitzGameState)
							{
								OtherBase = BlitzGameState->DeliveryPoint;
							}
						}
						if (OtherBase)
						{
							FVector StartLocation = HoldingPawn->GetActorLocation() + FVector(0.f, 0.f, HoldingPawn->BaseEyeHeight);
							FVector BaseLoc = OtherBase->GetActorLocation();
							ECollisionChannel TraceChannel = ECC_Visibility;
							FCollisionQueryParams QueryParams(GetClass()->GetFName(), true, HoldingPawn);
							FHitResult Hit;
							if (!GetWorld()->LineTraceSingleByChannel(Hit, StartLocation, BaseLoc, TraceChannel, QueryParams) || !GetWorld()->LineTraceSingleByChannel(Hit, StartLocation, BaseLoc + FVector(0.f, 0.f, 100.f), TraceChannel, QueryParams))
							{
								bCurrentlyPinged = true;;
							}
						}
					}
				}
				if (!bCurrentlyPinged && HoldingPawn && (GetWorld()->GetTimeSeconds() - HoldingPawn->LastTargetSeenTime < TargetPingedDuration))
				{
					bCurrentlyPinged = true;
				}
			}
			Holder->bSpecialPlayer = bCurrentlyPinged;
			if (bWasPinged != bCurrentlyPinged)
			{
				Holder->ForceNetUpdate();

				// 'pinged' means visible through walls so tell bots about my position
				for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
				{
					AUTBot* B = Cast<AUTBot>(It->Get());
					if (B != NULL && !B->IsTeammate(HoldingPawn))
					{
						B->UpdateEnemyInfo(HoldingPawn, EUT_HeardExact);
					}
				}
			}
			if (GetNetMode() != NM_DedicatedServer)
			{
				Holder->OnRepSpecialPlayer();
			}
		}
		if ((ObjectState == CarriedObjectState::Held) && HoldingPawn)
		{
			bool bAddedReturnSpot = false;
			FVector PreviousPos = (PastPositions.Num() > 0) ? PastPositions[PastPositions.Num() - 1].Location : (HomeBase ? HomeBase->GetActorLocation() : FVector(0.f));
			if (HoldingPawn->GetCharacterMovement() && HoldingPawn->GetCharacterMovement()->IsWalking() && (!HoldingPawn->GetMovementBase() || !MovementBaseUtility::UseRelativeLocation(HoldingPawn->GetMovementBase())))
			{
				bool bAlreadyInNoRallyZone = (PastPositions.Num() > 0) && (PastPositions[PastPositions.Num() - 1].bIsInNoRallyZone || PastPositions[PastPositions.Num() - 1].bEnteringNoRallyZone);
				bool bNowInNoRallyZone = GV && (GV->bIsDefenderBase || GV->bIsTeamSafeVolume);
				bool bJustTransitionedToNoRallyZone = !bAlreadyInNoRallyZone && bNowInNoRallyZone;
				FVector PendingNewPosition = bJustTransitionedToNoRallyZone ? RecentPosition[0] : HoldingPawn->GetActorLocation();
				if ((HoldingPawn->GetActorLocation() - RecentPosition[0]).Size() > 100.f)
				{
					RecentPosition[1] = RecentPosition[0];
					RecentPosition[0] = HoldingPawn->GetActorLocation();
				}
				if ((!bAlreadyInNoRallyZone || !bNowInNoRallyZone) && ((HoldingPawn->GetActorLocation() - PreviousPos).Size() > (bJustTransitionedToNoRallyZone ? 0.3f*MinGradualReturnDist : MinGradualReturnDist))
					&& (!bNowInNoRallyZone || !GV->EncompassesPoint(PendingNewPosition)) )
				{
					FFlagTrailPos NewPosition;
					NewPosition.Location = PendingNewPosition;
					NewPosition.MidPoints[0] = FVector::ZeroVector;
					NewPosition.bEnteringNoRallyZone = bJustTransitionedToNoRallyZone;
					PastPositions.Add(NewPosition);
					MidPointPos = 0;
					bAddedReturnSpot = true;
					for (int32 i = 0; i < NUM_MIDPOINTS; i++)
					{
						MidPoints[i] = FVector::ZeroVector;
					}
				}
			}
			if ((MidPointPos < NUM_MIDPOINTS) && !bAddedReturnSpot)
			{
				static FName NAME_FlagReturnLOS = FName(TEXT("FlagReturnLOS"));
				FCollisionQueryParams CollisionParms(NAME_FlagReturnLOS, true, HoldingPawn);
				FVector TraceEnd = (MidPointPos > 0) ? MidPoints[MidPointPos - 1] : PreviousPos;
				FHitResult Hit;
				bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, HoldingPawn->GetActorLocation(), TraceEnd, ECC_Visibility, CollisionParms);
				if (bHit)
				{
					MidPointPos++;
				}
				else
				{
					MidPoints[MidPointPos] = HoldingPawn->GetActorLocation() - 100.f * HoldingPawn->GetVelocity().GetSafeNormal();
				}
			}
		}
		if ((ObjectState == CarriedObjectState::Dropped) && GetWorldTimerManager().IsTimerActive(SendHomeWithNotifyHandle))
		{
			FlagReturnTime = FMath::Clamp(int32(GetWorldTimerManager().GetTimerRemaining(SendHomeWithNotifyHandle) + 1.f), 0, 255);
			// check if lift has pushed flag into wall
			FVector Adjust(0.f);
			if (RootComponent && RootComponent->GetAttachParent() && Cast<AUTLift>(RootComponent->GetAttachParent()->GetOwner()) && GetWorld()->EncroachingBlockingGeometry(this, GetActorLocation(), GetActorRotation(), &Adjust))
			{
				SendHomeWithNotify();
			}
		}
	}
	if (GetNetMode() != NM_DedicatedServer && ((ObjectState == CarriedObjectState::Dropped) || (ObjectState == CarriedObjectState::Home)))
	{
		for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			APlayerController* PlayerController = Iterator->Get();
			if (PlayerController && PlayerController->IsLocalPlayerController() && PlayerController->GetViewTarget())
			{
				FVector Dir = GetActorLocation() - PlayerController->GetViewTarget()->GetActorLocation();
				FRotator DesiredRot = Dir.Rotation();
				DesiredRot.Yaw += 90.f;
				DesiredRot.Pitch = 0.f;
				DesiredRot.Roll = 0.f;
				GetMesh()->SetWorldRotation(DesiredRot, false);
				break;
			}
		}
	}
}
