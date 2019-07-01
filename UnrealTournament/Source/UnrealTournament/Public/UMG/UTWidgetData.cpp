// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTGlobals.h"
#include "UTWidgetData.h"

//Helper Struct
struct FWidgetFinder
{
	/** Ctor */
	FWidgetFinder(FName _WidgetName)
	: WidgetName(_WidgetName)
	, WidgetClass(NULL)
	{
		check(IsValid());
	}

	FWidgetFinder(TSubclassOf<UUserWidget> _WidgetClass)
	: WidgetName(NAME_None)
	, WidgetClass(_WidgetClass)
	{
		check(IsValid());
	}

	FORCEINLINE bool IsValid() const
	{
		return (WidgetName != NAME_None) || (WidgetClass != NULL);
	}

	bool operator()(const FWidgetDataEntry& WidgetData) const
	{
		if (WidgetName != NAME_None)
		{
			return WidgetData.WidgetName == WidgetName;
		}
		else if (WidgetClass != NULL)
		{
			return WidgetData.Widget == WidgetClass;
		}

		return false;
	}

	FName WidgetName;
	TSubclassOf<UUserWidget> WidgetClass;
};

UUTWidgetData::UUTWidgetData(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{

}

class UUTWidgetData* UUTWidgetData::GetCurrent()
{
	UUTGlobals& UTGlobals = UUTGlobals::Get();
	return UTGlobals.GetWidgetData();
}

TSubclassOf<UUserWidget> UUTWidgetData::FindWidget(const FName WidgetName)
{
	return UUTWidgetData::GetCurrent()->FindWidgetHelper(WidgetName);
}

TSubclassOf<UUserWidget> UUTWidgetData::FindWidgetHelper(const FName WidgetName) const
{
	int32 Index = INDEX_NONE;
	if ((Index = WidgetEntries.IndexOfByPredicate(FWidgetFinder(WidgetName))) >= 0)
	{
		return WidgetEntries[Index].Widget;
	}
	else
	{
		UE_LOG(UT, Warning, TEXT("UUTWidgetData::FindWidgetHelper: Could not find WidgetClass %s"), *WidgetName.ToString());
		return NULL;
	}
}

#if WITH_EDITOR
void UUTWidgetData::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	static const FName WidgetName("Widget");
	if (PropertyChangedEvent.MemberProperty &&
		PropertyChangedEvent.Property &&
		PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UUTWidgetData, WidgetEntries) &&
		PropertyChangedEvent.Property->GetFName() == WidgetName)
	{
		for (FWidgetDataEntry& Entry : WidgetEntries)
		{
			if (Entry.WidgetName != NAME_None || Entry.Widget == NULL)
			{
				continue;
			}
			Entry.WidgetName = GetUniqueWidgetName(Entry.Widget);
		}
	}
}
#endif

FName UUTWidgetData::GetUniqueWidgetName(TSubclassOf<class UUserWidget> UserWidgetClass) const
{
	FName NameToUse = NAME_None;

	if (UserWidgetClass != NULL)
	{
		int32 NameIndex = 0;
		while (NameIndex < 1000)
		{
			FString WidgetName = UserWidgetClass->GetName();
			WidgetName.RemoveFromEnd(TEXT("_C"));

			//if we aren't trying for the first time, add a number to the end of this thing
			if (NameIndex > 0)
			{
				WidgetName = FString::Printf(TEXT("%s_%d"), *WidgetName, NameIndex);
			}

			//if we didn't find a match, then get out of this loop
			if (WidgetEntries.IndexOfByPredicate(FWidgetFinder(*WidgetName)) < 0)
			{
				NameToUse = *WidgetName;
				break;
			}
		}
	}

	return NameToUse;
}






