// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UTDroppedPickup.h"
#include "UTPickupAmmo.h"
#include "UTPickupMessage.h"

#include "UTDroppedAmmoBox.generated.h"

UCLASS()
class UNREALTOURNAMENT_API AUTDroppedAmmoBox : public AUTDroppedPickup
{
	GENERATED_BODY()
public:
	AUTDroppedAmmoBox(const FObjectInitializer& OI)
		: Super(OI)
	{
		SMComp = OI.CreateDefaultSubobject<UStaticMeshComponent>(this, FName(TEXT("SMComp")));
		SMComp->SetupAttachment(Collision);
		static ConstructorHelpers::FObjectFinder<UStaticMesh> BoxMesh(TEXT("/Game/RestrictedAssets/Proto/UT3_Pickups/Ammo/S_AmmoCrate.S_AmmoCrate"));
		SMComp->SetStaticMesh(BoxMesh.Object);
		SMComp->RelativeLocation = FVector(0.0f, 0.0f, -30.0f);
		SMComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		static ConstructorHelpers::FObjectFinder<USoundBase> PickupSoundRef(TEXT("/Game/RestrictedAssets/Proto/UT3_Pickups/Audio/Ammo/Cue/A_Pickup_Ammo_Stinger_Cue.A_Pickup_Ammo_Stinger_Cue"));
		PickupSound = PickupSoundRef.Object;
	}
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	UStaticMeshComponent* SMComp;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	USoundBase* PickupSound;

	/** ammo in the box */
	UPROPERTY(BlueprintReadWrite)
	TArray<FStoredAmmo> Ammo;

	/** if set, restore an additional percentage of all owned weapons' ammo */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float GlobalRestorePct;

	/** if set draw beacon on HUD through walls */
	UPROPERTY(EditDefaultsOnly)
	bool bDrawBeacon;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FCanvasIcon BeaconIcon;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FCanvasIcon BeaconArrow;

	virtual USoundBase* GetPickupSound_Implementation() const
	{
		return PickupSound;
	}

	virtual void BeginPlay() override
	{
		Super::BeginPlay();

		if (bDrawBeacon)
		{
			for (FLocalPlayerIterator It(GEngine, GetWorld()); It; ++It)
			{
				if (It->PlayerController != nullptr && It->PlayerController->MyHUD != nullptr)
				{
					It->PlayerController->MyHUD->AddPostRenderedActor(this);
				}
			}
		}
	}
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override
	{
		Super::EndPlay(EndPlayReason);

		for (FLocalPlayerIterator It(GEngine, GetWorld()); It; ++It)
		{
			if (It->PlayerController != nullptr && It->PlayerController->MyHUD != nullptr)
			{
				It->PlayerController->MyHUD->RemovePostRenderedActor(this);
			}
		}
	}

	virtual void PostRenderFor(APlayerController* PC, UCanvas* Canvas, FVector CameraPosition, FVector CameraDir) override
	{
		FVector Pos = Canvas->Project(GetActorLocation() + FVector(0.0f, 0.0f, Collision->GetScaledCapsuleHalfHeight()));
		if (Pos.X > 0.0f && Pos.Y > 0.0f && Pos.X < Canvas->SizeX && Pos.Y < Canvas->SizeY && Pos.Z > 0.0f)
		{
			Canvas->DrawColor = FColor::White;
			FVector2D Size(32.0f, 32.0f);
			Size *= Canvas->ClipX / 1920.0f * FMath::Clamp<float>(1.0f - (GetActorLocation() - CameraPosition).Size() / 5000.0f, 0.25f, 1.0f);
			Canvas->DrawTile(BeaconArrow.Texture, Pos.X - Size.X * 0.5f, Pos.Y - Size.Y * 0.5f, Size.X, Size.Y, BeaconArrow.U, BeaconArrow.V, BeaconArrow.UL, BeaconArrow.VL);
			Pos.Y -= Size.Y * 1.5f;
			Canvas->DrawTile(BeaconIcon.Texture, Pos.X - Size.X * 0.5f, Pos.Y - Size.Y * 0.5f, Size.X, Size.Y, BeaconIcon.U, BeaconIcon.V, BeaconIcon.UL, BeaconIcon.VL);
		}
	}

	virtual void GiveTo_Implementation(APawn* Target) override
	{
		AUTCharacter* UTC = Cast<AUTCharacter>(Target);
		if (UTC != NULL)
		{
			AUTPlayerController* PC = Cast<AUTPlayerController>(UTC->GetController());
			TArray<UClass*> AmmoClasses;
			GetDerivedClasses(AUTPickupAmmo::StaticClass(), AmmoClasses);
			TArray<FStoredAmmo> FinalAmmoAmounts = Ammo;
			if (GlobalRestorePct > 0.0f)
			{
				for (UClass* AmmoType : AmmoClasses)
				{
					AUTWeapon* Weap = UTC->FindInventoryType(AmmoType->GetDefaultObject<AUTPickupAmmo>()->Ammo.Type, true);
					if (Weap != nullptr && Weap->bWeaponStay)
					{
						bool bFound = false;
						for (FStoredAmmo& AmmoItem : FinalAmmoAmounts)
						{
							if (AmmoItem.Type == Weap->GetClass())
							{
								AmmoItem.Amount += Weap->MaxAmmo * GlobalRestorePct;
								bFound = true;
								break;
							}
						}
						if (!bFound)
						{
							int32 Index = FinalAmmoAmounts.AddZeroed();
							FinalAmmoAmounts[Index].Type = Weap->GetClass();
							FinalAmmoAmounts[Index].Amount = Weap->MaxAmmo * GlobalRestorePct;
						}
					}
				}
			}
			for (const FStoredAmmo& AmmoItem : FinalAmmoAmounts)
			{
				UTC->AddAmmo(AmmoItem);
				if (PC != NULL)
				{
					// send message per ammo type
					UClass** AmmoType = AmmoClasses.FindByPredicate([&](UClass* TestClass) { return AmmoItem.Type == TestClass->GetDefaultObject<AUTPickupAmmo>()->Ammo.Type; });
					if (AmmoType != NULL)
					{
						PC->ClientReceiveLocalizedMessage(UUTPickupMessage::StaticClass(), 0, NULL, NULL, *AmmoType);
					}
				}
			}
		}
	}
};