// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTRadialMenu_DropInventory.h"
#include "UTArmor.h"
#include "UTJumpBoots.h"
#include "UTPlaceablePowerup.h"
#include "UTTimedPowerup.h"

UUTRadialMenu_DropInventory::UUTRadialMenu_DropInventory(const class FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UTexture2D> MenuAtlas(TEXT("Texture2D'/Game/RestrictedAssets/UI/HUDAtlas04.HUDAtlas04'"));
	InnerCircleTemplate.QuickSet(MenuAtlas.Object, FTextureUVs(0.0f, 206.0f, 236.0f, 236.0f));
	SegmentTemplate.QuickSet(MenuAtlas.Object, FTextureUVs(275.0f, 200.0f, 189.0f, 113.0f));
	HighlightedSegmentTemplate.QuickSet(MenuAtlas.Object, FTextureUVs(275.0f, 319.0f, 189.0f, 113.0f));

	static ConstructorHelpers::FObjectFinder<UTexture2D> WeaponAtlas(TEXT("Texture2D'/Game/RestrictedAssets/UI/WeaponAtlas01.WeaponAtlas01'"));
	WeaponIconTemplate.Atlas = WeaponAtlas.Object;

	static ConstructorHelpers::FObjectFinder<UTexture2D> HUDAtlas(TEXT("Texture2D'/Game/RestrictedAssets/UI/HUDAtlas01.HUDAtlas01'"));
	FlagIcon.Atlas = HUDAtlas.Object;
	FlagIcon.UVs = FTextureUVs(841.0f, 86.0f, 47.0f, 44.0f);
	FlagIcon.RenderColor = FLinearColor::White;
	FlagIcon.RenderOffset = FVector2D(0.5f,0.5f);

	HealthIcon.Atlas = HUDAtlas.Object;
	HealthIcon.UVs = FTextureUVs(528.0f, 2.0f, 27.0f, 29.0f);
	HealthIcon.RenderColor = FLinearColor::White;
	HealthIcon.RenderOffset = FVector2D(0.5f,0.5f);

	ArmorIcon.Atlas = HUDAtlas.Object;
	ArmorIcon.UVs = FTextureUVs(561.0f, 3.0f, 26.0f, 27.0f);
	ArmorIcon.RenderColor = FLinearColor::White;
	ArmorIcon.RenderOffset = FVector2D(0.5f,0.5f);

	BootsIcon.Atlas = HUDAtlas.Object;
	BootsIcon.UVs = FTextureUVs(690.0f, 0.0f, 34.0f, 39.0f);
	BootsIcon.RenderColor = FLinearColor::White;
	BootsIcon.RenderOffset = FVector2D(0.5f,0.5f);

	ShieldIcon.Atlas = HUDAtlas.Object;
	ShieldIcon.UVs = FTextureUVs(730, 927.0f, 48.0f, 96.0f);
	ShieldIcon.RenderColor = FLinearColor::White;
	ShieldIcon.RenderOffset = FVector2D(0.5f,0.5f);

	PowerupIcon.Atlas = HUDAtlas.Object;
	PowerupIcon.UVs = FTextureUVs(561.0f, 3.0f, 26.0f, 27.0f);
	PowerupIcon.RenderColor = FLinearColor::White;
	PowerupIcon.RenderOffset = FVector2D(0.5f,0.5f);

	Powerups.AddZeroed(3);
}

void UUTRadialMenu_DropInventory::InitializeWidget(AUTHUD* Hud)
{
	Super::InitializeWidget(Hud);
	AutoLayout(8);
}

void UUTRadialMenu_DropInventory::DrawMenu(FVector2D ScreenCenter, float RenderDelta)
{
	SelectedPowerup = nullptr;
	if (bIsInteractive && UTHUDOwner != nullptr && UTHUDOwner->UTPlayerOwner != nullptr)
	{
		FVector2D CenterPoint = FVector2D(0.0f, -250.0f);
		AUTCharacter* UTCharacter = UTHUDOwner->UTPlayerOwner->GetUTCharacter();
		if (UTCharacter != nullptr)
		{
			FText Caption;
			for (int32 Segment = 0; Segment < 8; Segment++)
			{
				bool bCurrent = Segment == CurrentSegment;
				float Angle = GetMidSegmentAngle(Segment);
				FVector2D DrawScreenPosition = Rotate(CenterPoint, Angle);

				float SegmentScale = bCurrent ? 1.2f : 1.0f; 
				SegmentScale *= (Segment % 2 == 1) ? 0.7f : 1.2f;

				SegmentTemplate.RenderScale = SegmentScale;

				RenderObj_TextureAtWithRotation(SegmentTemplate, DrawScreenPosition, Angle);
				if (bCurrent)
				{
					HighlightedSegmentTemplate.RenderScale = SegmentScale;
					RenderObj_TextureAtWithRotation(HighlightedSegmentTemplate, DrawScreenPosition, Angle);
				}

				FVector2D IconRenderPosition = Rotate(FVector2D(0.0f,-250.0f), Angle);
				AUTWeapon* Weapon = UTCharacter->GetWeapon();

				// Scan the player's inventory looking for armor, shield, boots, powerups, etc.

				bool bHasBoots = false;
				int32 ArmorAmount = UTCharacter->GetArmorAmount();

				bool bHasArmor = ArmorAmount > 0;
				bool bHasFlag = UTCharacter->GetCarriedObject() != nullptr;
				bool bHasEnoughHealth = UTCharacter->Health > 50.0f;

				TArray<AUTTimedPowerup*> HeldPowerups;

				for (TInventoryIterator<> It(UTCharacter); It; ++It)
				{
					AUTJumpBoots* Boots = Cast<AUTJumpBoots>(*It);
					if (Boots != nullptr)
					{
						bHasBoots = true;
					}

					if (Cast<AUTTimedPowerup>(*It)  != nullptr )
					{
						AUTTimedPowerup* PU = Cast<AUTTimedPowerup>(*It);
						if (PU->TimeRemaining > 0)
						{
							HeldPowerups.AddUnique(PU);
						}
					}
				}

				// Remove any powerups not currently held
				for (int32 P = 0; P < Powerups.Num(); P++)
				{
					if (HeldPowerups.Find(Powerups[P]) == INDEX_NONE)
					{
						Powerups[P] = nullptr;
					}
				}

				for (int32 i=0; i < HeldPowerups.Num(); i++)
				{
					if (Powerups.Find(HeldPowerups[i]) == INDEX_NONE)
					{
						for (int32 j=0; j < Powerups.Num(); j++)
						{
							if (Powerups[j] == nullptr)
							{
								Powerups[j] = HeldPowerups[i];
								break;
							}
						}
					}
				}

				switch (Segment)
				{
					case 0:		
						// Current Weapon
						if (Weapon != nullptr && !Weapon->IsPendingKillPending())
						{
							// Draw the weapon icon

							WeaponIconTemplate.UVs = Weapon->WeaponBarSelectedUVs;
							WeaponIconTemplate.RenderOffset = FVector2D(0.5f,0.5f);

							// Draw it in black a little bigger
							WeaponIconTemplate.RenderColor = FLinearColor::Black;
							RenderObj_TextureAt(WeaponIconTemplate, IconRenderPosition.X, IconRenderPosition.Y, WeaponIconTemplate.UVs.UL * 1.55f, WeaponIconTemplate.UVs.VL * 1.05f);
			
							// Draw it colorized
							WeaponIconTemplate.RenderColor =Weapon->IconColor;
							RenderObj_TextureAt(WeaponIconTemplate, IconRenderPosition.X, IconRenderPosition.Y, WeaponIconTemplate.UVs.UL * 1.5f, WeaponIconTemplate.UVs.VL * 1.0f);

							if (bCurrent) Caption = Weapon->DisplayName;
						}

						break;

					case 1: 
						// The Jump Boots
						BootsIcon.RenderOpacity  = (bHasBoots) ? 1.0f : 0.25f;
						RenderObj_TextureAt(BootsIcon, IconRenderPosition.X, IconRenderPosition.Y, BootsIcon.UVs.UL,  BootsIcon.UVs.VL);
						if (bHasBoots && bCurrent) Caption = NSLOCTEXT("Inventory","JumpBoots","Jump Boots");

						break;

					case 2: 
/*
						// Armor
						ArmorIcon.RenderOpacity = bHasArmor ? 1.0f : 0.25f;
						RenderObj_TextureAt(ArmorIcon, IconRenderPosition.X, IconRenderPosition.Y, ArmorIcon.UVs.UL * 2.0f,  ArmorIcon.UVs.VL * 2.0f);
						if (bHasArmor && bCurrent) Caption = NSLOCTEXT("Inventory","Armor","Armor");
*/
						break;

					case 4: 
						FlagIcon.RenderOpacity = bHasFlag ? 1.0f : 0.25f;
						RenderObj_TextureAt(FlagIcon, IconRenderPosition.X, IconRenderPosition.Y, FlagIcon.UVs.UL, FlagIcon.UVs.VL);
						if (bHasFlag && bCurrent) Caption = NSLOCTEXT("Inventory","Flag","Flag");
						break;

					case 6: 
/*
						// Health
						HealthIcon.RenderOpacity = bHasEnoughHealth ? 1.0f : 0.25f;
						RenderObj_TextureAt(HealthIcon, IconRenderPosition.X, IconRenderPosition.Y, HealthIcon.UVs.UL * 2.0f,  HealthIcon.UVs.VL * 2.0f);
						if (bHasEnoughHealth && bCurrent) Caption = NSLOCTEXT("Inventory","Health","Health");
*/
						break;

					default:
						int32 PowerupIndex = Segment == 3 ? 0 : (Segment == 5 ? 1 : 2);
						if (Powerups.IsValidIndex(PowerupIndex) && Powerups[PowerupIndex] != nullptr)
						{
							FCanvasIcon* Icon = &Powerups[PowerupIndex]->HUDIcon;
							PowerupIcon.RenderOpacity = 1.0f;
							PowerupIcon.UVs = FTextureUVs(Icon->U, Icon->V, Icon->UL, Icon->VL);
							PowerupIcon.Atlas = Icon->Texture;
							RenderObj_TextureAt(PowerupIcon, IconRenderPosition.X, IconRenderPosition.Y, PowerupIcon.UVs.UL,  PowerupIcon.UVs.VL);
							if (bCurrent) 
							{
								Caption = Powerups[PowerupIndex]->DisplayName;
								SelectedPowerup = Powerups[PowerupIndex];
							}
						}
						else
						{

							PowerupIcon.Atlas = ArmorIcon.Atlas;
							PowerupIcon.RenderOpacity = 0.25f;
							PowerupIcon.UVs = FTextureUVs(407.0f, 940.0f, 72, 72);
							RenderObj_TextureAt(PowerupIcon, IconRenderPosition.X, IconRenderPosition.Y, PowerupIcon.UVs.UL,  PowerupIcon.UVs.VL);
						}

						break;
				}

			}

			if (!Caption.IsEmpty())
			{
				CaptionTemplate.Text = FText::Format(NSLOCTEXT("DropInventory","RadialCaptionFormat","Drop {0}"), Caption);
				CaptionTemplate.TextScale = 1.0f;
				RenderObj_Text(CaptionTemplate);
			}
		}
	}
}

void UUTRadialMenu_DropInventory::Execute()
{

	AUTPlayerController* UTPC = UTHUDOwner->UTPlayerOwner;
	if (UTPC == nullptr) return;
	AUTCharacter* UTCharacter = UTHUDOwner->UTPlayerOwner->GetUTCharacter();
	if (UTCharacter == nullptr) return;

	switch (CurrentSegment)
	{
		case 0 :
			// Weapon
			UTPC->ServerThrowWeapon();	
			break;

		case 1 : 
			// Boots
			UTCharacter->ServerDropBoots();
			break;

		case 2 : 
			// Armor
			//UTCharacter->ServerDropArmor();
			break;

		case 4 : 
			// Flag
			UTCharacter->DropCarriedObject();
			break;

		case 6 :
			// Health
			//UTCharacter->ServerDropHealth();
			break;

		default:
			int32 PowerupIndex = CurrentSegment == 3 ? 0 : (CurrentSegment == 5 ? 1 : 2);
			if (Powerups.IsValidIndex(PowerupIndex) && Powerups[PowerupIndex] != nullptr)
			{
				UTCharacter->ServerDropPowerup(Powerups[PowerupIndex]);
			}
	}
}

