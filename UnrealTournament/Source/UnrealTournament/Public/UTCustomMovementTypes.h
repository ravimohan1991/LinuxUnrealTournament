#pragma once

#include "UTCustomMovementTypes.generated.h"

/** Movement modes for Characters. */
UENUM(BlueprintType)
enum EUTCustomMovementTypes
{
	/** None (movement is disabled). */
	CUSTOMMOVE_None		UMETA(DisplayName = "None"),

	/** Inside of a Line Up */
	CUSTOMMOVE_LineUp		UMETA(DisplayName = "LineUp"),

	CUSTOMMOVE_MAX		UMETA(Hidden),
};