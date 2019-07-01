// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTProjectile.h"
#include "UTProj_Grenade_Sticky.generated.h"


UCLASS()
class UNREALTOURNAMENT_API AUTProj_Grenade_Sticky : public AUTProjectile
{
	GENERATED_UCLASS_BODY()
			
public:
	UFUNCTION(BlueprintCallable, Category = Projectile)
	uint8 GetInstigatorTeamNum();

	UPROPERTY(BlueprintReadOnly, Category = Projectile)
	class AUTWeap_GrenadeLauncher* GrenadeLauncherOwner;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Projectile)
	float LifeTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Projectile)
	float LifeTimeAfterArmed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Projectile)
	float MinimumLifeTime;

	UPROPERTY(BlueprintReadOnly, Category=Projectile)
	bool bArmed;

	UPROPERTY()
		ULightComponent* BlinkingLight;

	FTimerHandle FLifeTimeHandle;
	FTimerHandle FArmedHandle;

	virtual void BeginPlay() override;
	virtual void ShutDown() override;
	virtual void Destroyed() override;
	virtual	void Explode_Implementation(const FVector& HitLocation, const FVector& HitNormal, UPrimitiveComponent* HitComp) override;

	UFUNCTION()
	void ExplodeDueToTimeout();

	UFUNCTION(BlueprintCallable, Category = Projectile)
	void ArmGrenade();

	UFUNCTION(BlueprintImplementableEvent, Category = Projectile)
	void PlayDamagedDetonationEffects();

	UFUNCTION(BlueprintImplementableEvent, Category = Projectile)
	void PlayIdleEffects();

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;
	virtual void OnStop(const FHitResult& Hit) override;
	virtual void ProcessHit_Implementation(AActor* OtherActor, UPrimitiveComponent* OtherComp, const FVector& HitLocation, const FVector& HitNormal) override;
	virtual void Tick(float DeltaSeconds) override;

protected:

	UPROPERTY()
	AUTProjectile* SavedFakeProjectile;

};
