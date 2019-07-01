// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UMG.h"
#include "UTWidgetData.generated.h"

USTRUCT()
struct FWidgetDataEntry
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditDefaultsOnly, Category = WidgetData)
	FName WidgetName;

	UPROPERTY(EditDefaultsOnly, Category = WidgetData)
	TSubclassOf< UUserWidget > Widget;
};

USTRUCT()
struct FDefaultTextureDataEntry
{
	GENERATED_USTRUCT_BODY()

	/** Ctor */
	FDefaultTextureDataEntry()
	: DefaultTexture(nullptr)
	{}
	
	UPROPERTY(EditDefaultsOnly, Category = WidgetData)
	FName DefaultTextureName;

	UPROPERTY(EditDefaultsOnly, Category = WidgetData)
	class UTexture2D* DefaultTexture;
};

/** Used to get access to UClasses of UUserWidgets from Native Slate Code */
UCLASS()
class UNREALTOURNAMENT_API UUTWidgetData : public UDataAsset
{
	GENERATED_UCLASS_BODY()

	/** Gets the latest Widget Data */
	static class UUTWidgetData* GetCurrent();

	/** Static helper. Simply calls FindWidgetHelper. Eliminates the need to do UUTWidgetData::GetCurrent()->FindWidget */
	static TSubclassOf<UUserWidget> FindWidget(const FName WidgetName);

	/** Templated version of the function above */
	template< class T >
	static TSubclassOf<T> FindWidget(const FName WidgetName)
	{
		TSubclassOf<T> RetValue(*FindWidget(WidgetName));
		return RetValue;
	}

	/** Called by FindWidget  */
	TSubclassOf<UUserWidget> FindWidgetHelper(const FName WidgetName) const;

	////////////////////////////////////////////////////////////////
	// Editor Code 
	////////////////////////////////////////////////////////////////

#if WITH_EDITOR
	/** Used to automatically setup the Widget Name property */
	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent);
#endif

protected:

	/** These are all the Widgets that can be accessed from Native Code */
	UPROPERTY(EditDefaultsOnly, Category = Widgets)
	TArray<FWidgetDataEntry> WidgetEntries;

private:

	/** Helper function */
	FName GetUniqueWidgetName(TSubclassOf<class UUserWidget> UserWidgetClass) const;
};