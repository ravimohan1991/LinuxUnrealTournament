// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTProjectile.h"
#include "UTProj_Rocket.generated.h"

UCLASS()
class UNREALTOURNAMENT_API AUTProj_Rocket : public AUTProjectile
{
	GENERATED_UCLASS_BODY()

	/** If set, rocket seeks this target. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_TargetActor, Category = RocketSeeking)
	AActor* TargetActor;

	UFUNCTION()
		void OnRep_TargetActor();

	/**The speed added to velocity in the direction of the target*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RocketSeeking)
		float AdjustmentSpeed;

	/** If true, lead tracked target. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RocketSeeking)
	bool bLeadTarget;

	/** If bLeadTarget, max distance that rocket will start leading target. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RocketSeeking)
		float MaxLeadDistance;

	/** Min distance rocket will track. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RocketSeeking)
		float MinSeekDistance;

	UPROPERTY(BlueprintReadOnly, Category = RocketSeeking)
		bool bRocketTeamSet;

	/**The texture for locking on a target*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RocketSeeking)
		UTexture2D* LockCrosshairTexture;

	/** Max distance that target will see lock indicator. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RocketSeeking)
		float MaxTargetLockIndicatorDistance;

	/** Reward announcement when kill with air rocket. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Announcement)
		TSubclassOf<class UUTRewardMessage> AirRocketRewardClass;

	UPROPERTY()
		class UMaterialInstanceDynamic* MeshMI;

	virtual void Destroyed() override;
	virtual void Tick(float DeltaTime) override;
	virtual void OnRep_Instigator() override;
	virtual void PostRenderFor(APlayerController* PC, UCanvas* Canvas, FVector CameraPosition, FVector CameraDir) override;

	virtual void DamageImpactedActor_Implementation(AActor* OtherActor, UPrimitiveComponent* OtherComp, const FVector& HitLocation, const FVector& HitNormal);

};
