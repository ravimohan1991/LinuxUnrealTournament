// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTInputDisplayData.h"

#include "UTGameUIData.h"
#include "UTTextBlock.h"

#include "UTGlobals.h"

#define LOCTEXT_NAMESPACE "InputDisplayData"

/**
 * An image that scales proportionately in order to keep its desired height at the specified height.
 * This is a work around for input image icons being of all various sizes.
 */
class SScaledImage : public SImage
{
	SLATE_BEGIN_ARGS(SScaledImage)
	: _Image()
	, _TargetHeight(64.0f)
	{}
		SLATE_ARGUMENT(const FSlateBrush*, Image)
		SLATE_ARGUMENT(float, TargetHeight)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		SImage::Construct(
			SImage::FArguments()
			.Image(InArgs._Image)
		);

		TargetHeight = InArgs._TargetHeight;
	}


	virtual FVector2D ComputeDesiredSize(float) const override
	{
		static const float DontCare = 1.0f;
		const FVector2D ImageDesiredSize = SImage::ComputeDesiredSize(DontCare);

		const float ScaleFactor = TargetHeight / ((ImageDesiredSize.Y == 0) ? 1.0f : ImageDesiredSize.Y);
		
		return ImageDesiredSize*ScaleFactor;
	}

private:
	float TargetHeight;
};

UUTInputDisplayData::UUTInputDisplayData(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Gamepad Buttons
	GamepadButtons.Add(EKeys::Gamepad_DPad_Down);
	GamepadButtons.Add(EKeys::Gamepad_DPad_Left);
	GamepadButtons.Add(EKeys::Gamepad_DPad_Right);
	GamepadButtons.Add(EKeys::Gamepad_DPad_Up);

	GamepadButtons.Add(EKeys::Gamepad_LeftTrigger);
	GamepadButtons.Add(EKeys::Gamepad_RightTrigger);
	GamepadButtons.Add(EKeys::Gamepad_LeftShoulder);
	GamepadButtons.Add(EKeys::Gamepad_RightShoulder);
	
	GamepadButtons.Add(EKeys::Gamepad_FaceButton_Bottom);
	GamepadButtons.Add(EKeys::Gamepad_FaceButton_Left);
	GamepadButtons.Add(EKeys::Gamepad_FaceButton_Right);
	GamepadButtons.Add(EKeys::Gamepad_FaceButton_Top);

	GamepadButtons.Add(EKeys::Gamepad_Special_Left);
	GamepadButtons.Add(EKeys::Gamepad_Special_Right);

	GamepadButtons.Add(EKeys::Gamepad_LeftThumbstick);
	GamepadButtons.Add(EKeys::Gamepad_RightThumbstick);

	GamepadButtons.Add(EKeys::Gamepad_LeftStick_Up);
	GamepadButtons.Add(EKeys::Gamepad_LeftStick_Down);
	GamepadButtons.Add(EKeys::Gamepad_LeftStick_Left);
	GamepadButtons.Add(EKeys::Gamepad_LeftStick_Right);

	GamepadButtons.Add(EKeys::Gamepad_RightStick_Up);
	GamepadButtons.Add(EKeys::Gamepad_RightStick_Down);
	GamepadButtons.Add(EKeys::Gamepad_RightStick_Left);
	GamepadButtons.Add(EKeys::Gamepad_RightStick_Right);

	AxisRedirects.AddUnique(FAxisBindingsRedirectStruct(FName("MoveForward"), FName("MoveForward"), 1.0f));
	AxisRedirects.AddUnique(FAxisBindingsRedirectStruct(FName("MoveBack"), FName("MoveForward"), -1.0f));
	AxisRedirects.AddUnique(FAxisBindingsRedirectStruct(FName("MoveRight"), FName("MoveRight"), 1.0f));
	AxisRedirects.AddUnique(FAxisBindingsRedirectStruct(FName("MoveLeft"), FName("MoveRight"), -1.0f));

	AxisRedirects.AddUnique(FAxisBindingsRedirectStruct(FName("LookUp"), FName("LookPitch"), 1.0f));
	AxisRedirects.AddUnique(FAxisBindingsRedirectStruct(FName("LookDown"), FName("LookPitch"), -1.0f));
	AxisRedirects.AddUnique(FAxisBindingsRedirectStruct(FName("LookRight"), FName("LookYaw"), 1.0f));
	AxisRedirects.AddUnique(FAxisBindingsRedirectStruct(FName("LookLeft"), FName("LookYaw"), -1.0f));

	AxisKeyRedirects.AddUnique(FAxisKeyRedirect(EKeys::Gamepad_LeftY, 1.0f, EKeys::Gamepad_LeftStick_Up));
	AxisKeyRedirects.AddUnique(FAxisKeyRedirect(EKeys::Gamepad_LeftY, -1.0f, EKeys::Gamepad_LeftStick_Down));
	AxisKeyRedirects.AddUnique(FAxisKeyRedirect(EKeys::Gamepad_LeftX, 1.0f, EKeys::Gamepad_LeftStick_Right));
	AxisKeyRedirects.AddUnique(FAxisKeyRedirect(EKeys::Gamepad_LeftX, -1.0f, EKeys::Gamepad_LeftStick_Left));

	AxisKeyRedirects.AddUnique(FAxisKeyRedirect(EKeys::Gamepad_RightY, 1.0f, EKeys::Gamepad_RightStick_Up));
	AxisKeyRedirects.AddUnique(FAxisKeyRedirect(EKeys::Gamepad_RightY, -1.0f, EKeys::Gamepad_RightStick_Down));
	AxisKeyRedirects.AddUnique(FAxisKeyRedirect(EKeys::Gamepad_RightX, 1.0f, EKeys::Gamepad_RightStick_Right));
	AxisKeyRedirects.AddUnique(FAxisKeyRedirect(EKeys::Gamepad_RightX, -1.0f, EKeys::Gamepad_RightStick_Left));

	// Mouse Buttons
	MouseButtons.Add(EKeys::LeftMouseButton);
	MouseButtons.Add(EKeys::RightMouseButton);
	MouseButtons.Add(EKeys::MiddleMouseButton);
	MouseButtons.Add(EKeys::ThumbMouseButton);
	MouseButtons.Add(EKeys::ThumbMouseButton2);

	// Button icon sizing
	ButtonIconHeights[EUTWidgetStyleSize::Small] = 48.f;
	ButtonIconHeights[EUTWidgetStyleSize::Medium] = 64.f;
	ButtonIconHeights[EUTWidgetStyleSize::Large] = 80.f;

	// Override names for chord keys
	OverrideNames.Add(EKeys::LeftAlt.GetFName(), LOCTEXT("Alt Key", "Alt"));
	OverrideNames.Add(EKeys::LeftCommand.GetFName(), LOCTEXT("Command Key", "Cmd"));
	OverrideNames.Add(EKeys::LeftControl.GetFName(), LOCTEXT("Control Key", "Ctrl"));
	OverrideNames.Add(EKeys::LeftShift.GetFName(), LOCTEXT("Shift Key", "Shift"));
}

void UUTInputDisplayData::PreSave(const class ITargetPlatform* TargetPlatform)
{
#if WITH_EDITOR
	// When saving, convert the editor-exposed array into the map that we serialize
	GamepadActionEquivalencesMap.Empty(GamepadActionEquivalences.Num());
	for (const FInputActionGamepadEquivalency& Equivalency : GamepadActionEquivalences)
	{
		GamepadActionEquivalencesMap.Add(Equivalency.ActionName, Equivalency.GamepadActionNameEquivalent);
	}

	// Convert the editor-exposed array into a map
	ReadableNamesMap.Empty(ReadableNamesArray.Num());
	for (const FActionBindingToReadableName& ReadableName : ReadableNamesArray)
	{
		ReadableNamesMap.Add(ReadableName.ActionName, ReadableName.ReadableName);
	}

#endif

	Super::PreSave(TargetPlatform);
}

void UUTInputDisplayData::PostLoad()
{
#if WITH_EDITOR
	if (!IsRunningGame())
	{
		// At edit time, use the map to fill up the array for editing
		for (const auto& EquivalencyPair : GamepadActionEquivalencesMap)
		{
			FInputActionGamepadEquivalency NewEquivalency;
			NewEquivalency.ActionName = EquivalencyPair.Key;
			NewEquivalency.GamepadActionNameEquivalent = EquivalencyPair.Value;
			GamepadActionEquivalences.Add(NewEquivalency);
		}

		for (const auto& NamePair : ReadableNamesMap)
		{
			FActionBindingToReadableName NewStruct;
			NewStruct.ActionName = NamePair.Key;
			NewStruct.ReadableName = NamePair.Value;
			ReadableNamesArray.Add(NewStruct);
		}
	}
#endif // WITH_EDITOR

	Super::PostLoad();
}

void UUTInputDisplayData::ConvertToGamepadEquivalent(FName& ActionName) const
{
	if (const FName* GamepadActionName = GamepadActionEquivalencesMap.Find(ActionName))
	{
		ActionName = *GamepadActionName;
	}
}

const FSlateBrush& UUTInputDisplayData::GetButtonBrush(const FKey& Key, bool bPreferSecondary /*= false*/) const
{
	if (Key.IsGamepadKey())
	{
		const TFunction<bool(const FGamepadButtonArt&)> GamepadArtFinder(
			[Key](const FGamepadButtonArt& ButtonArt) 
			{ 
				return ButtonArt.Key == Key; 
			});

		const FGamepadButtonArt* GamepadArt = nullptr;
		if (bPreferSecondary)
		{
			GamepadArt = SecondaryGamepadButtons.FindByPredicate(GamepadArtFinder);
		}

		if (!GamepadArt)
		{
			// If we didn't find it in the secondary list (or didn't look), try the main list
			GamepadArt = GamepadButtons.FindByPredicate(GamepadArtFinder);
		}

		// Found anything?
		if (GamepadArt)
		{
#if PLATFORM_PS4
			//always return PS4 on the PS4
			return GamepadArt->PS4Icon;
#else
			//check for Nvidia Shield
			static const IConsoleVariable* CvarIsOnGFNPtr = IConsoleManager::Get().FindConsoleVariable(TEXT("UT.IsOnGFN"));
			const static bool bForcePS4 = FParse::Param(FCommandLine::Get(), TEXT("ForcePS4Icons"));

			if (bForcePS4)
			{
				return GamepadArt->PS4Icon;
			}
			if ((CvarIsOnGFNPtr->GetInt() != 0) && GamepadArt->NVShieldIcon.GetResourceObject() != nullptr)
			{
				return GamepadArt->NVShieldIcon;
			}
			//check for Xbox first, as it is more popular
			else if (GamepadArt->XBoxIcon.GetResourceObject() != nullptr)
			{
				return GamepadArt->XBoxIcon;
			}
			//fallback to PS4, as we know that works
			else
			{
				return GamepadArt->PS4Icon;
			}
#endif
		}
	}
	else if (Key.IsMouseButton())
	{
		// Mouse buttons don't have secondary entries, so just search the main list
		if (const FMouseButtonArt* MouseArt = MouseButtons.FindByPredicate([Key](const FMouseButtonArt& ButtonArt) { return ButtonArt.Key == Key; }))
		{
			return MouseArt->ButtonIcon;
		}
	}

	return MyEmptyBrush;
}

const TArray<FInputChord>& UUTInputDisplayData::GetInputBlacklist() const
{
	return InputBlacklist;
}

bool UUTInputDisplayData::IsKeyOnBlacklist(const FInputChord& InChord) const
{
	for (FInputChord BlacklistChord : InputBlacklist)
	{
		if (BlacklistChord == InChord)
		{
			return true;
		}
	}
	return false;
}

TSharedRef<SWidget> UUTInputDisplayData::CreateWidgetForKey(const FKey& Key, EUTWidgetStyleSize::Type Size, bool bIncludeKeyBorder, bool bPreferSecondaryIcon, const FText& KeyNameOverride) const
{
	TSharedPtr<SWidget> KeyWidget;
	if (Key.IsGamepadKey() || Key.IsMouseButton())
	{
		// non-keyboard keys (controller button, thumb stick, mouse button, etc.)
		// are best visualized by dedicated images
		KeyWidget =
			SNew(SScaledImage)
			.TargetHeight(ButtonIconHeights[Size])
			.Image(&GetButtonBrush(Key, bPreferSecondaryIcon));
	}
	else
	{
		FTextBlockStyle KeyTextStyle;
		if (KeyboardKeyTextStyle)
		{
			KeyboardKeyTextStyle.GetDefaultObject()->ToTextBlockStyle(KeyTextStyle, Size);
		}

		TSharedRef<STextBlock> KeyText =
			SNew(STextBlock)
			.TextStyle(&KeyTextStyle)
			.Text(KeyNameOverride.IsEmpty() ? Key.GetDisplayName() : KeyNameOverride);

		if (bIncludeKeyBorder)
		{
			KeyWidget =
				SNew(SBorder)
				.BorderImage(&KeyboardKeyBackground)
				.Padding(KeyTextPadding[Size])
				[
					KeyText
				];
		}
		else
		{
			KeyWidget = KeyText;
		}
	}

#if !UE_BUILD_SHIPPING
	if (FParse::Param(FCommandLine::Get(), TEXT("HideHUDKeys")))
	{
		KeyWidget->SetVisibility(EVisibility::Hidden);
	}
#endif

	return KeyWidget.ToSharedRef();
}

TSharedRef<SWidget> UUTInputDisplayData::CreateWidgetForAction(FName ActionName, const APlayerController* PC, EUTWidgetStyleSize::Type Size, bool bIncludeKeyBorder, bool bPreferSecondaryIcon) const
{
	if (ActionName == NAME_None || !PC || !PC->PlayerInput)
	{
		return SNullWidget::NullWidget;
	}

	//we will populate this depending on what input type we get
	FUTChords KeyCombo;
		
	if(!KeyCombo.PrimaryKey.IsValid())
	{
		// We'll use this a couple times to search for mappings
		const TFunction<bool(const FInputActionKeyMapping&)> KeyMappingFinder(
			[ActionName](const FInputActionKeyMapping& KeyMapping)
		{
			return ActionName == KeyMapping.ActionName;
		});

		const FInputActionKeyMapping* FoundMapping = PC->PlayerInput->ActionMappings.FindByPredicate(KeyMappingFinder);
		if (!FoundMapping)
		{
			FoundMapping = UPlayerInput::GetEngineDefinedActionMappings().FindByPredicate(KeyMappingFinder);
		}

		if (FoundMapping)
		{
			KeyCombo.PopulateFromActionKeyMapping(FoundMapping);
		}
	}

	if (KeyCombo.PrimaryKey.IsValid())
	{
		TSharedRef<SWidget> KeyWidget = CreateWidgetForKey(KeyCombo.PrimaryKey, Size, bIncludeKeyBorder, bPreferSecondaryIcon);

		//if we have chord keys to add, do that now
		if (KeyCombo.Chords.Num() > 0)
		{
			//TODO: Give the option for an VBox?
			TSharedRef<SHorizontalBox> KeyBox = SNew(SHorizontalBox);

			//go through all of our chords, adding them to our HBox
			for (const FKey& KeyToAdd : KeyCombo.Chords)
			{
				//check if we have an override name or not
				FText KeyNameOverride;
				if (const FText* FoundName = OverrideNames.Find(KeyToAdd.GetFName()))
				{
					KeyNameOverride = *FoundName;
				}

				//add the chord
				KeyBox->AddSlot()
				.AutoWidth()
				.Padding(0.f, 0.f, 4.f, 0.f)
				[
					CreateWidgetForKey(KeyToAdd, Size, bIncludeKeyBorder, bPreferSecondaryIcon, KeyNameOverride)
				];
			}

			// Finish with the the primary key in the mapping
			KeyBox->AddSlot()[KeyWidget];

			//return out finished widget
			return KeyBox;
		}
		else
		{
			return KeyWidget;
		}
	}

	return SNullWidget::NullWidget;
}

TSharedRef<SWidget> UUTInputDisplayData::CreateWidgetForAxis(FName AxisName, 
	const APlayerController* PC, 
	EUTWidgetStyleSize::Type Size, 
	bool bIncludeKeyBorder,
	bool bPreferSecondaryIcon) const
{
	TSharedRef<SWidget> KeyWidget = SNullWidget::NullWidget;

	if (const UPlayerInput* Input = PC ? PC->PlayerInput : nullptr)
	{
		//find the axis by name
		FName RealAxisName = NAME_None;
		float Scale = 0.0f;
		if (const FAxisBindingsRedirectStruct* FoundStruct = AxisRedirects.FindByPredicate([AxisName](const FAxisBindingsRedirectStruct& TempStruct) { return AxisName == TempStruct.MetaAxisName; }))
		{
			RealAxisName = FoundStruct->AxisName;
			Scale = FoundStruct->AxisScale;
		}
		
		//keep reusing this static array
		static TArray<FInputAxisKeyMapping> FoundMappings;
		FoundMappings.Reset();

		// We'll use this a couple times to search for mappings
		const TFunction<bool(const FInputAxisKeyMapping&)> AxisMappingFinder(
		[RealAxisName](const FInputAxisKeyMapping& AxisMapping)
		{
			return RealAxisName == AxisMapping.AxisName;
		});

		//filter our axis mappings so we just have matching ones left
		FoundMappings = Input->AxisMappings.FilterByPredicate(AxisMappingFinder);
		if (FoundMappings.Num() == 0)
		{
			FoundMappings = UPlayerInput::GetEngineDefinedAxisMappings().FilterByPredicate(AxisMappingFinder);
		}

		//track down the final mapping
		const FInputAxisKeyMapping* FinalMapping = nullptr;
		if (FoundMappings.Num() == 1)
		{
			FinalMapping = &FoundMappings[0];
		}
		else
		{
			//find the axis mapping by scale
			const TFunction<bool(const FInputAxisKeyMapping&)> FindByScale(
			[Scale](const FInputAxisKeyMapping& AxisMapping)
			{
				return FMath::Sign<float>(Scale) == FMath::Sign<float>(AxisMapping.Scale);
			});

			//grab the final mapping
			FinalMapping = FoundMappings.FindByPredicate(FindByScale);
		}

		//do we have a mapping and a key?
		if (FinalMapping && FinalMapping->Key.IsValid())
		{
			FKey FinalKey = FinalMapping->Key;

			const TFunction<bool(const FAxisKeyRedirect&)> FindRedirectKey(
			[FinalKey, Scale](const FAxisKeyRedirect& AxisKeyRedirect)
			{
				return FinalKey == AxisKeyRedirect.AxisKey && FMath::Sign<float>(Scale) == FMath::Sign<float>(AxisKeyRedirect.AxisScale);
			});

			//if we have a key redirect, do that here as well
			if (const FAxisKeyRedirect* FoundRedirect = AxisKeyRedirects.FindByPredicate(FindRedirectKey))
			{
				FinalKey = FoundRedirect->DisplayKey;
			}

		 	//create the key widget
		 	KeyWidget = CreateWidgetForKey(FinalKey, Size, bIncludeKeyBorder, bPreferSecondaryIcon);
		}
	}

	return KeyWidget;
}

const FText& UUTInputDisplayData::GetReadableName(FName ActionName) const
{
	if (const FText* FoundName = ReadableNamesMap.Find(ActionName))
	{
		return *FoundName;
	}
	else
	{
		ConvertToGamepadEquivalent(ActionName);
		if (const FText* GamepadFoundName = ReadableNamesMap.Find(ActionName))
		{
			return *GamepadFoundName;
		}
	}
	return FText::GetEmpty();
}

void UUTInputDisplayData::FUTChords::PopulateFromActionKeyMapping(const FInputActionKeyMapping* ActionKeyMapping)
{
	if (ensure(ActionKeyMapping))
	{
		PrimaryKey = ActionKeyMapping->Key;

		if (ActionKeyMapping->bAlt)
		{
			Chords.Add(EKeys::LeftAlt);
		}

		if (ActionKeyMapping->bCmd)
		{
			Chords.Add(EKeys::LeftCommand);
		}

		if (ActionKeyMapping->bCtrl)
		{
			Chords.Add(EKeys::LeftControl);
		}

		if (ActionKeyMapping->bShift)
		{
			Chords.Add(EKeys::LeftShift);
		}
	}
}

void UUTInputDisplayData::FUTChords::PopulateFromUTActionMapping(const FUTPlayerInput_ActionMapping* UTActionMapping)
{
	if (ensure(UTActionMapping))
	{
		PrimaryKey = UTActionMapping->Key;

		if (UTActionMapping->ChordKey.IsValid())
		{
			Chords.Add(UTActionMapping->ChordKey);
		}
	}
}

bool UUTInputDisplayData::FUTChords::IsValid() const
{
	return PrimaryKey.IsValid();
}

#undef LOCTEXT_NAMESPACE


