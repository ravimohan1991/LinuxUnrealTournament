// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTLineUpZone.h"
#include "UTCharacter.h"


AUTLineUpZone::AUTLineUpZone(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bSnapToFloor = false;
	SnapFloorOffset = 95.f;

	SceneRoot = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(this, TEXT("SceneComponent"));
	RootComponent = SceneRoot;
	RootComponent->Mobility = EComponentMobility::Movable;

	ZoneType = LineUpTypes::Invalid;

	bIsTeamSpawnList = true;

	bUseCustomCameraTransform = false;
	bUseCustomSpawnList = false;

	// Create a CameraComponent	
	Camera = ObjectInitializer.CreateDefaultSubobject<UCameraComponent>(this, TEXT("LineUpCamera"));
	if (Camera)
	{
		Camera->SetupAttachment(RootComponent);

		FTransform CameraTransform;
		CameraTransform.SetTranslation(FVector(-200.f, 0.f, 30.f));
		Camera->SetFieldOfView(60.f);

		FRotator DefaultCameraRotation;
		DefaultCameraRotation.Roll = 0.f;
		DefaultCameraRotation.Pitch = 0.f;
		DefaultCameraRotation.Yaw = 0.f;

		FTransform DefaultCameraTransform;
		DefaultCameraTransform.SetTranslation(FVector(-750.f, 0.f, 47.5f));
		DefaultCameraTransform.SetRotation(DefaultCameraRotation.Quaternion());

		Camera->SetRelativeTransform(DefaultCameraTransform);
	}

#if WITH_EDITORONLY_DATA
	SpriteComponent = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));

	if (SpriteComponent)
	{
		ConstructorHelpers::FObjectFinderOptional<UTexture2D> SpriteObject(TEXT("/Game/RestrictedAssets/EditorAssets/Icons/lineup_marker.lineup_marker"));
		SpriteComponent->Sprite = SpriteObject.Get();
			
		SpriteComponent->SpriteInfo.Category = FName(TEXT("Notes"));
		SpriteComponent->SpriteInfo.DisplayName = FText(NSLOCTEXT("SpriteCategory", "Notes", "Notes"));
	
		SpriteComponent->AttachToComponent(RootComponent,FAttachmentTransformRules::KeepRelativeTransform);
		SpriteComponent->RelativeScale3D = FVector(0.5f, 0.5f, 0.5f);
		SpriteComponent->Mobility = EComponentMobility::Movable;
	}
#endif //WITH_EDITORONLY_DATA
}

void AUTLineUpZone::BeginPlay()
{
	Super::BeginPlay();

	CallAppropriateCreate();
}

void AUTLineUpZone::Destroyed()
{
	Super::Destroyed();

	DeleteAllMeshVisualizations();
}

#if WITH_EDITOR

void AUTLineUpZone::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();

	if (GetWorld() && GetWorld()->WorldType == EWorldType::Editor)
	{
		InitializeMeshVisualizations();
		CallAppropriateCreate();
	}
}

void AUTLineUpZone::PostEditMove(bool bFinished)
{
	if (bFinished)
	{
		if (bSnapToFloor)
		{
			SnapToFloor();
		}
		else
		{
			UpdateMeshVisualizations();
		}
	}

	Super::PostEditMove(bFinished);
}

void AUTLineUpZone::SetIsTemporarilyHiddenInEditor(bool bIsHidden)
{
	Super::SetIsTemporarilyHiddenInEditor(bIsHidden);

	//Delete all the mesh visualizations if we are hiding the line up in editor
	if (bIsHidden)
	{
		DeleteAllMeshVisualizations();
	}
	else
	{
		InitializeMeshVisualizations();
	}
}

void AUTLineUpZone::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.Property != NULL && PropertyChangedEvent.Property->GetFName() == FName(TEXT("RedAndWinningTeamSpawnLocations")))
	{
		UpdateMeshVisualizations();
	}

	if (PropertyChangedEvent.Property != NULL && PropertyChangedEvent.Property->GetFName() == FName(TEXT("BlueAndLosingTeamSpawnLocations")))
	{
		UpdateMeshVisualizations();
	}

	if (PropertyChangedEvent.Property != NULL && PropertyChangedEvent.Property->GetFName() == FName(TEXT("FFATeamSpawnLocations")))
	{
		UpdateMeshVisualizations();
	}

	if (PropertyChangedEvent.Property != NULL && PropertyChangedEvent.Property->GetFName() == FName(TEXT("bSnapToFloor")))
	{
		if (bSnapToFloor)
		{
			SnapToFloor();
		}
	}

	if (PropertyChangedEvent.Property != NULL && PropertyChangedEvent.Property->GetFName() == FName(TEXT("SceneRoot")))
	{
		if (RootComponent->IsVisible())
		{
			InitializeMeshVisualizations();
		}
		else
		{
			DeleteAllMeshVisualizations();
		}
	}
	
	if (PropertyChangedEvent.Property != NULL && PropertyChangedEvent.Property->GetFName() == FName(TEXT("bUseCustomCameraTransform")))
	{
		CallAppropriateCreate();
	}

	if (PropertyChangedEvent.Property != NULL && PropertyChangedEvent.Property->GetFName() == FName(TEXT("bUseCustomSpawnList")))
	{
		CallAppropriateCreate();
	}

	if (PropertyChangedEvent.Property != NULL && PropertyChangedEvent.Property->GetFName() == FName(TEXT("ZoneType")))
	{
		CallAppropriateCreate();
	}

	if (PropertyChangedEvent.Property != NULL && PropertyChangedEvent.Property->GetFName() == FName(TEXT("bIsTeamSpawnList")))
	{
		CallAppropriateCreate();

		if (bUseCustomSpawnList)
		{
			bUseCustomSpawnList = false;
			CallAppropriateCreate();
			bUseCustomSpawnList = true;
		}

		if (bUseCustomCameraTransform)
		{
			bUseCustomCameraTransform = false;
			CallAppropriateCreate();
			bUseCustomCameraTransform = true;
		}

		UpdateMeshVisualizations();
	}

	//Moved something, call appropriate create will snap it back if not using custom values
	if (PropertyChangedEvent.Property != NULL && (PropertyChangedEvent.MemberProperty->GetFName() == FName(TEXT("Location")) || (PropertyChangedEvent.MemberProperty->GetFName() == FName(TEXT("Rotation"))) || (PropertyChangedEvent.MemberProperty->GetFName() == FName(TEXT("Scale")))))
	{
		CallAppropriateCreate();
	}

	//We have changed a default property. Go through all actors in the level to reflect this change
	if (PropertyChangedEvent.Property != NULL && (PropertyChangedEvent.MemberProperty->GetFName() == FName(TEXT("TeamIntroSpawnLocations")) 
											  || (PropertyChangedEvent.MemberProperty->GetFName() == FName(TEXT("TeamIntermissionSpawnLocations"))) 
											  || (PropertyChangedEvent.MemberProperty->GetFName() == FName(TEXT("TeamPostMatchSpawnLocations")))
											  || (PropertyChangedEvent.MemberProperty->GetFName() == FName(TEXT("SoloIntroSpawnLocations")))
		                                      || (PropertyChangedEvent.MemberProperty->GetFName() == FName(TEXT("SoloIntermissionSpawnLocations")))
		                                      || (PropertyChangedEvent.MemberProperty->GetFName() == FName(TEXT("SoloPostMatchSpawnLocations"))) ))
	{
		if (GetWorld())
		{
			for (FActorIterator It(GetWorld()); It; ++It)
			{
				if (It->IsA<AUTLineUpZone>())
				{
					AUTLineUpZone* ZoneToUpdate = Cast<AUTLineUpZone>(*It);

					TSubclassOf<AUTLineUpZone> LineUpClass = LoadClass<AUTLineUpZone>(NULL, TEXT("/Game/RestrictedAssets/Blueprints/LineUpZone.LineUpZone_C"), NULL, LOAD_NoWarn | LOAD_Quiet, NULL);
					AUTLineUpZone* DefaultZone = LineUpClass ? LineUpClass->GetDefaultObject<AUTLineUpZone>() : nullptr;
					if (DefaultZone)
					{
						ZoneToUpdate->TeamIntroSpawnLocations = DefaultZone->TeamIntroSpawnLocations;
						ZoneToUpdate->TeamIntermissionSpawnLocations = DefaultZone->TeamIntermissionSpawnLocations;
						ZoneToUpdate->TeamPostMatchSpawnLocations = DefaultZone->TeamPostMatchSpawnLocations;
						ZoneToUpdate->SoloIntroSpawnLocations = DefaultZone->SoloIntroSpawnLocations;
						ZoneToUpdate->SoloIntermissionSpawnLocations = DefaultZone->SoloIntermissionSpawnLocations;
						ZoneToUpdate->SoloPostMatchSpawnLocations = DefaultZone->SoloPostMatchSpawnLocations;
					}

					ZoneToUpdate->CallAppropriateCreate();
					UpdateMeshVisualizations();
				}
			}
		}
	}
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void AUTLineUpZone::PreEditUndo()
{
	//Sometimes an edit undo will leave mesh visualizations stranded due to a change
	//Go ahead and delete them all so we don't leave any behind in the level
	DeleteAllMeshVisualizations();
	Super::PreEditUndo();
}

void AUTLineUpZone::PostEditUndo()
{
	//We deleted all mesh visualiztions in preeditundo, so now recreate them after the undo.
	InitializeMeshVisualizations();
	Super::PostEditUndo();
}
#endif

void AUTLineUpZone::UpdateMeshVisualizations()
{
#if WITH_EDITORONLY_DATA
	//If visualization counts don't match up wipe them and start over
	DeleteAllMeshVisualizations();
	InitializeMeshVisualizations();
#endif
}

void AUTLineUpZone::DeleteAllMeshVisualizations()
{
#if WITH_EDITORONLY_DATA
	for (int index = 0; index < MeshVisualizations.Num(); ++index)
	{
		if (MeshVisualizations[index])
		{
			//MeshVisualizations[index]->DestroyComponent();
			MeshVisualizations[index]->DetachRootComponentFromParent(true);
			MeshVisualizations[index]->Instigator = nullptr;
			MeshVisualizations[index]->SetOwner(nullptr);
			MeshVisualizations[index]->Destroy(true, true);
		}
	}

	MeshVisualizations.Empty();
#endif
}

void AUTLineUpZone::InitializeMeshVisualizations()
{
#if WITH_EDITORONLY_DATA
	DeleteAllMeshVisualizations();

	if ((EditorVisualizationCharacter == nullptr) || (GetWorld() == nullptr) || (!RootComponent->IsVisible()) || (IsTemporarilyHiddenInEditor()))
	{
		return;
	}

	for (int index = 0; index < SpawnLocations.Num(); ++index)
	{
		FActorSpawnParameters Params;
		Params.Owner = this;

		AUTLineUpZoneVisualizationCharacter* SpawnedActor = GetWorld()->SpawnActor<AUTLineUpZoneVisualizationCharacter>(EditorVisualizationCharacter, SpawnLocations[index].Location, Params);
		if (SpawnedActor)
		{
			MeshVisualizations.Add(SpawnedActor);
			SpawnedActor->AttachToComponent(SceneRoot, FAttachmentTransformRules::KeepRelativeTransform);
			
			if (SpawnLocations[index].SpawnType == LineUpSpawnTypes::Team1 || SpawnLocations[index].SpawnType == LineUpSpawnTypes::WinningTeam)
			{
				SpawnedActor->TeamNum = 0;
			}
			else if (SpawnLocations[index].SpawnType == LineUpSpawnTypes::Team2 || SpawnLocations[index].SpawnType == LineUpSpawnTypes::LosingTeam)
			{
				SpawnedActor->TeamNum = 1;
			}
			else
			{
				SpawnedActor->TeamNum = 255;
			}
			SpawnedActor->OnChangeTeamNum();
		}
	}
#endif
}

void AUTLineUpZone::SnapToFloor()
{
	static const FName NAME_FreeCam = FName(TEXT("FreeCam"));

	if (bSnapToFloor)
	{
		//Move base LineUpZone actor to snapped floor position
		{
			FTransform TestLocation = ActorToWorld();

			FVector Start(TestLocation.GetTranslation().X, TestLocation.GetTranslation().Y, TestLocation.GetTranslation().Z + 250.f);
			FVector End(TestLocation.GetTranslation().X, TestLocation.GetTranslation().Y, TestLocation.GetTranslation().Z - 500.0f);

			FHitResult Hit;
			GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, COLLISION_TRACE_WEAPON, FCollisionShape::MakeBox(FVector(12.f)), FCollisionQueryParams(NAME_FreeCam, false, this));
			if (Hit.bBlockingHit)
			{
				FVector NewLocation = Hit.Location;
				NewLocation.Z += SnapFloorOffset;
				SetActorLocation(NewLocation);
			}
		}

		for (int index = 0; index < SpawnLocations.Num(); ++index)
		{
			FTransform TestLocation = SpawnLocations[index].Location * ActorToWorld();
			
			FVector Start(TestLocation.GetTranslation().X, TestLocation.GetTranslation().Y, TestLocation.GetTranslation().Z + 250.0f);
			FVector End(TestLocation.GetTranslation().X, TestLocation.GetTranslation().Y, TestLocation.GetTranslation().Z - 500.0f);

			FHitResult Hit;
			GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, COLLISION_TRACE_WEAPON, FCollisionShape::MakeBox(FVector(12.f)), FCollisionQueryParams(NAME_FreeCam, false, this));
			if (Hit.bBlockingHit)
			{
				FVector NewLocation = SpawnLocations[index].Location.GetLocation();
				NewLocation.Z = (Hit.Location - ActorToWorld().GetLocation()).Z + SnapFloorOffset;
				SpawnLocations[index].Location.SetLocation(NewLocation);
			}
		}

		UpdateMeshVisualizations();
	}
}

void AUTLineUpZone::UpdateSpawnLocationsWithVisualizationMove()
{
#if WITH_EDITORONLY_DATA
	
	int RedIndex = 0;
	int BlueIndex = 0;
	int FFAIndex = 0;

	for (int MeshIndex = 0; MeshIndex < MeshVisualizations.Num(); ++MeshIndex)
	{
		AUTLineUpZoneVisualizationCharacter* Mesh = Cast<AUTLineUpZoneVisualizationCharacter>(MeshVisualizations[MeshIndex]);
		if (Mesh)
		{
			if (SpawnLocations.IsValidIndex(MeshIndex))
			{
				SpawnLocations[MeshIndex].Location = MeshVisualizations[MeshIndex]->GetRootComponent()->GetRelativeTransform();
			}

			if (Mesh->TeamNum == 0)
			{
				SpawnLocations[MeshIndex].SpawnType = LineUpSpawnTypes::Team1;
			}
			else if (Mesh->TeamNum == 1)
			{
				SpawnLocations[MeshIndex].SpawnType = LineUpSpawnTypes::Team2;
			}
			else
			{
				SpawnLocations[MeshIndex].SpawnType = LineUpSpawnTypes::FFA;
			}
		}
	}

	if (bSnapToFloor)
	{
		SnapToFloor();
	}
#endif
}

void AUTLineUpZone::DefaultCreateForOnly1Character()
{
	FRotator DefaultCameraRotation;
	DefaultCameraRotation.Roll = 0.f;
	DefaultCameraRotation.Pitch = 0.f;
	DefaultCameraRotation.Yaw = -180.f;
	
	CameraLocation.SetLocation(FVector(500.f, 0.f, 30.f));
	CameraLocation.SetRotation(DefaultCameraRotation.Quaternion());

	FTransform StartLocation;
	StartLocation.SetLocation(FVector(0, 0, 0));
	
	FLineUpSpawn StartSpawn;
	StartSpawn.Location = StartLocation;
	StartSpawn.SpawnType = LineUpSpawnTypes::FFA;

	SpawnLocations.Empty();
	SpawnLocations.Add(StartSpawn);

	SnapToFloor();
	DeleteAllMeshVisualizations();
	InitializeMeshVisualizations();

	Camera->SetFieldOfView(60.f);
	Camera->AttachToComponent(SceneRoot, FAttachmentTransformRules::KeepRelativeTransform);

	Camera->SetRelativeTransform(CameraLocation);
}

void AUTLineUpZone::CallAppropriateCreate()
{
	if (ZoneType == LineUpTypes::Intro)
	{
		DefaultCreateForIntro();
	}
	else if (ZoneType == LineUpTypes::Intermission)
	{
		DefaultCreateForIntermission();
	}
	else if (ZoneType == LineUpTypes::PostMatch)
	{
		DefaultCreateForEndMatch();
	}
}

void AUTLineUpZone::DefaultCreateForIntro()
{
	if (!bUseCustomCameraTransform)
	{
		CameraLocation = bIsTeamSpawnList ? TeamIntroCameraLocation : SoloIntroCameraLocation;
	}

	if (!bUseCustomSpawnList || (SpawnLocations.Num() == 0))
	{
		SpawnLocations = bIsTeamSpawnList ? TeamIntroSpawnLocations : SoloIntroSpawnLocations;
	}

	DefaultCreate();
}

void AUTLineUpZone::DefaultCreateForIntermission()
{
	if (!bUseCustomCameraTransform)
	{
		CameraLocation = bIsTeamSpawnList ? TeamIntermissionCameraLocation : SoloIntermissionCameraLocation;
	}

	if (!bUseCustomSpawnList || (SpawnLocations.Num() == 0))
	{
		SpawnLocations = bIsTeamSpawnList ? TeamIntermissionSpawnLocations : SoloIntermissionSpawnLocations;
	}

	DefaultCreate();
}

void AUTLineUpZone::DefaultCreateForEndMatch()
{
	if (!bUseCustomCameraTransform)
	{
		CameraLocation = bIsTeamSpawnList ? TeamPostMatchCameraLocation : SoloPostMatchCameraLocation;
	}

	if (!bUseCustomSpawnList || (SpawnLocations.Num() == 0))
	{
		SpawnLocations = bIsTeamSpawnList ? TeamPostMatchSpawnLocations : SoloPostMatchSpawnLocations;
	}

	DefaultCreate();
}

void AUTLineUpZone::DefaultCreate()
{
	SnapToFloor();
	DeleteAllMeshVisualizations();
	InitializeMeshVisualizations();

	Camera->SetFieldOfView(60.f);
	Camera->AttachToComponent(SceneRoot, FAttachmentTransformRules::KeepRelativeTransform);

	Camera->SetRelativeTransform(CameraLocation);
}

void AUTLineUpZone::OnLineUpStart_Implementation(LineUpTypes LineUpType)
{
}

void AUTLineUpZone::OnLineUpEnd_Implementation(LineUpTypes LineUpType)
{
}

void AUTLineUpZone::OnPlayIntroAnimationOnCharacter_Implementation(AUTCharacter* UTChar)
{
}

void AUTLineUpZone::OnPlayWeaponReadyAnimOnCharacter_Implementation(AUTCharacter* UTChar)
{
}

void AUTLineUpZone::OnTransitionToWeaponReadyAnims_Implementation()
{
}