// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTObjectListItem.h"

UUTObjectListItem::UUTObjectListItem(const FObjectInitializer& Initializer)
	: Super(Initializer)
{
}

UObject* IUTObjectListItem::GetData_Implementation() const
{
	return nullptr;
}

void IUTObjectListItem::SetData_Implementation(UObject* InData)
{
	// stub
}

void IUTObjectListItem::Reset_Implementation()
{
	// stub
}
