// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UTDualWeapon.generated.h"

UCLASS(Blueprintable, Abstract, NotPlaceable, Config = Game)
class UNREALTOURNAMENT_API AUTDualWeapon : public AUTWeapon
{
	GENERATED_UCLASS_BODY()

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void BecomeDual();
	virtual void DualEquipFinished();

	virtual float GetBringUpTime() override;
	virtual float GetPutDownTime() override;

	virtual TArray<UMeshComponent*> Get1PMeshes_Implementation() const
	{
		TArray<UMeshComponent*> Result = Super::Get1PMeshes_Implementation();
		Result.Add(LeftMesh);
		Result.Add(LeftOverlayMesh);
		return Result;
	}

	virtual bool StackPickup_Implementation(AUTInventory* ContainedInv) override;
	virtual void UpdateOverlays() override;
	virtual void BringUp(float OverflowTime) override;
	virtual bool PutDown() override;
	virtual void StateChanged() override;
	virtual void PlayImpactEffects_Implementation(const FVector& TargetLoc, uint8 FireMode, const FVector& SpawnLocation, const FRotator& SpawnRotation) override;
	virtual void PlayFiringEffects() override;
	virtual void UpdateWeaponHand() override;
	virtual void SetSkin(UMaterialInterface* NewSkin) override;

	virtual void PlayWeaponAnim(UAnimMontage* WeaponAnim, UAnimMontage* HandsAnim = NULL, float RateOverride = 0.0f) override;

	void UpdateWeaponRenderScaleOnLeftMesh();

	virtual void DetachFromOwner_Implementation() override;
	virtual void AttachToOwner_Implementation() override;

	virtual void GotoEquippingState(float OverflowTime) override;

	UPROPERTY(Instanced, BlueprintReadOnly, Category = "States")
	class UUTWeaponStateEquipping_DualWeapon* DualWeaponEquippingState;

	UPROPERTY(Instanced, BlueprintReadOnly, Category = "States")
	class UUTWeaponStateUnequipping_DualWeapon* DualWeaponUnequippingState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TSubclassOf<class AUTWeaponAttachment> DualWieldAttachmentType;

	/** Left hand weapon mesh */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	USkeletalMeshComponent* LeftMesh;

	UPROPERTY()
	TArray<UMaterialInstanceDynamic*> LeftMeshMIDs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	UAnimMontage* Dual_BringUpLeftHandFirstAttach;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	UAnimMontage* Dual_BringUpLeftWeaponFirstAttach;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	UAnimMontage* Dual_BringUpHand;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	UAnimMontage* Dual_PutDownHand;

	/** Unequip anim for when we have dual enforcer out */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	UAnimMontage* Dual_PutDownLeftWeapon;

	/** Unequip anim for when we have dual enforcer out */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	UAnimMontage* Dual_PutDownRightWeapon;

	/** AnimMontage to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TArray<UAnimMontage*> Dual_FireAnimationLeftHand;

	/** AnimMontage to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TArray<UAnimMontage*> Dual_FireAnimationRightHand;

	/** socket to attach weapon to hands; if None, then the hands are hidden */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName HandsAttachSocketLeft;

	/** AnimMontage to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TArray<UAnimMontage*> Dual_FireAnimationLeftWeapon;

	/** AnimMontage to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TArray<UAnimMontage*> Dual_FireAnimationRightWeapon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "0.1"))
	TArray<float> FireIntervalDualWield;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TArray<float> SpreadDualWield;

	/** Weapon bring up time when dual wielding. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float DualBringUpTime;

	/** Weapon put down time when dual wielding. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float DualPutDownTime;

	UPROPERTY()
	bool bDualWeaponMode;

	UPROPERTY(BlueprintReadOnly, Replicated, ReplicatedUsing = BecomeDual, Category = "Weapon")
	bool bBecomeDual;

	/** Toggle when firing dual enforcers to make them alternate. */
	UPROPERTY()
	bool bFireLeftSide;

	/**Track whether the last fired shot was from the left or right gun. Used to sync firing effects and impact effects **/
	UPROPERTY()
	bool bFireLeftSideImpact;

	UPROPERTY(EditAnywhere, Category = "Weapon")
	TArray<UParticleSystemComponent*> LeftMuzzleFlash;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Dual Animation")
	UAnimSequence* dual_idle;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Dual Animation")
	UAnimSequence* dual_idle_pose;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Dual Animation")
	UAnimSequence* dual_idleOffset_pose;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Dual Animation")
	UAnimSequence* dual_idleEmpty_pose;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Dual Animation")
	UAnimSequence* dual_idleAlt_offset_pose;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Dual Animation")
	UAnimSequence* dual_idle_pose_zero;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Dual Animation")
	UAnimSequence* dual_secondary_idle;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Dual Animation")
	UAnimSequence* dual_secondary_idle_into;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Dual Animation")
	UAnimSequence* dual_secondary_idle_out;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Dual Animation")
	UAnimSequence* dual_secondary_idle_pose;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Dual Animation")
	UAnimSequence* dual_secondary_idleOffset_pose;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Dual Animation")
	UAnimSequence* dual_secondary_idleAlt_offset_pose;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Dual Animation")
	UAnimSequence* dual_runForward;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Dual Animation")
	UAnimSequence* dual_runForward_L;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Dual Animation")
	UAnimSequence* dual_runForward_R;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Dual Animation")
	UAnimSequence* dual_jump;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Dual Animation")
	UAnimSequence* dual_fall;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Dual Animation")
	UAnimSequence* dual_fall_long;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Dual Animation")
	UAnimSequence* dual_land;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Dual Animation")
	UAnimSequence* dual_land_soft;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Dual Animation")
	UAnimSequence* dual_land_medium;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Dual Animation")
	UAnimSequence* dual_land_heavy;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Dual Animation")
	UAnimSequence* dual_slide;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Dual Animation")
	UAnimSequence* dual_dodgeForward;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Dual Animation")
	UAnimSequence* dual_dodgeBack;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Dual Animation")
	UAnimSequence* dual_dodgeLeft;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Dual Animation")
	UAnimSequence* dual_dodgeRight;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Dual Animation")
	UAnimSequence* dual_dodgeForward_right;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Dual Animation")
	UAnimSequence* dual_dodgeForward_left;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Dual Animation")
	UAnimSequence* dual_wallRun_L_into;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Dual Animation")
	UAnimSequence* dual_wallRun_L;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Dual Animation")
	UAnimSequence* dual_wallRun_L_out;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Dual Animation")
	UAnimSequence* dual_wallRun_R_into;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Dual Animation")
	UAnimSequence* dual_wallRun_R;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Dual Animation")
	UAnimSequence* dual_wallRun_R_out;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Dual Animation")
	UAnimSequence* dual_wallRun_L_dodge;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Dual Animation")
	UAnimSequence* dual_wallRun_R_dodge;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Dual Animation")
	class UAimOffsetBlendSpace* dual_lagAO;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Dual Animation")
	class UBlendSpace* dual_leanBS;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Dual Animation")
	UAnimSequence* dual_inspect;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Dual Animation")
	UAnimSequence* dual_accent_A;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Dual Animation")
	UAnimSequence* dual_accent_B;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Dual Animation")
	UAnimSequence* dual_fidget_A;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Dual Animation")
	UAnimSequence* dual_fidget_B;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Dual Animation")
	UAnimSequence* dual_fidget_C;

protected:
	UPROPERTY()
	USkeletalMeshComponent* LeftOverlayMesh;

	virtual void AttachLeftMesh();
};
