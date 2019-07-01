#pragma once

#include "UTLineUpZoneVisualizationCharacter.h"

#include "UTLineUpZone.generated.h"

UENUM(BlueprintType)
enum class LineUpTypes : uint8
{
	Invalid,
	Intro,
	Intermission,
	PostMatch,
	None UMETA(Hidden)
};

UENUM(BlueprintType)
enum class LineUpSpawnTypes : uint8
{
	Invalid,
	FFA,
	Team1,
	Team2,
	WinningTeam,
	LosingTeam,
	None UMETA(Hidden)
};

USTRUCT()
struct FLineUpSpawn
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY(EditAnywhere)
	LineUpSpawnTypes SpawnType;

	UPROPERTY(EditAnywhere)
	FTransform Location;
};

/**This class represents a collection of spawn points to use for an In Game Intro Zone based on a particular TeamNum. Note multiple AUTInGameIntroZones might use the same TeamSpawnPointList. **/
UCLASS()
class UNREALTOURNAMENT_API AUTLineUpZone : public AActor
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Team Spawn Point List")
	LineUpTypes ZoneType;

	//**Determines if Spawn Locations should snap to the floor when being placed in the editor **/
	UPROPERTY(EditAnywhere, Category = "Team Spawn Point List")
	bool bSnapToFloor;

	/**Extra offset we add when snapping objects to the floor. Should be equal to the visualization capsule half height. **/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Team Spawn Point List")
	float SnapFloorOffset;

	/** Determines if this list is holding Team Spawn Locations or FFA Spawn Locations. True = Team Spawn Locations. False = FFA Spawn Locations.**/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Team Spawn Point List")
	bool bIsTeamSpawnList;

	/** Determines if the default spawn list should be used, or if you'd like to override the default **/
	UPROPERTY(EditInstanceOnly)
	bool bUseCustomSpawnList;

	UPROPERTY(EditInstanceOnly)
	bool bUseCustomCameraTransform;

	UPROPERTY(Instanced, EditAnywhere, Category = "Camera")
	UCameraComponent* Camera;

	/** Camera Animation to play during this line-up **/
	UPROPERTY(EditAnywhere, Category = "Camera")
	UCameraAnim* CameraAnimation;

	/** If set, certain game modes will use this reference to pick between different Line-Ups. In CTF, this should reference the flag base of the team associated with this LineUpZone.*/
	UPROPERTY(EditAnywhere, Category = "Team Spawn Point List")
	AUTGameObjective* GameObjectiveReference;

	UPROPERTY(EditInstanceOnly, meta = (EditCondition = "bUseCustomCameraTransform", MakeEditWidget = ""))
	FTransform CameraLocation;

	UPROPERTY(EditInstanceOnly, meta = (EditCondition = "bUseCustomSpawnList", MakeEditWidget = ""))
	TArray<FLineUpSpawn> SpawnLocations;

	UPROPERTY(EditDefaultsOnly, Category = "Global Defaults")
	TArray<FLineUpSpawn> TeamIntroSpawnLocations;

	UPROPERTY(EditDefaultsOnly, Category = "Global Defaults")
	TArray<FLineUpSpawn> TeamIntermissionSpawnLocations;

	UPROPERTY(EditDefaultsOnly, Category = "Global Defaults")
	TArray<FLineUpSpawn> TeamPostMatchSpawnLocations;

	UPROPERTY(EditDefaultsOnly, Category = "Global Defaults")
	TArray<FLineUpSpawn> SoloIntroSpawnLocations;

	UPROPERTY(EditDefaultsOnly, Category = "Global Defaults")
	TArray<FLineUpSpawn> SoloIntermissionSpawnLocations;

	UPROPERTY(EditDefaultsOnly, Category = "Global Defaults")
	TArray<FLineUpSpawn> SoloPostMatchSpawnLocations;

	UPROPERTY(EditDefaultsOnly, Category = "Global Defaults")
	FTransform SoloIntroCameraLocation;

	UPROPERTY(EditDefaultsOnly, Category = "Global Defaults")
	FTransform SoloIntermissionCameraLocation;
	
	UPROPERTY(EditDefaultsOnly, Category = "Global Defaults")
	FTransform SoloPostMatchCameraLocation;

	UPROPERTY(EditDefaultsOnly, Category = "Global Defaults")
	FTransform TeamIntroCameraLocation;
	
	UPROPERTY(EditDefaultsOnly, Category = "Global Defaults")
	FTransform TeamIntermissionCameraLocation;
	
	UPROPERTY(EditDefaultsOnly, Category = "Global Defaults")
	FTransform TeamPostMatchCameraLocation;

	UPROPERTY(EditDefaultsOnly, Category = "Global Defaults")
	TSubclassOf<AUTLineUpZoneVisualizationCharacter> EditorVisualizationCharacter;

	UPROPERTY(EditDefaultsOnly, Category = "Global Defaults")
	TArray<TSubclassOf<AUTIntro>> DefaultIntros;

	UPROPERTY(EditDefaultsOnly, Category = "Global Defaults")
	float MinIntroSpawnTime;

	UPROPERTY(EditDefaultsOnly, Category = "Global Defaults")
	float MaxIntroSpawnTime;

	UPROPERTY(EditDefaultsOnly, Category = "Global Defaults")
	float TimeToReadyWeaponStance;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USceneComponent* SceneRoot;

	UPROPERTY(BlueprintReadOnly, Category = "Intro Zone")
	TArray<UMeshComponent*> RenderedPlayerStates;

	UFUNCTION()
	void CallAppropriateCreate();

#if WITH_EDITORONLY_DATA
	UPROPERTY(Instanced,Transient, VisibleAnywhere, meta = (MakeEditWidget = ""))
	TArray<AUTLineUpZoneVisualizationCharacter*> MeshVisualizations;

	UPROPERTY()
	class UBillboardComponent* SpriteComponent;

#endif

	UFUNCTION()
	void UpdateSpawnLocationsWithVisualizationMove();

	UFUNCTION()
	void SnapToFloor();

	UFUNCTION()
	void InitializeMeshVisualizations();

	UFUNCTION()
	void UpdateMeshVisualizations();

	UFUNCTION()
	void DeleteAllMeshVisualizations();

	virtual void Destroyed() override;

	UFUNCTION(BlueprintNativeEvent, Category = "LineUpEvents")
	void OnLineUpStart(LineUpTypes Type);

	UFUNCTION(BlueprintNativeEvent, Category = "LineUpEvents")
	void OnLineUpEnd(LineUpTypes Type);

	UFUNCTION(BlueprintNativeEvent, Category = "LineUpEvents")
	void OnPlayIntroAnimationOnCharacter(AUTCharacter* UTChar);
	
	UFUNCTION(BlueprintNativeEvent, Category = "LineUpEvents")
	void OnPlayWeaponReadyAnimOnCharacter(AUTCharacter* UTChar);

	UFUNCTION(BlueprintNativeEvent, Category = "LineUpEvents")
	void OnTransitionToWeaponReadyAnims();


#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditMove(bool bFinished) override;
	virtual void PostRegisterAllComponents() override;

	virtual void PreEditUndo() override;
	virtual void PostEditUndo() override;

	virtual void SetIsTemporarilyHiddenInEditor(bool bIsHidden);
#endif // WITH_EDITOR

	public:
	virtual void BeginPlay() override;

public:
	void DefaultCreateForIntro();
	void DefaultCreateForIntermission();
	void DefaultCreateForEndMatch();
	void DefaultCreateForOnly1Character();

private:
	void DefaultCreate();
};