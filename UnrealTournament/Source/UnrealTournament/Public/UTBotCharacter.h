// editor-creatable bot character data
// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UTBotCharacter.generated.h"

UCLASS()
class UNREALTOURNAMENT_API UUTBotCharacter : public UDataAsset
{
	GENERATED_UCLASS_BODY()
public:
	/** if set a UTProfileItem is required for this character to be available */
	UPROPERTY(EditAnywhere, AssetRegistrySearchable, Meta = (DisplayName = "Requires Online Item"))
	bool bRequiresItem;

	/** Available only if ?Proto=1. */
	UPROPERTY(EditAnywhere, AssetRegistrySearchable)
		bool bProtoBot;

	/** Modifier to base game difficulty for this bot. */
	UPROPERTY(EditAnywhere, AssetRegistrySearchable, Meta = (ClampMin = "-1.0", UIMin = "1.0", ClampMax = "1.5", UIMax = "1.5"), Category = Skill)
		float SkillAdjust;

	/** Minimum skill rating this bot will appear at. */
	UPROPERTY(EditAnywhere, AssetRegistrySearchable, Meta = (ClampMin = "0", UIMin = "0", ClampMax = "7.0", UIMax = "7.0"), Category = Skill)
		float MinimumSkill;

	/** bot personality attributes affecting behavior */
	UPROPERTY(EditAnywhere, Category = Skill)
	FBotPersonality Personality;
	
	/** character content (primary mesh/material) */
	UPROPERTY(EditAnywhere, Meta = (MetaClass = "UTCharacterContent"), Category = Cosmetics)
	FStringClassReference Character;

	/** hat to use */
	UPROPERTY(EditAnywhere, Meta = (MetaClass = "UTHat"), Category=Cosmetics)
	FStringClassReference HatType;
	/** optional hat variant ID */
	UPROPERTY(EditAnywhere)
	int32 HatVariantId;
	/** eyewear to use */
	UPROPERTY(EditAnywhere, Meta = (MetaClass = "UTEyewear"), Category = Cosmetics)
	FStringClassReference EyewearType;
	/** optional eyewear variant ID */
	UPROPERTY(EditAnywhere)
	int32 EyewearVariantId;

	/* Voice associated with this character. */
	UPROPERTY(EditAnywhere, Meta = (MetaClass = "UTCharacterVoice"), Category = Cosmetics)
	FStringClassReference CharacterVoice;

	/** If greater than 1, can multijump. */
	UPROPERTY(EditAnywhere, Category = Movement)
		int32 MaxMultiJumpCount;

	/** Vertical impulse on multijump. */
	UPROPERTY(EditAnywhere, Category = Movement)
		float MultiJumpImpulse;

	UPROPERTY(EditAnywhere, Category = Movement)
		float MaxRunSpeed;

	UPROPERTY(EditAnywhere, Category = Movement)
		float AirControl;

	/* Vertical impulse magnitude when player jumps. */
	UPROPERTY(EditAnywhere, Category = Movement)
		float JumpImpulse;

	/**  Wall run stops when falling faster than this */
	UPROPERTY(EditAnywhere, Category = Movement)
		float MaxWallRunFallZ;

	/** Gravity reduction during wall run */
	UPROPERTY(EditAnywhere, Category = Movement)
		float WallRunGravityScaling;

	/** Whether this character can dodge. */
	UPROPERTY(EditAnywhere, Category = Dodging)
		bool bCanDodge;

	/** Time after landing dodge before another can be attempted. */
	UPROPERTY(EditAnywhere, Category = Dodging)
		float DodgeResetInterval;

	/** Dodge impulse in XY plane */
	UPROPERTY(EditAnywhere, Category = Dodging)
		float DodgeImpulseHorizontal;

	/** Dodge impulse added in Z direction */
	UPROPERTY(EditAnywhere, Category = Dodging)
		float DodgeImpulseVertical;

	/** Max positive Z speed with additive Wall Dodge Vertical Impulse.  Wall Dodge will not add impulse making vertical speed higher than this amount. */
	UPROPERTY(EditAnywhere, Category = Dodging)
		float MaxAdditiveDodgeJumpSpeed;

	/** Maximum XY velocity of dodge (dodge impulse + current movement combined). */
	UPROPERTY(EditAnywhere, Category = Dodging)
		float DodgeMaxHorizontalVelocity;

	/** Wall Dodge impulse in XY plane */
	UPROPERTY(EditAnywhere, Category = Dodging)
		float WallDodgeImpulseHorizontal;

	/** Vertical impulse for first wall dodge. */
	UPROPERTY(EditAnywhere, Category = Dodging)
		float WallDodgeImpulseVertical;

	/** Vertical impulse for subsequent consecutive wall dodges. */
	UPROPERTY(EditAnywhere, Category = Dodging)
		float WallDodgeSecondImpulseVertical;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray< TSubclassOf<AUTInventory> > CardInventory;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		int32 ExtraArmor;

	/** Horizontal speed reduction on slide ending (multiplied). */
	UPROPERTY(Category = "FloorSlide", EditAnywhere, BlueprintReadWrite)
		float FloorSlideEndingSpeedFactor;

	/** Acceleration during a floor slide. */
	UPROPERTY(Category = "FloorSlide", BlueprintReadOnly)
		float FloorSlideAcceleration;

	/** Max speed during a floor slide (decelerates to this if too fast). */
	UPROPERTY(Category = "FloorSlide", BlueprintReadOnly)
		float MaxFloorSlideSpeed;

	/** Max initial speed for a floor slide. */
	UPROPERTY(Category = "FloorSlide", BlueprintReadOnly)
		float MaxInitialFloorSlideSpeed;

	/** How long floor slide lasts. */
	UPROPERTY(Category = "FloorSlide", BlueprintReadOnly)
		float FloorSlideDuration;
};
