// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTPickup.h"
#include "UTPickupInventory.h"
#include "UTDroppedArmor.h"
#include "UnrealNetwork.h"

AUTDroppedArmor::AUTDroppedArmor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	ArmorAmount = 0;
	InitialLifeSpan = 5.0f;

	UStaticMeshComponent* MeshComp = ObjectInitializer.CreateDefaultSubobject<UStaticMeshComponent>(this, TEXT("ArmorMesh"));

	Mesh = MeshComp;
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetupAttachment(RootComponent);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshMega(TEXT("StaticMesh'/Game/RestrictedAssets/Pickups/Pickup_Proto/Meshes/Test_armor_Mega.Test_armor_Mega'"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshLarge(TEXT("StaticMesh'/Game/RestrictedAssets/Pickups/Pickup_Proto/Meshes/Test_armor_Large.Test_armor_Large'"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshMedium(TEXT("StaticMesh'/Game/RestrictedAssets/Pickups/Pickup_Proto/Meshes/Test_armor_Medium.Test_armor_Medium'"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshSmall(TEXT("StaticMesh'/Game/RestrictedAssets/Pickups/Pickup_Proto/Meshes/Test_armor_Small.Test_armor_Small'"));

	ArmorMeshes.Add(MeshMega.Object);
	ArmorMeshes.Add(MeshLarge.Object);
	ArmorMeshes.Add(MeshMedium.Object);
	ArmorMeshes.Add(MeshSmall.Object);
}

void AUTDroppedArmor::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AUTDroppedArmor, ArmorAmount);
}


void AUTDroppedArmor::PostInitializeComponents() 
{
	Super::PostInitializeComponents();
}

void AUTDroppedArmor::SetArmorAmount(class AUTArmor* inArmorType, int32 NewAmount)
{
	ArmorAmount = NewAmount;
	ArmorType = inArmorType;
	OnArmorAmountReceived();
}

void AUTDroppedArmor::OnArmorAmountReceived()
{
	if (ArmorAmount <= 25 && ArmorMeshes.IsValidIndex(3) && ArmorMeshes[3] != nullptr) Cast<UStaticMeshComponent>(Mesh)->SetStaticMesh(ArmorMeshes[3]);
	else if (ArmorAmount <= 50 && ArmorMeshes.IsValidIndex(2) && ArmorMeshes[2] != nullptr) Cast<UStaticMeshComponent>(Mesh)->SetStaticMesh(ArmorMeshes[2]);
	else if (ArmorAmount <= 100 && ArmorMeshes.IsValidIndex(1) && ArmorMeshes[1] != nullptr) Cast<UStaticMeshComponent>(Mesh)->SetStaticMesh(ArmorMeshes[1]);
	else if (ArmorMeshes.IsValidIndex(0) && ArmorMeshes[0] != nullptr) Cast<UStaticMeshComponent>(Mesh)->SetStaticMesh(ArmorMeshes[0]);
}


void AUTDroppedArmor::GiveTo_Implementation(APawn* Target)
{
	AUTCharacter* P = Cast<AUTCharacter>(Target);
	if (P)
	{
		P->SetArmorAmount(ArmorType, FMath::Clamp<int32>(P->GetArmorAmount() + ArmorAmount, 0, 150));
	}
}

USoundBase* AUTDroppedArmor::GetPickupSound_Implementation() const
{
	return PickupSound;
}
