// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "SUTPlayerInfoDialog.h"
#include "../SUWindowsStyle.h"
#include "../Widgets/SUTTabWidget.h"
#include "UTCanvasRenderTarget2D.h"
#include "EngineModule.h"
#include "SlateMaterialBrush.h"
#include "UTPlayerCameraManager.h"
#include "UTCharacterContent.h"
#include "UTWeap_ShockRifle.h"
#include "UTWeaponAttachment.h"
#include "Engine/UserInterfaceSettings.h"
#include "UTHUDWidget_ReplayTimeSlider.h"
#include "UTPlayerInput.h"
#include "StatNames.h"
#include "Panels/SUTWebBrowserPanel.h"

#if !UE_SERVER
#include "SlateBasics.h"
#include "SlateExtras.h"
#include "SScaleBox.h"
#include "Widgets/SDragImage.h"

void SUTPlayerInfoDialog::Construct(const FArguments& InArgs)
{
	FVector2D ViewportSize;
	InArgs._PlayerOwner->ViewportClient->GetViewportSize(ViewportSize);
	TargetUniqueId = InArgs._TargetUniqueId;

	FText NewDialogTitle = FText::Format(NSLOCTEXT("SUTMenuBase", "PlayerInfoTitleFormat", "Player Info - {0}"), FText::FromString(InArgs._TargetName));
	SUTDialogBase::Construct(SUTDialogBase::FArguments()
							.PlayerOwner(InArgs._PlayerOwner)
							.DialogTitle(NewDialogTitle)
							.DialogSize(InArgs._DialogSize)
							.DialogPosition(InArgs._DialogPosition)
							.DialogAnchorPoint(InArgs._DialogAnchorPoint)
							.ContentPadding(FVector2D(0,0))
							.IsScrollable(false)
							.ButtonMask(UTDIALOG_BUTTON_OK)
							.OnDialogResult(InArgs._OnDialogResult)
							.bShadow(false)
						);

	PlayerPreviewMesh = nullptr;
	PreviewWeapon = nullptr;
	bSpinPlayer = true;
	ZoomOffset = 90.f;

	PlayerPreviewAnimBlueprint = LoadObject<UClass>(nullptr, TEXT("/Game/RestrictedAssets/UI/ABP_PlayerPreview.ABP_PlayerPreview_C"));
	PlayerPreviewAnimFemaleBlueprint = LoadObject<UClass>(nullptr, TEXT("/Game/RestrictedAssets/UI/ABP_Female_PlayerPreview.ABP_Female_PlayerPreview_C"));

	// allocate a preview scene for rendering
	PlayerPreviewWorld = UWorld::CreateWorld(EWorldType::GamePreview, true);
	PlayerPreviewWorld->bShouldSimulatePhysics = true;
	GEngine->CreateNewWorldContext(EWorldType::GamePreview).SetCurrentWorld(PlayerPreviewWorld);
	PlayerPreviewWorld->InitializeActorsForPlay(FURL(), true);
	ViewState.Allocate();
	{
		UClass* EnvironmentClass = LoadObject<UClass>(nullptr, TEXT("/Game/RestrictedAssets/UI/PlayerPreviewEnvironment.PlayerPreviewEnvironment_C"));
		PreviewEnvironment = PlayerPreviewWorld->SpawnActor<AActor>(EnvironmentClass, FVector(500.f, 50.f, 0.f), FRotator(0, 0, 0));
	}
	
	UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(NULL, TEXT("/Game/RestrictedAssets/UI/PlayerPreviewProxy.PlayerPreviewProxy"));
	if (BaseMat != NULL)
	{
		PlayerPreviewTexture = Cast<UUTCanvasRenderTarget2D>(UCanvasRenderTarget2D::CreateCanvasRenderTarget2D(GetPlayerOwner().Get(), UUTCanvasRenderTarget2D::StaticClass(), ViewportSize.X, ViewportSize.Y));
		PlayerPreviewTexture->ClearColor = FLinearColor(0.0f, 0.0f, 0.0f, 1.0f);
		PlayerPreviewTexture->OnNonUObjectRenderTargetUpdate.BindSP(this, &SUTPlayerInfoDialog::UpdatePlayerRender);
		PlayerPreviewMID = UMaterialInstanceDynamic::Create(BaseMat, PlayerPreviewWorld);
		PlayerPreviewMID->SetTextureParameterValue(FName(TEXT("TheTexture")), PlayerPreviewTexture);
		PlayerPreviewBrush = new FSlateMaterialBrush(*PlayerPreviewMID, ViewportSize);
	}
	else
	{
		PlayerPreviewTexture = NULL;
		PlayerPreviewMID = NULL;
		PlayerPreviewBrush = new FSlateMaterialBrush(*UMaterial::GetDefaultMaterial(MD_Surface), ViewportSize);
	}

	PlayerPreviewTexture->TargetGamma = GEngine->GetDisplayGamma();
	PlayerPreviewTexture->InitCustomFormat(ViewportSize.X, ViewportSize.Y, PF_B8G8R8A8, false);
	PlayerPreviewTexture->UpdateResourceImmediate();

	FVector2D ResolutionScale(ViewportSize.X / 1280.0f, ViewportSize.Y / 720.0f);

	if (DialogContent.IsValid())
	{
		const float MessageTextPaddingX = 10.0f;
		TSharedPtr<STextBlock> MessageTextBlock;
		DialogContent->AddSlot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SBox)
					.WidthOverride(850)
					[
						SAssignNew(ModelBox, SVerticalBox)
						+SVerticalBox::Slot().FillHeight(1.0f).VAlign(VAlign_Center)
						[
							SNew(SVerticalBox)
							+SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
							[
								SNew(STextBlock)
								.Text(NSLOCTEXT("SUTPlayerInfoDialog", "Loading", "Requesting Player Information..."))
								.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Medium.Bold")
							]
							+SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
							[
								SNew(SThrobber)
							]
						]
					]
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0)
				[
					SNew(SVerticalBox)
					+SVerticalBox::Slot()
					.FillHeight(1.0)
					[
						SNew(SScrollBox)
						+ SScrollBox::Slot()
						[
							SAssignNew(InfoPanel, SOverlay)
						]
					]
				]
			]
		];
	}

	OnUniqueIdChanged();


	// Turn on Screen Space Reflection max quality
	auto SSRQualityCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.SSR.Quality"));
	OldSSRQuality = SSRQualityCVar->GetInt();
	SSRQualityCVar->Set(4, ECVF_SetByCode);
}

SUTPlayerInfoDialog::~SUTPlayerInfoDialog()
{
	// Reset Screen Space Reflection max quality, wish there was a cleaner way to reset the flags
	auto SSRQualityCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.SSR.Quality"));
	EConsoleVariableFlags Flags = SSRQualityCVar->GetFlags();
	Flags = (EConsoleVariableFlags)(((uint32)Flags & ~ECVF_SetByMask) | ECVF_SetByScalability);
	SSRQualityCVar->Set(OldSSRQuality, ECVF_SetByCode);
	SSRQualityCVar->SetFlags(Flags);

	if (!GExitPurge)
	{
		if (PlayerPreviewTexture != NULL)
		{
			PlayerPreviewTexture->OnNonUObjectRenderTargetUpdate.Unbind();
			PlayerPreviewTexture = NULL;
		}
		FlushRenderingCommands();
		if (PlayerPreviewBrush != NULL)
		{
			// FIXME: Slate will corrupt memory if this is deleted. Must be referencing it somewhere that doesn't get cleaned up...
			//		for now, we'll take the minor memory leak (the texture still gets GC'ed so it's not too bad)
			//delete PlayerPreviewBrush;
			PlayerPreviewBrush->SetResourceObject(NULL);
			PlayerPreviewBrush = NULL;
		}
		if (PlayerPreviewWorld != NULL)
		{
			PlayerPreviewWorld->DestroyWorld(true);
			GEngine->DestroyWorldContext(PlayerPreviewWorld);
			PlayerPreviewWorld = NULL;
			GetPlayerOwner()->GetWorld()->ForceGarbageCollection(true);
		}
	}
	ViewState.Destroy();
}

void SUTPlayerInfoDialog::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(PlayerPreviewMesh);
	Collector.AddReferencedObject(PreviewWeapon);
	Collector.AddReferencedObject(PreviewEnvironment);
	Collector.AddReferencedObject(PlayerPreviewTexture);
	Collector.AddReferencedObject(PlayerPreviewMID);
	Collector.AddReferencedObject(PlayerPreviewWorld);
	Collector.AddReferencedObject(PlayerPreviewAnimBlueprint);
	Collector.AddReferencedObject(PlayerPreviewAnimFemaleBlueprint);
}

void SUTPlayerInfoDialog::OnUniqueIdChanged()
{

	// Start off by looking to see if we have an assoicated player state that matches this unique id

	TargetPlayerState.Reset();

	AUTGameState* UTGameState = PlayerOwner->GetWorld()->GetGameState<AUTGameState>();
	if (UTGameState)
	{
		for (int32 i=0; i < UTGameState->PlayerArray.Num(); i++)
		{
			if (UTGameState->PlayerArray[i] && UTGameState->PlayerArray[i]->UniqueId.ToString().Equals(TargetUniqueId, ESearchCase::IgnoreCase))
			{
				TargetPlayerState = Cast<AUTPlayerState>(UTGameState->PlayerArray[i]);
				break;
			}
		}
	}

	if (TargetPlayerState.IsValid())
	{
		// We have the player state, so we can instantly use it to get the latest configuration data.
		
		CharacterClass = TargetPlayerState->GetSelectedCharacter();
		HatClass = TargetPlayerState->HatClass;
		HatVariant = TargetPlayerState->HatVariant;
		EyewearClass = TargetPlayerState->EyewearClass;
		EyewearVariant = TargetPlayerState->EyewearVariant;
	}
	else
	{
		// We need to hide the model preview until we get it from the MCP	

		IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
		if (OnlineSubsystem && OnlineSubsystem->GetUserCloudInterface().IsValid())
		{
			OnReadUserFileCompleteDelegate = OnlineSubsystem->GetUserCloudInterface()->AddOnReadUserFileCompleteDelegate_Handle(FOnReadUserFileCompleteDelegate::CreateSP(this, &SUTPlayerInfoDialog::OnReadUserFileComplete));

			FUniqueNetId* Id = new FUniqueNetIdString(TargetUniqueId);
			OnlineSubsystem->GetUserCloudInterface()->ReadUserFile(*Id, PlayerOwner->GetProfileFilename());
			delete Id;
		}
	}

	UpdatePlayerCustomization();
	TabWidget->SelectTab(0);
}


void SUTPlayerInfoDialog::OnReadUserFileComplete(bool bWasSuccessful, const FUniqueNetId& InUserId, const FString& FileName)
{
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		IOnlineUserCloudPtr CloudInterface = OnlineSubsystem->GetUserCloudInterface();
		if (CloudInterface.IsValid())
		{
			CloudInterface->ClearOnReadUserFileCompleteDelegate_Handle(OnReadUserFileCompleteDelegate);

			if (bWasSuccessful)
			{
				TArray<uint8> FileContents;
				CloudInterface->GetFileContents(InUserId, FileName, FileContents);
				UUTProfileSettings* TargetProfileSettings = PlayerOwner->CreateProfileSettingsObject(FileContents);
				if (TargetProfileSettings != nullptr)
				{
					CharacterClass = (!TargetProfileSettings->CharacterPath.IsEmpty()) ? TSubclassOf<AUTCharacterContent>(FindObject<UClass>(NULL, *TargetProfileSettings->CharacterPath, false)) : GetDefault<AUTCharacter>()->CharacterData;
					HatClass = (!TargetProfileSettings->HatPath.IsEmpty()) ? LoadClass<AUTHat>(NULL, *TargetProfileSettings->HatPath, NULL, LOAD_NoWarn, NULL) : nullptr;
					HatVariant = TargetProfileSettings->HatVariant;
					EyewearClass = (!TargetProfileSettings->EyewearPath.IsEmpty()) ? LoadClass<AUTEyewear>(NULL, *TargetProfileSettings->EyewearPath, NULL, LOAD_NoWarn, NULL) : nullptr;
					EyewearVariant = TargetProfileSettings->EyewearVariant;
					UpdatePlayerCustomization();
				}
			}
		}
	}
}



FReply SUTPlayerInfoDialog::OnButtonClick(uint16 ButtonID)
{
	if (ButtonID == UTDIALOG_BUTTON_OK) 
	{
		GetPlayerOwner()->CloseDialog(SharedThis(this));	
	}
	return FReply::Handled();
}

void SUTPlayerInfoDialog::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SUTDialogBase::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	if (PlayerPreviewWorld != nullptr)
	{
		PlayerPreviewWorld->Tick(LEVELTICK_All, InDeltaTime);
	}
	
	// Force the preview mesh to put the highest mips into memory
	if (PlayerPreviewMesh != nullptr)
	{
		PlayerPreviewMesh->PrestreamTextures(1, true);
		if (PlayerPreviewMesh->Hat)
		{
			PlayerPreviewMesh->Hat->PrestreamTextures(1, true);
		}
	}
	if (PreviewWeapon)
	{
		PreviewWeapon->PrestreamTextures(1, true);
	}

	if ( PlayerPreviewTexture != nullptr )
	{
		PlayerPreviewTexture->FastUpdateResource();
	}
}

void SUTPlayerInfoDialog::RecreatePlayerPreview()
{
	if (ModelBox.IsValid())
	{
		ModelBox->ClearChildren();
		ModelBox->AddSlot().FillHeight(1.0f)
		[
			SNew(SScaleBox)
			.Stretch(EStretch::ScaleToFill)
			[
				SNew(SDragImage)
				.Image(PlayerPreviewBrush)
				.OnDrag(this, &SUTPlayerInfoDialog::DragPlayerPreview)
				.OnZoom(this, &SUTPlayerInfoDialog::ZoomPlayerPreview)
			]
		];
	}

	FRotator ActorRotation = FRotator(0.0f, 180.0f, 0.0f);

	// If we haven't received a character class yet, then just don't change anything
	if (CharacterClass == nullptr) return;

	if (PlayerPreviewMesh != nullptr)
	{
		ActorRotation = PlayerPreviewMesh->GetActorRotation();
		PlayerPreviewMesh->Destroy();
	}

	if ( PreviewWeapon != nullptr )
	{
		PreviewWeapon->Destroy();
	}

	AGameStateBase* GameState = GetPlayerOwner()->GetWorld()->GetGameState();
	const AUTBaseGameMode* DefaultGameMode = GameState->GetDefaultGameMode<AUTBaseGameMode>();
	if (DefaultGameMode)
	{
		TSubclassOf<class APawn> DefaultPawnClass = Cast<UClass>(StaticLoadObject(UClass::StaticClass(), NULL, *DefaultGameMode->PlayerPawnObject.ToStringReference().ToString(), NULL, LOAD_NoWarn));

		//For game modes without a default pawn class (menu game modes), spawn our default one
		if (DefaultPawnClass == nullptr)
		{
			DefaultPawnClass = Cast<UClass>(StaticLoadObject(UClass::StaticClass(), NULL, *AUTGameMode::StaticClass()->GetDefaultObject<AUTGameMode>()->PlayerPawnObject.ToStringReference().ToString(), NULL, LOAD_NoWarn));
		}

		PlayerPreviewMesh = PlayerPreviewWorld->SpawnActor<AUTCharacter>(DefaultPawnClass, FVector(300.0f, 0.f, 4.f), ActorRotation);
		if (PlayerPreviewMesh && PlayerPreviewMesh->GetMesh())
		{
			PlayerPreviewMesh->ApplyCharacterData(CharacterClass);
			PlayerPreviewMesh->SetHatClass(HatClass);
			PlayerPreviewMesh->SetHatVariant(HatVariant);
			PlayerPreviewMesh->SetEyewearClass(EyewearClass);
			PlayerPreviewMesh->SetEyewearVariant(EyewearVariant);
						
			if (TargetPlayerState.IsValid() && TargetPlayerState->Team != NULL)
			{
				float SkinSelect = TargetPlayerState->Team->TeamIndex;
				
				for (UMaterialInstanceDynamic* MI : PlayerPreviewMesh->GetBodyMIs())
				{
					MI->SetScalarParameterValue(TEXT("TeamSelect"), SkinSelect);
				}
			}

			PlayerPreviewMesh->GetMesh()->MeshComponentUpdateFlag = EMeshComponentUpdateFlag::AlwaysTickPoseAndRefreshBones;
			if (CharacterClass.GetDefaultObject()->bIsFemale)
			{
				if (PlayerPreviewAnimFemaleBlueprint)
				{
					PlayerPreviewMesh->GetMesh()->SetAnimInstanceClass(PlayerPreviewAnimFemaleBlueprint);
				}
			}
			else
			{
				if (PlayerPreviewAnimBlueprint)
				{
					PlayerPreviewMesh->GetMesh()->SetAnimInstanceClass(PlayerPreviewAnimBlueprint);
				}
			}

			// FIXME: Figure out the favorite weapon
			UClass* PreviewAttachmentType = nullptr; //TargetPlayerState.IsValid() && TargetPlayerState->FavoriteWeapon ? TargetPlayerState->FavoriteWeapon->GetDefaultObject<AUTWeapon>()->AttachmentType : NULL;
			if (!PreviewAttachmentType)
			{
				PreviewAttachmentType = LoadClass<AUTWeaponAttachment>(NULL, TEXT("/Game/RestrictedAssets/Weapons/ShockRifle/ShockAttachment.ShockAttachment_C"), NULL, LOAD_None, NULL);
			}

			if (PreviewAttachmentType != NULL)
			{
				PreviewWeapon = PlayerPreviewWorld->SpawnActor<AUTWeaponAttachment>(PreviewAttachmentType, FVector(0, 0, 0), FRotator(0, 0, 0));
				PreviewWeapon->Instigator = PlayerPreviewMesh;
			}

			// Tick the world to make sure the animation is up to date.
			if ( PlayerPreviewWorld != nullptr )
			{
				PlayerPreviewWorld->Tick(LEVELTICK_All, 0.0);
			}

			if ( PreviewWeapon )
			{
				PreviewWeapon->BeginPlay();
				PreviewWeapon->AttachToOwner();
			}
		}
		else
		{
			UE_LOG(UT,Log,TEXT("Could not spawn the player's mesh (DefaultPawnClass = %s"), *DefaultGameMode->DefaultPawnClass->GetFullName());
		}
	}
}


void SUTPlayerInfoDialog::UpdatePlayerRender(UCanvas* C, int32 Width, int32 Height)
{
	FEngineShowFlags ShowFlags(ESFIM_Game);
	ShowFlags.SetMotionBlur(false);
	ShowFlags.SetGrain(false);
	FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(PlayerPreviewTexture->GameThread_GetRenderTargetResource(), PlayerPreviewWorld->Scene, ShowFlags).SetRealtimeUpdate(true));

	FVector CameraPosition(ZoomOffset, 0, -60);

	const float FOV = 45;
	const float AspectRatio = Width / (float)Height;

	FSceneViewInitOptions PlayerPreviewInitOptions;
	PlayerPreviewInitOptions.SetViewRectangle(FIntRect(0, 0, C->SizeX, C->SizeY));
	PlayerPreviewInitOptions.ViewOrigin = -CameraPosition;
	PlayerPreviewInitOptions.ViewRotationMatrix = FMatrix(FPlane(0, 0, 1, 0), FPlane(1, 0, 0, 0), FPlane(0, 1, 0, 0), FPlane(0, 0, 0, 1));
	PlayerPreviewInitOptions.ProjectionMatrix = 
		FReversedZPerspectiveMatrix(
			FMath::Max(0.001f, FOV) * (float)PI / 360.0f,
			AspectRatio,
			1.0f,
			GNearClippingPlane );
	PlayerPreviewInitOptions.ViewFamily = &ViewFamily;
	PlayerPreviewInitOptions.SceneViewStateInterface = ViewState.GetReference();
	PlayerPreviewInitOptions.BackgroundColor = FLinearColor::Black;
	PlayerPreviewInitOptions.WorldToMetersScale = GetPlayerOwner()->GetWorld()->GetWorldSettings()->WorldToMeters;
	PlayerPreviewInitOptions.CursorPos = FIntPoint(-1, -1);
	
	ViewFamily.bUseSeparateRenderTarget = true;

	FSceneView* View = new FSceneView(PlayerPreviewInitOptions); // note: renderer gets ownership
	View->ViewLocation = FVector::ZeroVector;
	View->ViewRotation = FRotator::ZeroRotator;
	FPostProcessSettings PPSettings = GetDefault<AUTPlayerCameraManager>()->DefaultPPSettings;

	ViewFamily.Views.Add(View);

	View->StartFinalPostprocessSettings(CameraPosition);
	View->EndFinalPostprocessSettings(PlayerPreviewInitOptions);
	View->ViewRect = View->UnscaledViewRect;

	// workaround for hacky renderer code that uses GFrameNumber to decide whether to resize render targets
	--GFrameNumber;
	GetRendererModule().BeginRenderingViewFamily(C->Canvas, &ViewFamily);
}

void SUTPlayerInfoDialog::DragPlayerPreview(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (PlayerPreviewMesh != nullptr)
	{
		bSpinPlayer = false;
		PlayerPreviewMesh->SetActorRotation(PlayerPreviewMesh->GetActorRotation() + FRotator(0, 0.2f * -MouseEvent.GetCursorDelta().X, 0.0f));
	}
}

void SUTPlayerInfoDialog::ZoomPlayerPreview(float WheelDelta)
{
	ZoomOffset = FMath::Clamp(ZoomOffset + (-WheelDelta * 5.0f), -100.0f, 400.0f);
}


FReply SUTPlayerInfoDialog::OnSendFriendRequest()
{
/*
	if (FriendStatus != FFriendsStatus::FriendRequestPending && FriendStatus != FFriendsStatus::IsBot)
	{
		GetPlayerOwner()->RequestFriendship(TargetUniqueId);

		FriendPanel->ClearChildren();
		FriendPanel->AddSlot()
			.Padding(10.0, 0.0, 0.0, 0.0)
			[
				SNew(STextBlock)
				.Text(NSLOCTEXT("SUTPlayerInfoDialog", "FriendRequestPending", "You have sent a friend request..."))
				.TextStyle(SUWindowsStyle::Get(), "UT.TopMenu.Button.SmallTextStyle")
			];

		FriendStatus = FFriendsStatus::FriendRequestPending;
	}
*/
	return FReply::Handled();
}

FText SUTPlayerInfoDialog::GetFunnyText()
{
	return NSLOCTEXT("SUTPlayerInfoDialog", "FunnyDefault", "Viewing self.");
}

void SUTPlayerInfoDialog::BuildFriendPanel()
{
/*
	if (TargetPlayerState == NULL) return;

	FName NewFriendStatus;
	if (GetPlayerOwner()->PlayerController->PlayerState == TargetPlayerState)
	{
		NewFriendStatus = FFriendsStatus::IsYou;
	}
	else if (TargetPlayerState->bIsABot)
	{
		NewFriendStatus = FFriendsStatus::IsBot;
	}
	else
	{
		NewFriendStatus = GetPlayerOwner()->IsAFriend(TargetPlayerState->UniqueId) ? FFriendsStatus::Friend : FFriendsStatus::NotAFriend;
	}

	bool bRequiresRefresh = false;
	if (FriendStatus == FFriendsStatus::FriendRequestPending)
	{
		if (NewFriendStatus == FFriendsStatus::Friend)
		{
			FriendStatus = NewFriendStatus;
			bRequiresRefresh = true;
		}
	}
	else
	{
		bRequiresRefresh = FriendStatus != NewFriendStatus;
		FriendStatus = NewFriendStatus;
	}

	if (bRequiresRefresh)
	{
		FriendPanel->ClearChildren();
		if (FriendStatus == FFriendsStatus::IsYou)
		{
			FText FunnyText = GetFunnyText();
			FriendPanel->AddSlot()
			.Padding(10.0, 0.0, 0.0, 0.0)
				[
					SNew(STextBlock)
					.Text(FunnyText)
					.TextStyle(SUWindowsStyle::Get(), "UT.TopMenu.Button.SmallTextStyle")
				];
		}
		else if (FriendStatus == FFriendsStatus::IsBot)
		{
			FriendPanel->AddSlot()
				.Padding(10.0, 0.0, 0.0, 0.0)
				[
					SNew(STextBlock)
					.Text(NSLOCTEXT("SUTPlayerInfoDialog", "IsABot", "AI (C) Liandri Corp."))
					.TextStyle(SUWindowsStyle::Get(), "UT.TopMenu.Button.SmallTextStyle")
				];
		}
		else if (FriendStatus == FFriendsStatus::Friend)
		{
			FriendPanel->AddSlot()
				.Padding(10.0, 0.0, 0.0, 0.0)
				[
					SNew(STextBlock)
					.Text(NSLOCTEXT("SUTPlayerInfoDialog", "IsAFriend", "Is your friend"))
					.TextStyle(SUWindowsStyle::Get(), "UT.TopMenu.Button.SmallTextStyle")
				];
		}
		else if (FriendStatus == FFriendsStatus::FriendRequestPending)
		{
		}
		else
		{
			FriendPanel->AddSlot()
				.Padding(10.0, 0.0, 0.0, 0.0)
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.ButtonStyle(SUWindowsStyle::Get(), "UT.BottomMenu.Button")
					.ContentPadding(FMargin(5.0f, 5.0f, 5.0f, 5.0f))
					.Text(NSLOCTEXT("SUTPlayerInfoDialog", "SendFriendRequest", "Send Friend Request"))
					.TextStyle(SUWindowsStyle::Get(), "UT.TopMenu.Button.SmallTextStyle")
					.OnClicked(this, &SUTPlayerInfoDialog::OnSendFriendRequest)
				];
		}
	}
*/
}


FReply SUTPlayerInfoDialog::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	//Close with escape
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		GetPlayerOwner()->CloseDialog(SharedThis(this));
		return FReply::Handled();
	}

	//If the key matches the TogglePlayerInfo spectator bind then close the dialog
	FName KeyName = InKeyEvent.GetKey().GetFName();

	UUTPlayerInput* UTInput = Cast<UUTPlayerInput>(GetPlayerOwner()->PlayerController->PlayerInput);
	if (UTInput != nullptr)
	{
		for (auto& Bind : UTInput->SpectatorBinds)
		{
			if (Bind.Command == TEXT("TogglePlayerInfo") && KeyName == Bind.KeyName)
			{
				GetPlayerOwner()->CloseDialog(SharedThis(this));
				return FReply::Handled();
			}
		}
	}

	if (InKeyEvent.GetKey() == EKeys::Left)
	{
		PreviousPlayer();
	}
	else if (InKeyEvent.GetKey() == EKeys::Right)
	{
		NextPlayer();
	}

	return FReply::Unhandled();
}

void SUTPlayerInfoDialog::OnTabButtonSelectionChanged(const FText& NewText)
{
	static const FText Score = NSLOCTEXT("AUTGameMode", "Score", "Score");
	static const FText Weapons = NSLOCTEXT("AUTGameMode", "Weapons", "Weapons");
	static const FText Rewards = NSLOCTEXT("AUTGameMode", "Rewards", "Rewards");
	static const FText Movement = NSLOCTEXT("AUTGameMode", "Movement", "Movement");

	AUTPlayerController* UTPC = Cast<AUTPlayerController>(GetPlayerOwner()->PlayerController);
	if (UTPC != nullptr)
	{
		if (NewText.EqualTo(Score))
		{
			UTPC->ServerSetViewedScorePS(TargetPlayerState.Get(), 0);
		}
		else if (NewText.EqualTo(Weapons))
		{
			UTPC->ServerSetViewedScorePS(TargetPlayerState.Get(), 1);
		}
		else if (NewText.EqualTo(Rewards))
		{
			UTPC->ServerSetViewedScorePS(TargetPlayerState.Get(), 2);
		}
		else if (NewText.EqualTo(Movement))
		{
			UTPC->ServerSetViewedScorePS(TargetPlayerState.Get(), 3);
		}
		else
		{
			UTPC->ServerSetViewedScorePS(nullptr, 0);
		}
	}

	CurrentTab = NewText;
}

FReply SUTPlayerInfoDialog::NextPlayer()
{
/*
	if (TargetPlayerState.IsValid())
	{
		TargetPlayerState = GetNextPlayerState(1);
		if (TargetPlayerState.IsValid())
		{
			OnUpdatePlayerState();
		}
		else
		{
			GetPlayerOwner()->CloseDialog(SharedThis(this));
		}
	}
*/	
	return FReply::Handled();
}
FReply SUTPlayerInfoDialog::PreviousPlayer()
{
/*
	if (TargetPlayerState.IsValid())
	{
		TargetPlayerState = GetNextPlayerState(-1);
		if (TargetPlayerState.IsValid())
		{
			OnUpdatePlayerState();
		}
		else
		{
			GetPlayerOwner()->CloseDialog(SharedThis(this));
		}
	}
*/
	return FReply::Handled();
}

AUTPlayerState* SUTPlayerInfoDialog::GetNextPlayerState(int32 dir)
{
	int32 CurrentIndex = -1;	
	if (TargetPlayerState.IsValid())
	{
		UWorld* World = TargetPlayerState->GetWorld();
		AGameState* GameState = World->GetGameState<AGameState>();

		// Find index of current viewtarget's PlayerState
		for (int32 i = 0; i < GameState->PlayerArray.Num(); i++)
		{
			if (TargetPlayerState.Get() == GameState->PlayerArray[i])
			{
				CurrentIndex = i;
				break;
			}
		}

		// Find next valid viewtarget in appropriate direction
		int32 NewIndex;
		for (NewIndex = CurrentIndex + dir; (NewIndex >= 0) && (NewIndex < GameState->PlayerArray.Num()); NewIndex = NewIndex + dir)
		{
			AUTPlayerState* const PlayerState = Cast<AUTPlayerState>(GameState->PlayerArray[NewIndex]);
			if ((PlayerState != NULL) && (!PlayerState->bOnlySpectator))
			{
				return PlayerState;
			}
		}

		// wrap around
		CurrentIndex = (NewIndex < 0) ? GameState->PlayerArray.Num() : -1;
		for (NewIndex = CurrentIndex + dir; (NewIndex >= 0) && (NewIndex < GameState->PlayerArray.Num()); NewIndex = NewIndex + dir)
		{
			AUTPlayerState* const PlayerState = Cast<AUTPlayerState>(GameState->PlayerArray[NewIndex]);
			if ((PlayerState != NULL) && (!PlayerState->bOnlySpectator))
			{
				return PlayerState;
			}
		}
	}
	return NULL;
}

void SUTPlayerInfoDialog::UpdatePlayerCustomization()
{
	UpdatePlayerStateInReplays();

	StatList.Empty();
	InfoPanel->ClearChildren();

	InfoPanel->AddSlot()
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Fill)
	[
		SAssignNew(TabWidget, SUTTabWidget)
		.OnTabButtonSelectionChanged(this, &SUTPlayerInfoDialog::OnTabButtonSelectionChanged)
	];

	CreatePlayerTab();

	if (TargetPlayerState.IsValid())
	{
		//Draw the game specific stats
		AGameStateBase* GameState = GetPlayerOwner()->GetWorld()->GetGameState();
		if (GameState && !TargetPlayerState->bOnlySpectator)
		{
			AUTBaseGameMode* UTDefaultGameMode = const_cast<AUTBaseGameMode*>(GameState->GetDefaultGameMode<AUTBaseGameMode>());
			if (UTDefaultGameMode)
			{
				UTDefaultGameMode->BuildPlayerInfo(TargetPlayerState.Get(), TabWidget, StatList);
			}
		}
	}

	FriendStatus = NAME_None;
	RecreatePlayerPreview();

	TabWidget->OnButtonClicked(CurrentTab);
}

void SUTPlayerInfoDialog::CreatePlayerTab()
{
	// Add code here to grab a different URL based on the epicapp id.  
	FString PlayerInfoURL = TEXT("https://epicgames-gamedev.ol.epicgames.net/unrealtournament/playerCard?playerId=");

	const IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();

	if (OnlineSub != nullptr)
	{
		EOnlineEnvironment::Type Env = OnlineSub->GetOnlineEnvironment();
		if (Env == EOnlineEnvironment::Production)
		{
			PlayerInfoURL = TEXT("https://www.epicgames.com/unrealtournament/playerCard?playerId=");
		}
	}
		
	PlayerInfoURL += TargetUniqueId;
												   		
	TabWidget->AddTab(NSLOCTEXT("AUTPlayerState", "PlayerInfo", "Player Info"),
		SAssignNew(PlayerCardBox,SVerticalBox)
		+SVerticalBox::Slot()
		.Padding(10.0f, 20.0f, 10.0f, 5.0f)
		.AutoHeight().HAlign(HAlign_Fill).VAlign(VAlign_Center)
		[
			SNew(SBox).HeightOverride(890)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot().FillWidth(1.0).VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.Text(NSLOCTEXT("SUTPlayerInfoDialog", "Loading", "Requesting Player Information..."))
						.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Medium.Bold")
					]
					+SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[
						SNew(SThrobber)
					]
				]
			]
		]
	);

	if ( PlayerOwner.IsValid() )
	{
		if (PlayerCardWebBrowser.IsValid())
		{
			PlayerCardWebBrowser->Browse(PlayerInfoURL);
		}
		else
		{
			SAssignNew(PlayerCardWebBrowser, SUTWebBrowserPanel, PlayerOwner)
			.InitialURL(PlayerInfoURL)
			.ShowControls(false)
			.OnLoadCompleted(FSimpleDelegate::CreateSP(this, &SUTPlayerInfoDialog::OnPlayerCardLoadCompleted))
			.OnLoadError(FSimpleDelegate::CreateSP(this, &SUTPlayerInfoDialog::OnPlayerCardLoadError));
		}
	}

}

void SUTPlayerInfoDialog::OnPlayerCardLoadCompleted()
{
	if (PlayerCardBox.IsValid() && PlayerCardWebBrowser.IsValid())
	{
		PlayerCardBox->ClearChildren();
		if (true)
		{
			PlayerCardBox->AddSlot()
				.Padding(10.0f, 20.0f, 10.0f, 5.0f)
				.AutoHeight().HAlign(HAlign_Fill)
				[
					SNew(SBox).HeightOverride(890)
					[
						PlayerCardWebBrowser.ToSharedRef()
					]
				];
		}
		else
		{
			PlayerCardBox->AddSlot()
				.Padding(10.0f, 20.0f, 10.0f, 5.0f)
				.AutoHeight().HAlign(HAlign_Fill)
				[
					SNew(STextBlock)
					.Text(FText(NSLOCTEXT("AUTPlayerState", "LoadingPlayerCardLoadError", "Could not load player information from the NEG player database.  Please try again later.")))
				.TextStyle(SUWindowsStyle::Get(), "UT.Common.ButtonText.White")
				];
		}
	}

}

void SUTPlayerInfoDialog::OnPlayerCardLoadError()
{
}



void SUTPlayerInfoDialog::UpdatePlayerStateInReplays()
{
	if (TargetPlayerState.IsValid() && TargetPlayerState->IsOwnedByReplayController())
	{
		FMMREntry DuelMMR;
		FMMREntry CTFMMR;
		FMMREntry TDMMMR;
		FMMREntry DMMMR;
		FMMREntry ShowdownMMR;
		FMMREntry FlagRunMMR;

		PlayerOwner->GetMMREntry(NAME_SkillRating.ToString(), DuelMMR);
		PlayerOwner->GetMMREntry(NAME_CTFSkillRating.ToString(), CTFMMR);
		PlayerOwner->GetMMREntry(NAME_TDMSkillRating.ToString(), TDMMMR);
		PlayerOwner->GetMMREntry(NAME_DMSkillRating.ToString(), DMMMR);
		PlayerOwner->GetMMREntry(NAME_ShowdownSkillRating.ToString(), ShowdownMMR);
		PlayerOwner->GetMMREntry(NAME_FlagRunSkillRating.ToString(), FlagRunMMR);

		UpdatePlayerStateRankingStatsFromLocalPlayer(
			DuelMMR.MMR,
			CTFMMR.MMR,
			TDMMMR.MMR,
			DMMMR.MMR,
			ShowdownMMR.MMR,
			FlagRunMMR.MMR,
			PlayerOwner->GetTotalChallengeStars(), 
			FMath::Min(255, DuelMMR.MatchesPlayed),
			FMath::Min(255, CTFMMR.MatchesPlayed),
			FMath::Min(255, TDMMMR.MatchesPlayed),
			FMath::Min(255, DMMMR.MatchesPlayed),
			FMath::Min(255, ShowdownMMR.MatchesPlayed),
			FMath::Min(255, FlagRunMMR.MatchesPlayed));

		UpdatePlayerCharacterPreviewInReplays();
	}
}

void SUTPlayerInfoDialog::UpdatePlayerStateRankingStatsFromLocalPlayer(int32 NewDuelRank, int32 NewCTFRank, int32 NewTDMRank, int32 NewDMRank, int32 NewShowdownRank, int32 NewFlagRunRank, int32 TotalStars, uint8 DuelMatchesPlayed, uint8 CTFMatchesPlayed, uint8 TDMMatchesPlayed, uint8 DMMatchesPlayed, uint8 ShowdownMatchesPlayed, uint8 FlagRunMatchesPlayed)
{
	if (TargetPlayerState.IsValid())
	{
		TargetPlayerState->DuelRank = NewDuelRank;
		TargetPlayerState->CTFRank = NewCTFRank;
		TargetPlayerState->TDMRank = NewTDMRank;
		TargetPlayerState->DMRank = NewDMRank;
		TargetPlayerState->FlagRunRank = NewFlagRunRank;
		TargetPlayerState->ShowdownRank = NewShowdownRank;
		TargetPlayerState->TotalChallengeStars = TotalStars;
		TargetPlayerState->DuelMatchesPlayed = DuelMatchesPlayed;
		TargetPlayerState->CTFMatchesPlayed = CTFMatchesPlayed;
		TargetPlayerState->TDMMatchesPlayed = TDMMatchesPlayed;
		TargetPlayerState->DMMatchesPlayed = DMMatchesPlayed;
		TargetPlayerState->FlagRunMatchesPlayed = FlagRunMatchesPlayed;
		TargetPlayerState->ShowdownMatchesPlayed = ShowdownMatchesPlayed;
	}
}

void SUTPlayerInfoDialog::UpdatePlayerCharacterPreviewInReplays()
{
	if (TargetPlayerState.IsValid() && PlayerOwner.IsValid() && PlayerOwner->GetProfileSettings())
	{
		TargetPlayerState->SetCharacter(PlayerOwner->GetProfileSettings()->CharacterPath);
	}
}

TSharedRef<class SWidget> SUTPlayerInfoDialog::BuildTitleBar(FText InDialogTitle)
{
	if (TargetPlayerState.IsValid())
	{
		UWorld* World = TargetPlayerState->GetWorld();
		if (World && World->GetGameState() && (World->GetGameState()->PlayerArray.Num() > 1))
		{
			return SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.Padding(20.0f, 0.0f, 0.0f, 0.0f)
				.AutoWidth()
				[
					SNew(SButton)
					.HAlign(HAlign_Left)
					.ButtonStyle(SUWindowsStyle::Get(), "UT.BottomMenu.Button")
					.ContentPadding(FMargin(5.0f, 5.0f, 5.0f, 5.0f))
					.Text(NSLOCTEXT("SUTPlayerInfoDialog", "PreviousPlayer", "<- Previous"))
					.TextStyle(SUWindowsStyle::Get(), "UT.TopMenu.Button.SmallTextStyle")
					.OnClicked(this, &SUTPlayerInfoDialog::PreviousPlayer)
				]
			+ SHorizontalBox::Slot()
				.HAlign(HAlign_Fill)
				.FillWidth(1.0f)
				[
					SUTDialogBase::BuildTitleBar(InDialogTitle)
				]
			+ SHorizontalBox::Slot()
				.Padding(0.0f, 0.0f, 20.0f, 0.0f)
				.AutoWidth()
				[
					SNew(SButton)
					.HAlign(HAlign_Right)
					.ButtonStyle(SUWindowsStyle::Get(), "UT.BottomMenu.Button")
					.ContentPadding(FMargin(5.0f, 5.0f, 5.0f, 5.0f))
					.Text(NSLOCTEXT("SUTPlayerInfoDialog", "NextPlayer", "Next ->"))
					.TextStyle(SUWindowsStyle::Get(), "UT.TopMenu.Button.SmallTextStyle")
					.OnClicked(this, &SUTPlayerInfoDialog::NextPlayer)
				];
		}
	}
	return SUTDialogBase::BuildTitleBar(InDialogTitle);
}


#endif