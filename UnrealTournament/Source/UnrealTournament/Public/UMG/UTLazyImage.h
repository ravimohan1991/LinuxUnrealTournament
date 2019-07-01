// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTLoadGuard.h"
#include "UTUITypes.h"
#if WITH_PROFILE
#include "UtMcpDefinition.h"
#else
#include "GithubStubs.h"
#endif
#include "UTLazyImage.generated.h"

class UUTLoadGuard;

/**
 * A special Image widget that can show unloaded images and takes care of the loading for you!
 */
UCLASS()
class UNREALTOURNAMENT_API UUTLazyImage : public UImage
{
	GENERATED_UCLASS_BODY()

public:
	/** Set the brush from a lazy texture asset pointer - will load the texture as needed. */
	UFUNCTION(BlueprintCallable, Category = LazyImage)
	void SetBrushFromLazyTexture(const TAssetPtr<UTexture2D>& LazyTexture, bool bMatchSize = false);

	/** Set the brush from a lazy material asset pointer - will load the material as needed. */
	UFUNCTION(BlueprintCallable, Category = LazyImage)
	void SetBrushFromLazyMaterial(const TAssetPtr<UMaterialInterface>& LazyMaterial);
	
	/** Convenience function to display an mcp item icon */
	UFUNCTION(BlueprintCallable, Category = LazyImage)
	void SetBrushFromItemDefinition(UUtMcpDefinition* ItemDefinition, bool bMatchTextureSize = false);
	void SetBrushFromItemDefinition(const UUtMcpDefinition* ItemDefinition, bool bMatchTextureSize = false);

	/** Set the brush from a string asset ref only - expects the referenced asset to be a texture, material, or item definition. */
	UFUNCTION(BlueprintCallable, Category = LazyImage)
	void SetBrushFromLazyDisplayAsset(const TAssetPtr<UObject>& LazyObject, bool bMatchTextureSize = false);

	/** Sets a material parameter on the dynamic brush material, provided the given  */
	void SetTextureParamFromLazyAsset(const FName& TextureParamName, const TAssetPtr<UObject>& LazyObject);

	UFUNCTION(BlueprintCallable, Category = LazyImage)
	bool IsLoading() const;

	void CancelLoading();

	UPROPERTY(BlueprintAssignable, Category = LazyImage)
	FOnLoadingStateChanged OnLoadingStateChanged;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void OnWidgetRebuilt() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	virtual void SynchronizeProperties() override;

#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override { return FText::FromString(TEXT("UT")); }
#endif	

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = LoadGuard)
	bool bShowLoading;
#endif

private:
	// Simply forwards the changes from the load guard
	UFUNCTION() void ForwardLoadingStateChanged(bool bIsLoading);

	TAssetPtr<UObject> LazyAsset;
	bool bMatchSizeIfTexture;

	void ShowDefaultTexture();

	UPROPERTY(Transient)
	UUTLoadGuard* LoadGuard;
};