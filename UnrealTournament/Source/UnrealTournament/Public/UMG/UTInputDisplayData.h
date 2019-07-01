// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UTUITypes.h"
#include "InputChord.h"
#include "UTInputDisplayData.generated.h"

// Forward declares
struct FInputActionKeyMapping;
struct FUTPlayerInput_ActionMapping;

/** Art to represent a mouse button */
USTRUCT()
struct FMouseButtonArt
{
	GENERATED_BODY()

public:
	/** The input key this represents */
	UPROPERTY(VisibleAnywhere)
	FKey Key;

	/** The icon to display for this mouse button */
	UPROPERTY(EditAnywhere)
	FSlateBrush ButtonIcon;

	FMouseButtonArt()
		: Key(EKeys::Invalid)
		, ButtonIcon()
	{}

	FMouseButtonArt(const FKey& InKey)
		: Key(InKey)
		, ButtonIcon()
	{}
};

/** Art to represent a gamepad button */
USTRUCT()
struct FGamepadButtonArt
{
	GENERATED_BODY()

public:
	/** The input key these icons represent */
	UPROPERTY(VisibleAnywhere)
	FKey Key;

	/** The icon to display for this button on PS4 */
	UPROPERTY(EditAnywhere)
	FSlateBrush PS4Icon;

	/** The icon to display for this button on Xbox */
	UPROPERTY(EditAnywhere)
	FSlateBrush XBoxIcon;

	/** The icon to display for this button on Xbox */
	UPROPERTY(EditAnywhere)
	FSlateBrush NVShieldIcon;

	FGamepadButtonArt()
		: Key(EKeys::Invalid)
		, PS4Icon()
		, XBoxIcon()
		, NVShieldIcon()
	{}

	FGamepadButtonArt(const FKey& InKey)
		: Key(InKey)
		, PS4Icon()
		, XBoxIcon()
		, NVShieldIcon()
	{}
};

/** 
 * For associating basic actions with an associated gamepad action override. 
 * The vast majority of actions do not need this, but there are some cases where this is necessary.
 * This is primarily only in cases where the gamepad needs to make use of the L2 modifier button to
 * effectively double the number of possible inputs on the controller.
 */
USTRUCT()
struct FInputActionGamepadEquivalency
{
	GENERATED_BODY()

public:
	/** The "standard" action name entered in the project input settings for this action */
	UPROPERTY(EditAnywhere)
	FName ActionName;

	/** The name of the equivalent action that is used by the gamepad */
	UPROPERTY(EditAnywhere)
	FName GamepadActionNameEquivalent;

	FInputActionGamepadEquivalency()
		: ActionName(NAME_None)
		, GamepadActionNameEquivalent()
	{}
};

/** 
 *  Simple editor-only struct used to map ActionNames to Human Readable names
 */
USTRUCT()
struct FActionBindingToReadableName
{
	GENERATED_BODY()

	/** Ctor */
	FActionBindingToReadableName()
	: ActionName(NAME_None)
	{}

	/** The action binding name that is associated with the ReadableName property below */
	UPROPERTY(EditAnywhere)
	FName ActionName;

	/** The Human Readable Name that is associated with the action binding name property above */
	UPROPERTY(EditAnywhere)
	FText ReadableName;
};

USTRUCT()
struct FAxisBindingsRedirectStruct
{
	GENERATED_BODY()

	/** Ctor */
	FAxisBindingsRedirectStruct(FName _MetaAxisName = NAME_None, FName _AxisName = NAME_None, float _AxisDirection = 0.0f)
	: MetaAxisName(_MetaAxisName)
	, AxisName(_AxisName)
	, AxisScale(_AxisDirection)
	{}

	/** Helper */
	bool operator==(const FAxisBindingsRedirectStruct& LHS) const
	{
		return LHS.MetaAxisName == MetaAxisName &&
			LHS.AxisName == AxisName &&
			LHS.AxisScale == AxisScale;
	}

	/** Helper */
	bool operator!=(const FAxisBindingsRedirectStruct& LHS) const
	{
		return !(LHS == *this);
	}

	UPROPERTY(EditAnywhere)
	FName MetaAxisName;

	UPROPERTY(EditAnywhere)
	FName AxisName;

	UPROPERTY(EditAnywhere)
	float AxisScale;
};

USTRUCT()
struct FAxisKeyRedirect
{
	GENERATED_BODY()

		/** Ctor */
	FAxisKeyRedirect(FKey _AxisKey = EKeys::Invalid, float _AxisScale = 0.0f, FKey _DisplayKey = EKeys::Invalid)
	: AxisKey(_AxisKey)
	, AxisScale(_AxisScale)
	, DisplayKey(_DisplayKey)
	{}

	/** Helper */
	bool operator==(const FAxisKeyRedirect& LHS) const
	{
		return AxisKey == LHS.AxisKey &&
			AxisScale == LHS.AxisScale &&
			DisplayKey == LHS.DisplayKey;
	}

	/** Helper */
	bool operator!=(const FAxisKeyRedirect& LHS) const
	{
		return !(LHS == *this);
	}

	UPROPERTY(EditAnywhere)
	FKey AxisKey;

	UPROPERTY(EditAnywhere)
	float AxisScale;

	UPROPERTY(EditAnywhere)
	FKey DisplayKey;
};

/** 
 * Central shared storehouse for everything needed to display input keys and bindings.
 * This should be the one asset to rule them all when it comes to managing how we display input stuff.
 * 
 * There should be very few cases where you need to access this information directly - instead rely on
 * the UUTInputVisualizer widget in UMG or, if you're working at Slate level, call CreateWidgetForKey() here.
 */
UCLASS()
class UNREALTOURNAMENT_API UUTInputDisplayData : public UDataAsset
{
	GENERATED_UCLASS_BODY()

public:
	virtual void PreSave(const class ITargetPlatform* TargetPlatform) override;
	virtual void PostLoad() override;

	/** Replaces the given action name with the gamepad equivalent */
	void ConvertToGamepadEquivalent(FName& ActionName) const;

	/** @return The brush for the given button key, or an empty brush if there is no icon or the key isn't a button. */
	const FSlateBrush& GetButtonBrush(const FKey& Key, bool bPreferSecondary = false) const;

	/** @return the input blacklist */
	const TArray<FInputChord>& GetInputBlacklist() const;

	/** @return true if the given key is on the blacklist */
	bool IsKeyOnBlacklist(const FInputChord& InChord) const;

	/** @return Widget content appropriate to the given key */
	TSharedRef<SWidget> CreateWidgetForKey(const FKey& Key,
		EUTWidgetStyleSize::Type Size = EUTWidgetStyleSize::Small,
		bool bIncludeKeyBorder = true,
		bool bPreferSecondaryIcon = false,
		const FText& KeyNameOverride = FText::GetEmpty()) const;

	/** @return Widget content appropriate to the given action */
	TSharedRef<SWidget> CreateWidgetForAction(FName ActionName, 
		const APlayerController* PC,
		EUTWidgetStyleSize::Type Size = EUTWidgetStyleSize::Small,
		bool bIncludeKeyBorder = true,
		bool bPreferSecondaryIcon = false) const;

	/** Creates a widget representing the axis binding */
	TSharedRef<SWidget> CreateWidgetForAxis(FName AxisName,
		const APlayerController* PC,
		EUTWidgetStyleSize::Type Size = EUTWidgetStyleSize::Small,
		bool bIncludeKeyBorder = true,
		bool bPreferSecondaryIcon = false) const;

	/** Gets the readable name from an action name */
	const FText& GetReadableName(FName ActionName) const;

private:

	/** Action name equivalences for cases where gamepad input utilizes a differently named binding for the same action. */
	UPROPERTY(Transient, EditAnywhere, Category = Actions)
	TArray<FInputActionGamepadEquivalency> GamepadActionEquivalences;

	/** We serialize the equivalences above into a map - much easier to work with that way */
	UPROPERTY()
	TMap<FName, FName> GamepadActionEquivalencesMap;

	/** The preferred height of gamepad buttons */
	UPROPERTY(EditAnywhere, Category = Buttons)
	float ButtonIconHeights[EUTWidgetStyleSize::MAX];

	/** Display information for all gamepad buttons */
	UPROPERTY(EditAnywhere, Category = Gamepad)
	TArray<FGamepadButtonArt> GamepadButtons;

	/** 
	 * On occasion, gamepad buttons may need a secondary or otherwise nonstandard icon.
	 * In such cases, it is up to the user of this asset to determine which version they need.
	 * Ex: DPad buttons can include the entire DPad or just the button itself
	 */
	UPROPERTY(EditAnywhere, Category = Gamepad)
	TArray<FGamepadButtonArt> SecondaryGamepadButtons;

	/** Display icons for all mouse buttons */
	UPROPERTY(EditAnywhere, Category = Mouse)
	TArray<FMouseButtonArt> MouseButtons;

	/** The background image on which to add the appropriate text for the key */
	UPROPERTY(EditAnywhere, Category = Keyboard)
	FSlateBrush KeyboardKeyBackground;

	/** The style to use for the key text */
	UPROPERTY(EditAnywhere, Category = Keyboard)
	TSubclassOf<class UUTTextStyle> KeyboardKeyTextStyle;

	/** The padding to put around the key text */
	UPROPERTY(EditAnywhere, Category = Keyboard)
	FMargin KeyTextPadding[EUTWidgetStyleSize::MAX];

	/** An array of structs defining an action binding name and the associated readable name */
	UPROPERTY(Transient, EditAnywhere, Category = "Readable Names")
	TArray<FActionBindingToReadableName> ReadableNamesArray;

	UPROPERTY(EditAnywhere, Category = "Axis Bindings")
	TArray<FAxisBindingsRedirectStruct> AxisRedirects;

	UPROPERTY(EditAnywhere, Category = "Axis Bindings")
	TArray<FAxisKeyRedirect> AxisKeyRedirects;

	/** Serialized out from the array above */
	UPROPERTY()
	TMap<FName, FText> ReadableNamesMap;

	UPROPERTY()
	TMap<FName, FText> OverrideNames;

	UPROPERTY(EditAnywhere, Category = "Input Blacklist")
	TArray<FInputChord> InputBlacklist;

	FSlateBrush MyEmptyBrush;

	/** Internal helper struct */
	struct FUTChords
	{
		/** Default Ctor */
		FUTChords()
		{}

		/** Helper functions */
		void PopulateFromActionKeyMapping(const FInputActionKeyMapping* ActionKeyMapping);
		void PopulateFromUTActionMapping(const FUTPlayerInput_ActionMapping* UTActionMapping);

		/** Checks to see if this has any keys at all */
		bool IsValid() const;

		FKey PrimaryKey;
		TArray<FKey> Chords;
	};
};