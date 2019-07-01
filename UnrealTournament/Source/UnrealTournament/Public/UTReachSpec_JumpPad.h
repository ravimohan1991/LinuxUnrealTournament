// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UTReachSpec.h"
#include "UTRecastNavMesh.h"
#include "UTJumpPad.h"

#include "UTReachSpec_JumpPad.generated.h"

UCLASS(CustomConstructor)
class UNREALTOURNAMENT_API UUTReachSpec_JumpPad : public UUTReachSpec
{
	GENERATED_UCLASS_BODY()

	/** the jump pad Actor that needs to be touched to jump */
	UPROPERTY()
	TWeakObjectPtr<AUTJumpPad> JumpPad;

	UUTReachSpec_JumpPad(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	{
		PathColor = FLinearColor(0.0f, 0.0f, 1.0f);
	}

	virtual TWeakObjectPtr<AActor> GetSourceActor() const override
	{
		return JumpPad;
	}

	virtual int32 CostFor(int32 DefaultCost, const FUTPathLink& OwnerLink, APawn* Asker, const FNavAgentProperties& AgentProps, const FUTReachParams& ReachParams, AController* RequestOwner, NavNodeRef StartPoly, const class AUTRecastNavMesh* NavMesh) override
	{
		if (!JumpPad.IsValid() || !JumpPad->IsEnabled())
		{
			// TODO: check for trigger to enable?
			return BLOCKED_PATH_COST;
		}
		else
		{
			int32 AdjustedCost = DefaultCost;
			// add extra cost if the boost goes through water which will slow us down
			if (OwnerLink.ReachFlags & R_SWIM)
			{
				AdjustedCost += FMath::TruncToInt((NavMesh->GetPolyCenter(OwnerLink.StartEdgePoly) - NavMesh->GetPolyCenter(OwnerLink.EndPoly)).Size()) / 2;
			}
			return AdjustedCost;
		}
	}

	virtual bool GetMovePoints(const FUTPathLink& OwnerLink, const FVector& StartLoc, APawn* Asker, const FNavAgentProperties& AgentProps, const struct FRouteCacheItem& Target, const TArray<FRouteCacheItem>& FullRoute, const class AUTRecastNavMesh* NavMesh, TArray<FComponentBasedPosition>& MovePoints) const
	{
		TArray<NavNodeRef> PolyRoute;
		NavMesh->FindPolyPath(StartLoc, AgentProps, FRouteCacheItem(NavMesh->GetPolyCenter(OwnerLink.StartEdgePoly), OwnerLink.StartEdgePoly), PolyRoute, false);
		NavMesh->DoStringPulling(StartLoc, PolyRoute, AgentProps, MovePoints);
		if (JumpPad.IsValid())
		{
			// the jump pad is generally affixed to the floor, so we want to offset upwards to where the Pawn's location would be when touching it
			// however, we don't want to do this for pads that are affixed to walls, as the navmesh is only built on floors and so pushing upwards in either local or world Z is likely to just push the location off the mesh
			FVector JumpPadMoveLocation = JumpPad->TriggerBox->GetComponentLocation();
			const FVector JumpPadZAxis = JumpPad->TriggerBox->GetComponentTransform().GetUnitAxis(EAxis::Z);
			if (JumpPadZAxis.Z >= 0.7f)
			{
				JumpPadMoveLocation += JumpPadZAxis * ((AgentProps.AgentHeight * 0.5f) - JumpPad->TriggerBox->RelativeLocation.Z);
			}

			MovePoints.Add(FComponentBasedPosition(JumpPad->GetRootComponent(), JumpPadMoveLocation));
		}
		MovePoints.Add(FComponentBasedPosition(Target.GetLocation(Asker)));
		return true;
	}
};