#pragma  once

#include "UTLineUpZone.h"
#include "UTLineUpHelper.generated.h"


USTRUCT()
struct FLineUpSlot
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FTransform SpotLocation;

	UPROPERTY()
	AController* ControllerInSpot;
	
	UPROPERTY()
	AUTCharacter* CharacterInSpot;

	UPROPERTY()
	int TeamNumOfSlot;

	bool operator==(const FLineUpSlot& other) const
	{
		return ((&other == this) || ((other.SpotLocation.Equals(SpotLocation)) && (other.TeamNumOfSlot == TeamNumOfSlot)));
	}
};

UCLASS()
class UNREALTOURNAMENT_API AUTLineUpHelper : public AActor
{
	GENERATED_UCLASS_BODY()

	UFUNCTION()
	static void ApplyCharacterAnimsForLineUp(AUTCharacter* UTChar);

	UFUNCTION()
	static bool IsControllerInLineup(AController* Controller);

	UFUNCTION()
	bool CanInitiateGroupTaunt(AUTPlayerState* PlayerToCheck);

	UFUNCTION()
	void ServerOnPlayerChange(AUTPlayerState* PlayerChanged);

	UFUNCTION()
	void ClientLineUpIntroPlayerChange(AUTPlayerState* PlayerChanged);

	UFUNCTION()
	void InitializeLineUp(LineUpTypes LineUpType);

	UFUNCTION()
	void CleanUp();
	
	UPROPERTY(Replicated)
	bool bIsActive;

	UPROPERTY(Replicated, ReplicatedUsing = OnRep_CheckForClientIntro)
	LineUpTypes ActiveType;
	
	UPROPERTY()
	bool bIsPlacingPlayers;

	/*Handles all the clean up for a particular player when a line-up is ending*/
	static void CleanUpPlayerAfterLineUp(AUTPlayerController* UTPC);

	virtual void BeginPlay() override;

	UFUNCTION()
	bool IsLineupDataReplicated();

	UFUNCTION()
	bool IsActive();

protected:

	UFUNCTION()
	void CalculateAllLineUpData();

	UFUNCTION()
	void SpawnLineUp();

	UFUNCTION()
	void HandleIntroClientAnimations();

	UFUNCTION()
	void PlayIntroClientAnimationOnCharacter(AUTCharacter* UTChar, bool bShouldPlayFullIntro);

	UFUNCTION()
	void DestroySpawnedClones();

	UFUNCTION()
	void SpawnCharactersToSlots();

	UFUNCTION()
	AUTCharacter* SpawnCharacterFromLineUpSlot(FLineUpSlot& Slot);

	UFUNCTION()
	void MoveCharacterToLineUpSlot(AUTCharacter* UTChar, FLineUpSlot& Slot);

	UFUNCTION()
	void IntroSwapMeshComponentLocations(AUTCharacter* UTChar1, AUTCharacter* UTChar2);

	UFUNCTION()
	bool IntroCheckForPawnReplicationToComplete();

	UFUNCTION()
	void PerformLineUp();

	UFUNCTION()
	void SetLineUpWeapons();

	UFUNCTION()
	void SpawnPlayerWeapon(AUTCharacter* UTChar);
	
	/*Flag can be in a bad state since we recreate pawns during Line Up. This function re-assigns the flag to the correct player pawn*/
	UFUNCTION()
	void FlagFixUp();

	UFUNCTION()
	void BuildMapWeaponList();

	UFUNCTION()
	void NotifyClientsOfLineUp();
	
	UFUNCTION()
	void SetupCharactersForLineUp();

	UFUNCTION()
	void OnRep_CheckForClientIntro();

	FTimerHandle DelayedLineUpHandle;
	FTimerHandle ReplicationDelayHandle;

	/** preview actors */
	UPROPERTY()
	TArray<class AUTCharacter*> PlayerPreviewCharacters;

	/** preview weapon */
	UPROPERTY()
	TArray<class AUTWeapon*> PreviewWeapons;

	UPROPERTY()
	TArray<class UAnimationAsset*> PreviewAnimations;

	UPROPERTY()
	TArray<TSubclassOf<AUTWeapon>> MapWeaponTypeList;

	UPROPERTY(Replicated, ReplicatedUsing = OnRep_CheckForClientIntro)
	int NumControllersUsedInLineUp;

	UPROPERTY(Replicated, ReplicatedUsing = OnRep_CheckForClientIntro)
	TArray<FLineUpSlot> LineUpSlots;

	UPROPERTY()
	TArray<FLineUpSlot> UnusedLineUpSlots;

private:

	float CalculateLineUpDelay();
	void CalculateLineUpSlots();
	void StartLineUpWithDelay(float TimeDelay);
	
	void SortControllers(TArray<AController*>& ControllersToSort);

	void IntroSetFirstSlotToLocalPlayer();
	void IntroBreakSlotsIntoTeams();
	void IntroBuildStaggeredSpawnTimerList();

	void IntroSpawnDelayedCharacter();
	void IntroTransitionToWeaponReadyAnims();

	FLineUpSlot* Intro_GetRandomUnSpawnedLineUpSlot();

	/**Variables used by clients to track introduction line-ups that happen locally **/
	TArray<FLineUpSlot*> Intro_MyTeamLineUpSlots;
	TArray<FLineUpSlot*> Intro_OtherTeamLineUpSlots;
	TArray<float> Intro_TimeDelaysOnAnims;

	int Intro_TotalSpawnedPlayers;

	FTimerHandle Intro_NextClientSpawnHandle;
	FTimerHandle Intro_ClientSwitchToWeaponFaceoffHandle;
};