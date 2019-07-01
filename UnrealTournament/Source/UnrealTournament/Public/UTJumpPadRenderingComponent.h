// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UnrealTournament.h"
//#include "UTJumpPad.h"
#include "Runtime/Engine/Classes/Components/PrimitiveComponent.h"

#include "UTJumpPadRenderingComponent.generated.h"

UCLASS()
class UNREALTOURNAMENT_API UUTJumpPadRenderingComponent : public UPrimitiveComponent
{
	GENERATED_UCLASS_BODY()

	// Begin UPrimitiveComponent Interface
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;

	/** Should recreate proxy one very update */
	virtual bool ShouldRecreateProxyOnUpdateTransform() const override{ return true; }
	// End UPrimitiveComponent Interface

	// Begin USceneComponent Interface
	virtual FBoxSphereBounds CalcBounds(const FTransform &LocalToWorld) const override;
	// End USceneComponent Interface

	void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

	UPROPERTY()
	FVector GameThreadJumpVelocity;

	UPROPERTY()
	float GameThreadGravityZ;
};

//#if !UE_SERVER
/*
class UNREALTOURNAMENT_API UUTJumpPadRenderingProxy : public FPrimitiveSceneProxy
{
    GENERATED_UCLASS_BODY()

private:
    FVector JumpPadLocation;
    FVector JumpPadTarget;
    FVector JumpVelocity;
    float	JumpTime;
    float	GravityZ;

public:

    UUTJumpPadRenderingProxy(const UPrimitiveComponent* InComponent);// : FPrimitiveSceneProxy(InComponent);

*/
    /** Return a type (or subtype) specific hash for sorting purposes */
 /*   ENGINE_API virtual SIZE_T GetTypeHash() const override;


    virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override;


    virtual uint32 GetMemoryFootprint(void) const override
    {
        return(sizeof(*this));
    }

    virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override
    {
        FPrimitiveViewRelevance Result;
        Result.bDrawRelevance = IsShown(View) && (IsSelected() || View->Family->EngineShowFlags.Navigation);
        Result.bDynamicRelevance = true;
        Result.bNormalTranslucencyRelevance = IsShown(View);
        Result.bShadowRelevance = IsShadowCast(View);
        Result.bEditorPrimitiveRelevance = UseEditorCompositing(View);
        return Result;
    }
};

//#endif*/
