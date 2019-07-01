// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTBotCharacter.h"

UUTBotCharacter::UUTBotCharacter(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	MaxRunSpeed = 1.f;
	MultiJumpImpulse = 1.f;
	MaxMultiJumpCount = 0;
	bCanDodge = true;
	AirControl = 1.f;
	DodgeResetInterval = 1.f;
	JumpImpulse = 1.f;
	ExtraArmor = 0;
	MaxWallRunFallZ = 1.f;
	WallRunGravityScaling = 1.f;
	MaxAdditiveDodgeJumpSpeed = 1.f;
	DodgeImpulseHorizontal = 1.f;
	DodgeImpulseVertical = 1.f;
	DodgeMaxHorizontalVelocity = 1.f;
	WallDodgeSecondImpulseVertical = 1.f;
	WallDodgeImpulseHorizontal = 1.f;
	WallDodgeImpulseVertical = 1.f;
	FloorSlideEndingSpeedFactor = 1.f;
	FloorSlideAcceleration = 1.f;
	MaxFloorSlideSpeed = 1.f;
	MaxInitialFloorSlideSpeed = 1.f;
	FloorSlideDuration = 1.f;
}