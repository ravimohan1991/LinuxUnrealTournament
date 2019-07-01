// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTLineUpHelper.h"
#include "UTLineUpZone.h"
#include "Net/UnrealNetwork.h"
#include "UTWeaponAttachment.h"
#include "UTHUD.h"
#include "UTCTFGameState.h"
#include "UTCTFGameMode.h"
#include "UTFlagRunGameState.h"
#include "UTPlayerState.h"
#include "UTCustomMovementTypes.h"
#include "UTPickupWeapon.h"
#include "UTSupplyChest.h"
#include "UTMutator.h"
#include "UTWeap_Redeemer.h"
#include "UTWeap_Enforcer.h"
#include "UTWeap_Translocator.h"
#include "UTBlitzFlag.h"
#include "UTDemoRecSpectator.h"

DEFINE_LOG_CATEGORY_STATIC(LogUTLineUp, Log, All);

void AUTLineUpHelper::InitializeLineUp(LineUpTypes LineUpType)
{
	ActiveType = LineUpType;
	StartLineUpWithDelay(CalculateLineUpDelay());
}

void AUTLineUpHelper::CalculateLineUpSlots()
{
	static const int FFATeamNum = 255;

	LineUpSlots.Empty();
	UnusedLineUpSlots.Empty();

	AUTGameState* UTGameState = GetWorld()->GetGameState<AUTGameState>();
	if (UTGameState)
	{
		AUTLineUpZone* ZoneToUse = UTGameState->GetAppropriateSpawnList(ActiveType);
		if (ZoneToUse)
		{
			TArray<AController*> UnassignedControllers;
			for (FConstControllerIterator Iterator = GetWorld()->GetControllerIterator(); Iterator; ++Iterator)
			{
				//Don't put any Demo Rec specators in the list, as they aren't in charge of characters. Also eliminate any controllers without attached player states.
				if ((Cast<AUTDemoRecSpectator>(*Iterator) == nullptr) && (Cast<AUTPlayerState>(Iterator->Get()->PlayerState) != nullptr))
				{
					UnassignedControllers.Add(Iterator->Get());
				}
			}
			SortControllers(UnassignedControllers);

			AUTTeamGameMode* TeamGM = Cast<AUTTeamGameMode>(GetWorld()->GetAuthGameMode());
			int Team1Number = 0;
			int Team2Number = 1;

			//Spawn using Winning / Losing teams instead of team color based teams. This means the red list = winning team and blue list = losing team.
			if (TeamGM && TeamGM->UTGameState && (ActiveType == LineUpTypes::PostMatch || ActiveType == LineUpTypes::Intermission))
			{
				uint8 WinningTeamNum = TeamGM->GetWinningTeamForLineUp();
				if (WinningTeamNum != 255)
				{
					Team1Number = WinningTeamNum;
					Team2Number = 1 - WinningTeamNum;
				}
			}

			for (FLineUpSpawn& Spawn : ZoneToUse->SpawnLocations)
			{
				FLineUpSlot NewSlot;
				NewSlot.SpotLocation = Spawn.Location * ZoneToUse->GetActorTransform();

				//Find the highest rated player controller to fill in this spot
				int TeamNumberToFill = -1;
				switch (Spawn.SpawnType)
				{	
					case LineUpSpawnTypes::Team1:
					case LineUpSpawnTypes::WinningTeam:
					{
						TeamNumberToFill = Team1Number;
						break;
					}
					case LineUpSpawnTypes::Team2:
					case LineUpSpawnTypes::LosingTeam:
					{
						TeamNumberToFill = Team2Number;
						break;
					}
					//Do nothing
					case LineUpSpawnTypes::FFA:
					default:
					{
						break;
					}
				}

				// This is a team slot, need to fill with someone on the correct team
				if (TeamNumberToFill >= 0)
				{
					bool bFoundMatch = false;
					for (int index = 0; index < UnassignedControllers.Num(); ++index)
					{
						const IUTTeamInterface* TeamInterface = Cast<IUTTeamInterface>(UnassignedControllers[index]);
						if (TeamInterface)
						{
							if ((TeamInterface->GetTeamNum() == TeamNumberToFill) || (TeamInterface->GetTeamNum() == FFATeamNum))
							{
								NewSlot.ControllerInSpot = UnassignedControllers[index];
								UnassignedControllers.RemoveAt(index);
								NewSlot.CharacterInSpot = nullptr; //initialize so this shows up null until spawned
								NewSlot.TeamNumOfSlot = TeamNumberToFill;

								//Found highest rated fit, don't need to look further
								LineUpSlots.Add(NewSlot);
								bFoundMatch = true;
								break;
							}
						}
					}

					if (!bFoundMatch)
					{
						//Didn't find a fit. Add to unused slots.
						NewSlot.ControllerInSpot = nullptr;
						NewSlot.CharacterInSpot = nullptr;
						NewSlot.TeamNumOfSlot = TeamNumberToFill;
						UnusedLineUpSlots.Add(NewSlot);
					}
				}
				//Just grab highest rated person for this spot as its FFA
				else
				{
					if (UnassignedControllers.Num() > 0)
					{
						NewSlot.ControllerInSpot = UnassignedControllers[0];
						UnassignedControllers.RemoveAt(0);
						NewSlot.CharacterInSpot = nullptr; //initialize so this shows up null until spawned
						NewSlot.TeamNumOfSlot = FFATeamNum;

						LineUpSlots.Add(NewSlot);
					}
					else
					{
						//Didn't find a fit. Add to unused slots.
						NewSlot.ControllerInSpot = nullptr;
						NewSlot.CharacterInSpot = nullptr;
						NewSlot.TeamNumOfSlot = FFATeamNum;
						UnusedLineUpSlots.Add(NewSlot);
					}
				}
			}
			
			// These are the controllers left
			for (AController* Controller : UnassignedControllers)
			{
				AUTGameMode* UTGM = Cast<AUTGameMode>(GetWorld()->GetGameState());
				if (UTGM)
				{
					UTGM->RestartPlayer(Controller);
				}

				if (Controller->GetPawn())
				{
					Controller->GetPawn()->Destroy();
				}
			}

		}
		UTGameState->LeadLineUpPlayer = ((LineUpSlots.Num() > 0) && (LineUpSlots[0].ControllerInSpot)) ? Cast<AUTPlayerState>(LineUpSlots[0].ControllerInSpot->PlayerState) : nullptr;
	}

	NumControllersUsedInLineUp = LineUpSlots.Num();
}

void AUTLineUpHelper::StartLineUpWithDelay(float TimeDelay)
{
	GetWorld()->GetTimerManager().ClearTimer(DelayedLineUpHandle);
	if ((TimeDelay > SMALL_NUMBER))
	{
		GetWorld()->GetTimerManager().SetTimer(DelayedLineUpHandle, FTimerDelegate::CreateUObject(this, &AUTLineUpHelper::PerformLineUp), TimeDelay, false);
	}
	else
	{
		PerformLineUp();
	}
}

bool AUTLineUpHelper::IsActive()
{
	return bIsActive;
}

float AUTLineUpHelper::CalculateLineUpDelay()
{
	AUTGameMode* UTGM = Cast<AUTGameMode>(GetWorld()->GetAuthGameMode());
	if (UTGM)
	{
		return UTGM->GetLineUpTime(ActiveType);
	}

	return 0.f;
}

AUTLineUpHelper::AUTLineUpHelper(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bAlwaysRelevant = true;

	NumControllersUsedInLineUp = 0;

	bIsPlacingPlayers = false;
	bIsActive = false;

	Intro_TotalSpawnedPlayers = 0;
}

void AUTLineUpHelper::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AUTLineUpHelper, bIsActive);
	DOREPLIFETIME(AUTLineUpHelper, ActiveType);
	DOREPLIFETIME(AUTLineUpHelper, LineUpSlots);
	DOREPLIFETIME(AUTLineUpHelper, NumControllersUsedInLineUp);

}

void AUTLineUpHelper::BeginPlay()
{
	Super::BeginPlay();
	
	BuildMapWeaponList();
}


void AUTLineUpHelper::CleanUp()
{
	//Notify Zone that line up is ending
	AUTGameState* UTGS = GetWorld() ? Cast<AUTGameState>(GetWorld()->GetGameState()) : nullptr;
	if (UTGS && UTGS->GetAppropriateSpawnList(ActiveType))
	{
		UTGS->GetAppropriateSpawnList(ActiveType)->OnLineUpEnd(ActiveType);
	}

	bIsActive = false;
	ActiveType = LineUpTypes::Invalid;

	if (GetWorld())
	{
		for (FConstControllerIterator Iterator = GetWorld()->GetControllerIterator(); Iterator; ++Iterator)
		{
			//skip any demo recorder
			if (Cast<AUTDemoRecSpectator>(*Iterator) != nullptr)
			{
				continue;
			}

			AUTPlayerState* UTPS = Cast<AUTPlayerState>((*Iterator)->PlayerState);
			if (UTPS)
			{
				UTPS->LineUpLocation = INDEX_NONE;
			}

			AUTPlayerController* UTPC = Cast<AUTPlayerController>(*Iterator);
			if (UTPC)
			{
				CleanUpPlayerAfterLineUp(UTPC);
			}
		}

		if (UTGS)
		{
			UTGS->LeadLineUpPlayer = nullptr;

			//If we are in the end game / map vote we don't need to destroy our spawned clones and should 
			//let them stick around and look fancy while voting / stats are being displayed
			if ((UTGS->GetMatchState() != MatchState::WaitingPostMatch) && (UTGS->GetMatchState() != MatchState::MapVoteHappening))
			{
				DestroySpawnedClones();
			}
		}
	}
}
		
void AUTLineUpHelper::CleanUpPlayerAfterLineUp(AUTPlayerController* UTPC)
{
	if (UTPC != nullptr)
	{
		//Clear any active taunts
		AUTCharacter* UTChar = UTPC->GetUTCharacter();
		if (UTChar && UTChar->CurrentTaunt && UTChar->GetMesh())
		{
			UAnimInstance* AnimInstance = UTChar->GetMesh()->GetAnimInstance();
			if (AnimInstance != nullptr)
			{
				AnimInstance->Montage_Stop(0.0f, UTChar->CurrentTaunt);
			}
		}

		if (UTPC->UTPlayerState)
		{
			UTPC->UTPlayerState->EmoteReplicationInfo.EmoteCount = 0;
		}
		
		UTPC->SetEmoteSpeed(1.0f);
		UTPC->FlushPressedKeys();

		UTPC->SetViewTarget(UTPC->GetPawn());
	}
}

void AUTLineUpHelper::PerformLineUp()
{
	bIsActive = true;

	CalculateAllLineUpData();

	if (GetNetMode() == NM_Standalone)
	{
		SpawnLineUp();
	}
	else
	{
		//Wait for line up data to be replicated to clients before we spawn everything in
		static const float ReplicationDelayTime = 0.5f;
		GetWorld()->GetTimerManager().SetTimer(ReplicationDelayHandle, FTimerDelegate::CreateUObject(this, &AUTLineUpHelper::SpawnLineUp), ReplicationDelayTime, false);
	}
}

void AUTLineUpHelper::CalculateAllLineUpData()
{
	CalculateLineUpSlots();
	SetLineUpWeapons();
}

void AUTLineUpHelper::SpawnLineUp()
{
	AUTGameMode* UTGM = Cast<AUTGameMode>(GetWorld()->GetAuthGameMode());
	if (UTGM)
	{
		UTGM->RemoveAllPawns();
	}

	SpawnCharactersToSlots();
	SetupCharactersForLineUp();
	FlagFixUp();

	NotifyClientsOfLineUp();

	//We normally rely on On_RepCheckForIntro, but if we are a standalone, OnRep will not fire, so kick off intro here
	if ((GetNetMode() == NM_Standalone) && (ActiveType == LineUpTypes::Intro))
	{
		HandleIntroClientAnimations();
	}

	//Notify Zone that line up is starting
	if ((ActiveType != LineUpTypes::Intro) && UTGM->UTGameState && UTGM->UTGameState->GetAppropriateSpawnList(ActiveType))
	{
		UTGM->UTGameState->GetAppropriateSpawnList(ActiveType)->OnLineUpStart(ActiveType);
	}
}

void AUTLineUpHelper::SpawnCharactersToSlots()
{
	bIsPlacingPlayers = true;
	AUTGameMode* UTGM = Cast<AUTGameMode>(GetWorld()->GetAuthGameMode());
	if (UTGM)
	{
		//Go through all controllers and spawn/respawn pawns
		for (int SlotIndex = 0; SlotIndex < LineUpSlots.Num(); ++SlotIndex)
		{
			SpawnCharacterFromLineUpSlot(LineUpSlots[SlotIndex]);

			if (LineUpSlots[SlotIndex].ControllerInSpot)
			{
				AUTPlayerState* UTPS = Cast<AUTPlayerState>(LineUpSlots[SlotIndex].ControllerInSpot->PlayerState);
				if (UTPS)
				{
					UTPS->LineUpLocation = SlotIndex;
				}
			}
		}
	}
	bIsPlacingPlayers = false;
}

AUTCharacter* AUTLineUpHelper::SpawnCharacterFromLineUpSlot(FLineUpSlot& Slot)
{
	if (Slot.ControllerInSpot != nullptr)
	{
		AUTCharacter* UTChar = Cast<AUTCharacter>(Slot.ControllerInSpot->GetPawn());
		if (Slot.ControllerInSpot->GetPawn())
		{
			Slot.ControllerInSpot->UnPossess();
		}

		AUTGameMode* UTGM = Cast<AUTGameMode>(GetWorld()->GetAuthGameMode());
		if (UTGM)
		{
			UTGM->RestartPlayer(Slot.ControllerInSpot);
		}

		if (Slot.ControllerInSpot->GetPawn())
		{
			UTChar = Cast<AUTCharacter>(Slot.ControllerInSpot->GetPawn());
		}

		if (UTChar && !UTChar->IsDead())
		{
			UTChar->bAlwaysRelevant = true;
			PlayerPreviewCharacters.Add(UTChar);
			MoveCharacterToLineUpSlot(UTChar, Slot);
			SpawnPlayerWeapon(UTChar);
		}

		return UTChar;
	}

	return nullptr;
}

void AUTLineUpHelper::MoveCharacterToLineUpSlot(AUTCharacter* UTChar, FLineUpSlot& Slot)
{
	if (UTChar && !UTChar->IsDead() && !UTChar->IsPendingKill() && UTChar->Controller)
	{
		UTChar->TeleportTo(Slot.SpotLocation.GetTranslation(), Slot.SpotLocation.GetRotation().Rotator(), false, true);
		UTChar->Controller->SetControlRotation(Slot.SpotLocation.GetRotation().Rotator());
		UTChar->Controller->ClientSetRotation(Slot.SpotLocation.GetRotation().Rotator());
		UTChar->DeactivateSpawnProtection();

		Slot.CharacterInSpot = UTChar;
	}
}

void AUTLineUpHelper::IntroSwapMeshComponentLocations(AUTCharacter* UTChar1, AUTCharacter* UTChar2)
{
	if (UTChar1 && UTChar2)
	{
		USkeletalMeshComponent* UTChar1Mesh = UTChar1->GetMesh();
		USkeletalMeshComponent* UTChar2Mesh = UTChar2->GetMesh();
		if (UTChar1Mesh && UTChar2Mesh)
		{
			FVector Char1DestLocation = UTChar2->GetTransform().GetLocation() + UTChar2Mesh->GetRelativeTransform().GetLocation();
			FVector Char2DestLocation = UTChar1->GetTransform().GetLocation() + UTChar1Mesh->GetRelativeTransform().GetLocation();
			UTChar1Mesh->SetWorldLocation(Char1DestLocation);
			UTChar2Mesh->SetWorldLocation(Char2DestLocation);
		}
	}
}

void AUTLineUpHelper::SetLineUpWeapons()
{
	//Setup Line-Up weapon to be what is currently equipped
	for (FLineUpSlot& Slot : LineUpSlots)
	{
		if (Slot.ControllerInSpot != nullptr)
		{
			AUTPlayerState* UTPS = Cast<AUTPlayerState>(Slot.ControllerInSpot->PlayerState);
			if (UTPS)
			{
				UTPS->LineUpWeapon = UTPS->LineUpWeapon = (UTPS->FavoriteWeapon != NULL) ? UTPS->FavoriteWeapon : NULL;

				//Either we didn't have an existing favorite weapon, or its not valid in this map
				if (UTPS->LineUpWeapon == NULL || (!MapWeaponTypeList.Contains(UTPS->LineUpWeapon)))
				{
					UTPS->LineUpWeapon = (Slot.CharacterInSpot) ? Slot.CharacterInSpot->GetWeaponClass() : nullptr;
				}
			}
		}
	}
}

void AUTLineUpHelper::NotifyClientsOfLineUp()
{
	for (FConstControllerIterator Iterator = GetWorld()->GetControllerIterator(); Iterator; ++Iterator)
	{
		AUTPlayerController* UTPC = Cast<AUTPlayerController>(*Iterator);
		if (UTPC)
		{
			UTPC->ClientPrepareForLineUp();
		}
	}
}

void AUTLineUpHelper::SetupCharactersForLineUp()
{
	for (FConstPawnIterator It = GetWorld()->GetPawnIterator(); It; ++It)
	{
		if (It->IsValid() && !Cast<ASpectatorPawn>(It->Get()))
		{
			// freeze all Pawns on server
			It->Get()->TurnOff();

			AUTCharacter* UTChar = Cast<AUTCharacter>(*It);
			if (UTChar)
			{
				// Setup custom animations
				AUTLineUpHelper::ApplyCharacterAnimsForLineUp(UTChar);
			}
		}
	}
}

void AUTLineUpHelper::FlagFixUp()
{
	if (GetWorld() && GetWorld()->GetAuthGameMode())
	{
		AUTGameMode* UTGM = Cast<AUTGameMode>(GetWorld()->GetAuthGameMode());
		if (UTGM)
		{
			AUTFlag* OffenseFlag = nullptr;
			AUTFlagRunGameState* UTFRGS = UTGM->GetGameState<AUTFlagRunGameState>();
			if (UTFRGS)
			{
				OffenseFlag = UTFRGS->GetOffenseFlag();
				if (OffenseFlag)
				{
					AController* FlagController = nullptr;
					if (OffenseFlag->Holder)
					{
						FlagController = Cast<AController>(OffenseFlag->Holder->GetOwner());
					}
					else if (OffenseFlag->LastHolder)
					{
						FlagController = Cast<AController>(OffenseFlag->LastHolder->GetOwner());
					}

					if (FlagController && FlagController->GetPawn())
					{
						OffenseFlag->SetHolder(Cast<AUTCharacter>(FlagController->GetPawn()));
					}
					else if (ActiveType != LineUpTypes::Intro)
					{
						OffenseFlag->Destroy();
					}
				}
			}
		}
	}
}

void AUTLineUpHelper::SpawnPlayerWeapon(AUTCharacter* UTChar)
{
	if (UTChar)
	{
		AUTPlayerState* UTPS = Cast<AUTPlayerState>(UTChar->PlayerState);
		TSubclassOf<AUTWeapon> WeaponClass = NULL;

		if (UTPS)
		{
			WeaponClass = UTPS->LineUpWeapon ? UTPS->LineUpWeapon->GetDefaultObject<AUTWeapon>()->GetClass() : NULL;
		}

		//If we already have a weapon attachment, keep that
		if (!WeaponClass)
		{
			//Try and pick a random weapon available on the map for pickup
			if (MapWeaponTypeList.Num() > 0)
			{
				int32 WeaponIndex = FMath::RandHelper(MapWeaponTypeList.Num());

				if (MapWeaponTypeList[WeaponIndex] != NULL)
				{
					WeaponClass = MapWeaponTypeList[WeaponIndex];
				}
			}
		}
		
		//Remove all inventory so that when we add this weapon in, it is equipped.
		UTChar->DiscardAllInventory();
		
		FActorSpawnParameters WeaponSpawnParams;
		WeaponSpawnParams.Instigator = UTChar;
		WeaponSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		WeaponSpawnParams.bNoFail = true;

		AUTWeapon* PreviewWeapon = GetWorld()->SpawnActor<AUTWeapon>(WeaponClass, FVector(0, 0, 0), FRotator(0, 0, 0), WeaponSpawnParams);
		if (PreviewWeapon)
		{
			PreviewWeapons.Add(PreviewWeapon);
			
			PreviewWeapon->bAlwaysRelevant = true;
			PreviewWeapon->SetReplicates(true);
			UTChar->AddInventory(PreviewWeapon, true);
			
			//Bots will not auto-switch to new weapon
			AUTBot* BotController = Cast<AUTBot>(UTChar->Controller);
			if (BotController)
			{
				BotController->SwitchToBestWeapon();
			}
		}
	}
}

void AUTLineUpHelper::BuildMapWeaponList()
{
	//All weapon spawning is on the server, so Clients don't need a MapWeaponList
	if (GetNetMode() != NM_Client)
	{
		for (FActorIterator It(GetWorld()); It; ++It)
		{
			//Weapon Pickups
			AUTPickupWeapon* Pickup = Cast<AUTPickupWeapon>(*It);
			if (Pickup && (Pickup->WeaponType != NULL))
			{
				MapWeaponTypeList.AddUnique(Pickup->WeaponType);
			}

			//Supply Chest Weapons
			AUTSupplyChest* UTSupplyChest = Cast<AUTSupplyChest>(*It);
			if (UTSupplyChest && UTSupplyChest->bIsActive && (UTSupplyChest->Weapons.Num() > 0))
			{
				for (TSubclassOf<AUTWeapon>& Weapon : UTSupplyChest->Weapons)
				{
					if (Weapon != NULL)
					{
						MapWeaponTypeList.AddUnique(Weapon);
					}
				}
			}
		}

		//Default Inventory Weapons
		AUTGameMode* GameMode = GetWorld()->GetAuthGameMode<AUTGameMode>();
		if (GameMode)
		{
			//From Character BP
			AUTCharacter* UTChar = Cast<AUTCharacter>(GameMode->DefaultPawnClass->GetDefaultObject());
			if (UTChar)
			{
				for (TSubclassOf<AUTInventory>& Item : UTChar->DefaultCharacterInventory)
				{
					TSubclassOf<AUTWeapon> Weapon = Item.Get();
					if (Weapon != NULL)
					{
						MapWeaponTypeList.AddUnique(Weapon);
					}
				}
			}

			//From GameMode
			for (TSubclassOf<AUTInventory>& Item : GameMode->DefaultInventory)
			{
				TSubclassOf<AUTWeapon> Weapon = Item.Get();
				if (Weapon != NULL)
				{
					MapWeaponTypeList.AddUnique(Weapon);
				}
			}
		}

		//Remove invalid weapons for line-ups
		AUTGameState* UTGS = GetWorld()->GetGameState<AUTGameState>();
		if (GameMode)
		{
			TArray<TSubclassOf<AUTWeapon>> InvalidWeaponsToRemove;
			for (TSubclassOf<AUTWeapon>& Weapon : MapWeaponTypeList)
			{
				//Remove anything mutators won't allow
				if ((GameMode->BaseMutator) && (!GameMode->BaseMutator->CheckRelevance(Weapon->GetDefaultObject<AActor>())))
				{
					InvalidWeaponsToRemove.Add(Weapon);
				}
				//Translocator
				else if (Weapon->IsChildOf(AUTWeap_Translocator::StaticClass()))
				{
					InvalidWeaponsToRemove.Add(Weapon);
				}
				//Enforcer
				else if (Weapon->IsChildOf(AUTWeap_Enforcer::StaticClass()))
				{
					InvalidWeaponsToRemove.Add(Weapon);
				}
				//Redeemer
				else if (Weapon->IsChildOf(AUTWeap_Redeemer::StaticClass()))
				{
					InvalidWeaponsToRemove.Add(Weapon);
				}
			}

			//If we are about to delete all the weapons, try and keep 1 in the list.
			//For this to happen the mutator or map must lack all weapons but the Enforcer / Translocator / Redeemer. Lets try and re-validate 1 of those to show.
			if ((InvalidWeaponsToRemove.Num() == MapWeaponTypeList.Num()) && (MapWeaponTypeList.Num() > 1))
			{
				TSubclassOf<AUTWeapon> RevalidateWeapon = nullptr;
				for (TSubclassOf<AUTWeapon>& InvalidWeapon : InvalidWeaponsToRemove)
				{
					//Recheck if the item is ok with the mutator, if so, don't remove it
					if ((GameMode->BaseMutator) && (GameMode->BaseMutator->CheckRelevance(InvalidWeapon->GetDefaultObject<AActor>())))
					{
						RevalidateWeapon = InvalidWeapon;
						break;
					}
				}

				if (RevalidateWeapon)
				{
					InvalidWeaponsToRemove.Remove(RevalidateWeapon);
				}
				else
				{
					UE_LOG(LogUTLineUp, Warning, TEXT("No valid weapons found for line-up!"));
				}
			}

			for (TSubclassOf<AUTWeapon>& InvalidWeapon : InvalidWeaponsToRemove)
			{
				// Make sure at least 1 weapon is always in the list
				if (MapWeaponTypeList.Num() > 1)
				{
					MapWeaponTypeList.Remove(InvalidWeapon);
				}
			}
		}
	}
}

void AUTLineUpHelper::ApplyCharacterAnimsForLineUp(AUTCharacter* UTChar)
{
	if (UTChar)
	{
		UUTCharacterMovement* UTCM = Cast<UUTCharacterMovement>(UTChar->GetMovementComponent());
		if (UTCM)
		{
			UTCM->OnLineUp();

			//Need to turn on collision so that the line-up movement mode can find the floor and reset it
			bool bOriginalCollisionSetting = UTChar->GetActorEnableCollision();
			UTChar->SetActorEnableCollision(true);

			// This movement mode is tied to LineUp specific anims.
			UTCM->SetMovementMode(MOVE_Custom, CUSTOMMOVE_LineUp);
		
			//Reset collision to whatever it was before line-up
			UTChar->SetActorEnableCollision(bOriginalCollisionSetting);
		}

		if (UTChar->GetMesh())
		{
			//Want to still update the animations and bones even though we have turned off the Pawn, so re-enable those.
			UTChar->GetMesh()->bPauseAnims = false;
			UTChar->GetMesh()->MeshComponentUpdateFlag = EMeshComponentUpdateFlag::AlwaysTickPoseAndRefreshBones;

			//Turn off local physics sim and collisions during line-ups
			UTChar->GetMesh()->SetSimulatePhysics(false);
			UTChar->GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}


void AUTLineUpHelper::SortControllers(TArray<AController*>& ControllersToSort)
{
	bool(*SortFunc)(AController&, AController&);
	SortFunc = [](AController& A, AController& B)
	{
		AUTPlayerState* PSA = Cast<AUTPlayerState>(A.PlayerState);
		AUTPlayerState* PSB = Cast<AUTPlayerState>(B.PlayerState);

		AUTFlag* AUTFlagA = nullptr;
		AUTFlag* AUTFlagB = nullptr;
		if (PSA)
		{
			AUTFlagA = Cast<AUTFlag>(PSA->CarriedObject);
		}
		if (PSB)
		{
			AUTFlagB = Cast<AUTFlag>(PSB->CarriedObject);
		}

		return !PSB || (AUTFlagA) || (PSA && (PSA->Score > PSB->Score) && !AUTFlagB);
	};
	ControllersToSort.Sort(SortFunc);
}

void AUTLineUpHelper::ServerOnPlayerChange(AUTPlayerState* PlayerChanged)
{
	if (PlayerChanged)
	{
		//If true, we need to add the old line up location to available spots
		if (PlayerChanged->LineUpLocation != INDEX_NONE)
		{
			if (LineUpSlots.IsValidIndex(PlayerChanged->LineUpLocation) && (LineUpSlots[PlayerChanged->LineUpLocation].TeamNumOfSlot != PlayerChanged->GetTeamNum()))
			{
				LineUpSlots[PlayerChanged->LineUpLocation].CharacterInSpot = nullptr;
				LineUpSlots[PlayerChanged->LineUpLocation].ControllerInSpot = nullptr;
				UnusedLineUpSlots.Insert(LineUpSlots[PlayerChanged->LineUpLocation], 0);
				PlayerChanged->LineUpLocation = INDEX_NONE;
			}
		}

		TArray<AUTPlayerState*> NewPlayerStates;
		for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
		{
			//Skip demo recorder
			if (Cast<AUTDemoRecSpectator>(*It) != nullptr)
			{
				continue;
			}

			AUTPlayerState* UTPS = Cast<AUTPlayerState>((*It)->PlayerState);
			if (UTPS && (UTPS->LineUpLocation == INDEX_NONE))
			{
				for (FLineUpSlot& Slot : UnusedLineUpSlots)
				{
					if (Slot.TeamNumOfSlot == UTPS->GetTeamNum())
					{
						Slot.ControllerInSpot = (It->Get());
						
						bIsPlacingPlayers = true;
						AUTCharacter* NewUTChar = SpawnCharacterFromLineUpSlot(Slot);
						if (NewUTChar)
						{
							NewPlayerStates.Add(UTPS);

							NewUTChar->TurnOff();
							AUTLineUpHelper::ApplyCharacterAnimsForLineUp(NewUTChar);

							if (LineUpSlots.Contains(Slot))
							{
								const int LineUpSlotLocation = LineUpSlots.Find(Slot);
								UTPS->LineUpLocation = LineUpSlotLocation;
							}
							else
							{
								const int LineUpSlotLocation = LineUpSlots.Add(Slot);
								UTPS->LineUpLocation = LineUpSlotLocation; 
							}

							if (LineUpSlots.IsValidIndex(UTPS->LineUpLocation))
							{
								LineUpSlots[UTPS->LineUpLocation].CharacterInSpot = NewUTChar;
								LineUpSlots[UTPS->LineUpLocation].ControllerInSpot = It->Get();
							}

							UnusedLineUpSlots.RemoveSingle(Slot);

							bIsPlacingPlayers = false;
							break;
						}
						bIsPlacingPlayers = false;
					}
				}
			}
		}

		for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
		{
			AUTPlayerController* UTPC = Cast<AUTPlayerController>(It->Get());
			if (UTPC)
			{
				for (AUTPlayerState* NewPS : NewPlayerStates)
				{
					UTPC->ClientLineUpIntroPlayerChange(NewPS);
				}
			}
		}

		FlagFixUp();
	}
}

void AUTLineUpHelper::ClientLineUpIntroPlayerChange(AUTPlayerState* PlayerChanged)
{
	if (PlayerChanged && PlayerChanged->GetUTCharacter())
	{
		PlayIntroClientAnimationOnCharacter(PlayerChanged->GetUTCharacter(), false);
	}
	else
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &AUTLineUpHelper::ClientLineUpIntroPlayerChange, PlayerChanged));
	}
}

bool AUTLineUpHelper::CanInitiateGroupTaunt(AUTPlayerState* PlayerToCheck)
{
	AUTGameState* UTGS = Cast<AUTGameState>(GetWorld()->GetGameState());
	return (UTGS &&
		    PlayerToCheck &&
			(ActiveType != LineUpTypes::Invalid) &&
			(ActiveType != LineUpTypes::None) &&
			(ActiveType != LineUpTypes::Intro) && //no group taunts during intro line ups
			(PlayerToCheck == UTGS->LeadLineUpPlayer));
}

void AUTLineUpHelper::DestroySpawnedClones()
{
	if (PlayerPreviewCharacters.Num() > 0)
	{
		for (int index = 0; index < PlayerPreviewCharacters.Num(); ++index)
		{
			if (PlayerPreviewCharacters[index])
			{
				PlayerPreviewCharacters[index]->Destroy();
			}
		}
		PlayerPreviewCharacters.Empty();
	}

	if (PreviewWeapons.Num() > 0)
	{
		for (int index = 0; index < PreviewWeapons.Num(); ++index)
		{
			if (PreviewWeapons[index])
			{
				PreviewWeapons[index]->Destroy();
			}
		}
		PreviewWeapons.Empty();
	}
}

void AUTLineUpHelper::OnRep_CheckForClientIntro()
{
	//Check if we have all the needed information transmitted to us
	if ((ActiveType == LineUpTypes::Intro) && IsLineupDataReplicated())
	{
		HandleIntroClientAnimations();
	}
}

bool AUTLineUpHelper::IsLineupDataReplicated()
{
	return ((NumControllersUsedInLineUp > 0) && (LineUpSlots.Num() == NumControllersUsedInLineUp));
}

void AUTLineUpHelper::HandleIntroClientAnimations()
{
	if (GetWorld())
	{
		AUTGameState* UTGS = Cast<AUTGameState>(GetWorld()->GetGameState());
		if (UTGS)
		{
			//Make sure we have all the pawns before we start the intro
			if (IntroCheckForPawnReplicationToComplete())
			{
				AUTLineUpZone* IntroZone = UTGS->GetAppropriateSpawnList(LineUpTypes::Intro);
				if (IntroZone)
				{
					IntroZone->OnLineUpStart(LineUpTypes::Intro);

					IntroBreakSlotsIntoTeams();
					IntroSetFirstSlotToLocalPlayer();
					IntroBuildStaggeredSpawnTimerList();

					// Setup first delayed spawn character
					if (Intro_TimeDelaysOnAnims.Num() > 0)
					{
						GetWorld()->GetTimerManager().SetTimer(Intro_NextClientSpawnHandle, FTimerDelegate::CreateUObject(this, &AUTLineUpHelper::IntroSpawnDelayedCharacter), Intro_TimeDelaysOnAnims[0], false);
					}

					//Set timer for weapon ready
					GetWorld()->GetTimerManager().SetTimer(Intro_ClientSwitchToWeaponFaceoffHandle, FTimerDelegate::CreateUObject(this, &AUTLineUpHelper::IntroTransitionToWeaponReadyAnims), IntroZone->TimeToReadyWeaponStance, false);
				}
			}
			else
			{
				//We don't have pawns yet. Wait for them to replicate before starting the intro
				GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &AUTLineUpHelper::HandleIntroClientAnimations));
			}
		}
	}
}

bool AUTLineUpHelper::IntroCheckForPawnReplicationToComplete()
{
	bool bHaveAllPawns = true;
	
	for (FLineUpSlot& LineUpSlot : LineUpSlots)
	{
		if (!LineUpSlot.CharacterInSpot)
		{
			bHaveAllPawns = false;
			break;
		}
	}

	return bHaveAllPawns;
}

void AUTLineUpHelper::IntroBreakSlotsIntoTeams()
{
	Intro_OtherTeamLineUpSlots.Empty();
	Intro_MyTeamLineUpSlots.Empty();

	//Move local pawn to front of my team's list
	UUTLocalPlayer* LocalPlayer = Cast<UUTLocalPlayer>(GetWorld()->GetFirstLocalPlayerFromController());
	if (LocalPlayer)
	{
		AUTPlayerController* UTPC = Cast<AUTPlayerController>(LocalPlayer->PlayerController);
		if (UTPC)
		{
			int LocalTeamNum = UTPC->GetTeamNum();
			
			for (FLineUpSlot& LineUpSlot : LineUpSlots)
			{
				if (LocalTeamNum != LineUpSlot.TeamNumOfSlot)
				{
					Intro_OtherTeamLineUpSlots.Add(&LineUpSlot);
				}
				else
				{
					Intro_MyTeamLineUpSlots.Add(&LineUpSlot);
				}
			}
		}
	}
}

void AUTLineUpHelper::IntroSetFirstSlotToLocalPlayer()
{
	//Move local pawn to front of my team's list
	UUTLocalPlayer* LocalPlayer = Cast<UUTLocalPlayer>(GetWorld()->GetFirstLocalPlayerFromController());
	if (LocalPlayer)
	{
		AUTPlayerController* UTPC = Cast<AUTPlayerController>(LocalPlayer->PlayerController);
		if (UTPC)
		{
			if ((Intro_MyTeamLineUpSlots.Num() > 0) && (Intro_MyTeamLineUpSlots[0]->ControllerInSpot != UTPC))
			{
				FLineUpSlot* CurrentLocalPlayerSlot = nullptr;
				for (FLineUpSlot* LineUpSlot : Intro_MyTeamLineUpSlots)
				{
					if (LineUpSlot->ControllerInSpot == UTPC)
					{
						CurrentLocalPlayerSlot = LineUpSlot;
						LineUpSlot->ControllerInSpot = Intro_MyTeamLineUpSlots[0]->ControllerInSpot;
						LineUpSlot->CharacterInSpot = Intro_MyTeamLineUpSlots[0]->CharacterInSpot;
						break;
					}
				}

				if (CurrentLocalPlayerSlot)
				{
					Intro_MyTeamLineUpSlots[0]->ControllerInSpot = UTPC;
					Intro_MyTeamLineUpSlots[0]->CharacterInSpot = Cast<AUTCharacter>(UTPC->GetPawn());
					IntroSwapMeshComponentLocations(Intro_MyTeamLineUpSlots[0]->CharacterInSpot, CurrentLocalPlayerSlot->CharacterInSpot);

					//Swap LineUpLocations
					AUTPlayerState* LocalUTPS = Intro_MyTeamLineUpSlots[0]->CharacterInSpot ? Cast<AUTPlayerState>(Intro_MyTeamLineUpSlots[0]->CharacterInSpot->PlayerState) : nullptr;
					AUTPlayerState* OtherUTPS = CurrentLocalPlayerSlot->CharacterInSpot ? Cast<AUTPlayerState>(CurrentLocalPlayerSlot->CharacterInSpot->PlayerState) : nullptr;
					if (LocalUTPS && OtherUTPS)
					{
						int LocalLineUpLoc = LocalUTPS->LineUpLocation;
						LocalUTPS->LineUpLocation = OtherUTPS->LineUpLocation;
						OtherUTPS->LineUpLocation = LocalLineUpLoc;
					}
				}
			}
		}
	}
}

void AUTLineUpHelper::IntroBuildStaggeredSpawnTimerList()
{
	if (Intro_TimeDelaysOnAnims.Num() != LineUpSlots.Num())
	{
		Intro_TimeDelaysOnAnims.Empty();

		AUTGameState* UTGS = GetWorld() ? Cast<AUTGameState>(GetWorld()->GetGameState()) : nullptr;
		if (UTGS)
		{
			AUTLineUpZone* SpawnZone = UTGS->GetAppropriateSpawnList(LineUpTypes::Intro);
			if (SpawnZone)
			{
				//First spawn is always the minimum time and everything else is random, so add that
				Intro_TimeDelaysOnAnims.Add(SpawnZone->MinIntroSpawnTime);

				for (int LineUpIndex = 1; LineUpIndex < LineUpSlots.Num(); ++LineUpIndex)
				{
					Intro_TimeDelaysOnAnims.Add(FMath::RandRange(SpawnZone->MinIntroSpawnTime, SpawnZone->MaxIntroSpawnTime));
				}

				Intro_TimeDelaysOnAnims.Sort();
			}
		}
	}
}

void AUTLineUpHelper::PlayIntroClientAnimationOnCharacter(AUTCharacter* UTChar, bool bShouldPlayFullIntro)
{
	//Notify Zone that we are playing an intro
	AUTGameState* UTGS = GetWorld() ? Cast<AUTGameState>(GetWorld()->GetGameState()) : nullptr;
	if (UTGS && UTGS->GetAppropriateSpawnList(ActiveType))
	{
		UTGS->GetAppropriateSpawnList(ActiveType)->OnPlayIntroAnimationOnCharacter(UTChar);
	}

	AUTPlayerState* UTPS = UTChar ? Cast<AUTPlayerState>(UTChar->PlayerState) : nullptr;
	if (UTPS)
	{
		UTPS->bHasPlayedLineUpIntro = true;

		//If we don't have an intro class selected, choose a random valid one
		if (UTPS->IntroClass == NULL)
		{
			if (UTGS)
			{
				AUTLineUpZone* IntroZone = UTGS->GetAppropriateSpawnList(LineUpTypes::Intro);
				if (IntroZone && (IntroZone->DefaultIntros.Num() > 0))
				{
					int randomIntroIndex = FMath::RandHelper(IntroZone->DefaultIntros.Num());
					UTPS->IntroClass = IntroZone->DefaultIntros[randomIntroIndex];
				}
			}
		}

		if (UTPS->IntroClass)
		{
			AUTIntro* IntroToPlay = UTPS->IntroClass->GetDefaultObject<AUTIntro>();
			if (IntroToPlay)
			{
				UAnimInstance* AnimInstance = UTChar->GetMesh()->GetAnimInstance();
				UAnimMontage* SpawnMontage = IntroToPlay->IntroMontage;
				FName& SectionName = bShouldPlayFullIntro ? IntroToPlay->SameTeamStartingSection : IntroToPlay->EnemyTeamStartingSection;
				if (AnimInstance && SpawnMontage)
				{
					AnimInstance->Montage_Play(SpawnMontage);

					if (!SectionName.IsNone())
					{
						AnimInstance->Montage_JumpToSection(SectionName, SpawnMontage);
					}
				}
			}
		}
	}
}

FLineUpSlot* AUTLineUpHelper::Intro_GetRandomUnSpawnedLineUpSlot()
{
	FLineUpSlot* FoundSlot = nullptr;

	if (LineUpSlots.Num() > 0)
	{
		int StartingIndex = FMath::RandHelper(LineUpSlots.Num());
	
		//Search from random point forward
		for (int SlotIndex = StartingIndex; SlotIndex < LineUpSlots.Num(); ++SlotIndex)
		{
			AUTPlayerState* UTPS = (LineUpSlots[SlotIndex].CharacterInSpot) ? Cast<AUTPlayerState>(LineUpSlots[SlotIndex].CharacterInSpot->PlayerState) : nullptr;
			if (UTPS)
			{
				if (!UTPS->bHasPlayedLineUpIntro)
				{
					FoundSlot = &LineUpSlots[SlotIndex];
					break;
				}
			}
		}

		//Search from beginning to random point
		if (nullptr == FoundSlot)
		{
			for (int SlotIndex = 0; SlotIndex < StartingIndex; ++SlotIndex)
			{
				AUTPlayerState* UTPS = (LineUpSlots[SlotIndex].CharacterInSpot) ? Cast<AUTPlayerState>(LineUpSlots[SlotIndex].CharacterInSpot->PlayerState) : nullptr;
				if (UTPS)
				{
					if (!UTPS->bHasPlayedLineUpIntro)
					{
						FoundSlot =& LineUpSlots[SlotIndex];
						break;
					}
				}
			}
		}
	}

	return FoundSlot;
}

void AUTLineUpHelper::IntroSpawnDelayedCharacter()
{
	if (LineUpSlots.IsValidIndex(Intro_TotalSpawnedPlayers))
	{
		UUTLocalPlayer* LocalPlayer = Cast<UUTLocalPlayer>(GetWorld()->GetFirstLocalPlayerFromController());
		if (LocalPlayer)
		{
			AUTPlayerController* UTPC = Cast<AUTPlayerController>(LocalPlayer->PlayerController);
			if (UTPC)
			{
				int LocalTeamNum = UTPC->GetTeamNum();

				//if this is the first person spawning, and we are spawning from my team. Always choose the first player on the local team.
				const FLineUpSlot* LineUpSlot = ((Intro_TotalSpawnedPlayers > 0) || (Intro_MyTeamLineUpSlots.Num() == 0)) ? Intro_GetRandomUnSpawnedLineUpSlot() : Intro_MyTeamLineUpSlots[0];
				if (LineUpSlot && LineUpSlot->CharacterInSpot)
				{
					const bool bIsOnSameTeamAsLocalPlayer = (LineUpSlot->CharacterInSpot->GetTeamNum() == LocalTeamNum);
					PlayIntroClientAnimationOnCharacter(LineUpSlot->CharacterInSpot, bIsOnSameTeamAsLocalPlayer);

					++Intro_TotalSpawnedPlayers;
				}
			}
		}
	}

	if (LineUpSlots.IsValidIndex(Intro_TotalSpawnedPlayers))
	{
		const float TimeToNextSpawn = LineUpSlots.IsValidIndex(Intro_TotalSpawnedPlayers - 1) ? (Intro_TimeDelaysOnAnims[Intro_TotalSpawnedPlayers] - Intro_TimeDelaysOnAnims[Intro_TotalSpawnedPlayers - 1]) : Intro_TimeDelaysOnAnims[Intro_TotalSpawnedPlayers];
		GetWorld()->GetTimerManager().SetTimer(Intro_NextClientSpawnHandle, FTimerDelegate::CreateUObject(this, &AUTLineUpHelper::IntroSpawnDelayedCharacter), TimeToNextSpawn, false);
	}
}

void AUTLineUpHelper::IntroTransitionToWeaponReadyAnims()
{
	AUTGameState* UTGS = GetWorld() ? Cast<AUTGameState>(GetWorld()->GetGameState()) : nullptr;
	AUTLineUpZone* SpawnZone = UTGS ? UTGS->GetAppropriateSpawnList(ActiveType) : nullptr;
	if (SpawnZone)
	{
		SpawnZone->OnTransitionToWeaponReadyAnims();
	}

	for (FLineUpSlot& Slot : LineUpSlots)
	{
		if (Slot.CharacterInSpot)
		{
			AUTPlayerState* UTPS = Cast<AUTPlayerState>(Slot.CharacterInSpot->PlayerState);
			if (UTPS && (UTPS->IntroClass != NULL))
			{
				AUTIntro* IntroToPlay = UTPS->IntroClass->GetDefaultObject<AUTIntro>();
				if (IntroToPlay)
				{
					UAnimInstance* AnimInstance = Slot.CharacterInSpot->GetMesh()->GetAnimInstance();
					UAnimMontage* SpawnMontage = IntroToPlay->IntroMontage;
					FName& SectionName = IntroToPlay->WeaponReadyStartingSection;
					if (AnimInstance && SpawnMontage)
					{
						AnimInstance->Montage_JumpToSection(SectionName, SpawnMontage);
					}

					if (SpawnZone)
					{
						SpawnZone->OnPlayWeaponReadyAnimOnCharacter(Slot.CharacterInSpot);
					}
				}
			}
		}
	}
}

bool AUTLineUpHelper::IsControllerInLineup(AController* Controller)
{
	if (Controller && Controller->PlayerState)
	{
		AUTPlayerState* UTPS = Cast<AUTPlayerState>(Controller->PlayerState);
		return UTPS ? (UTPS->LineUpLocation != INDEX_NONE) : false;
	}

	return false;
}