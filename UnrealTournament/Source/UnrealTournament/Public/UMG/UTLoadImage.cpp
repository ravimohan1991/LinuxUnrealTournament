// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTLazyImage.h"

#include "UTTextBlock.h"
#include "UTLoadGuard.h"
#include "UTGameUIData.h"

#if WITH_PROFILE
#include "UtMcpDefinition.h"
#else
#include "GithubStubs.h"
#endif

UUTLazyImage::UUTLazyImage(const FObjectInitializer& Initializer)
	: Super(Initializer)
	, LoadGuard(nullptr)
{
#if WITH_EDITORONLY_DATA
	bShowLoading = false;
#endif
}

bool UUTLazyImage::IsLoading() const
{
	return LoadGuard ? LoadGuard->IsLoading() : false;
}

void UUTLazyImage::CancelLoading()
{
	LazyAsset.Reset();
	if ( LoadGuard )
	{
		LoadGuard->SetIsLoading(true);
	}
}

TSharedRef<SWidget> UUTLazyImage::RebuildWidget()
{
	LoadGuard = NewObject<UUTLoadGuard>(this);
	// For images, just show the spinner
	LoadGuard->SetLoadingText(FText());

	// Very important to TakeWidget() BEFORE actually setting the content
	TSharedRef<SWidget> LoadGuardWidget = LoadGuard->TakeWidget();

	// The load guard contains the actual image
	LoadGuard->SetContent(Super::RebuildWidget());
	
	if (!IsDesignTime())
	{
		LoadGuard->OnLoadingStateChanged.AddUniqueDynamic(this, &UUTLazyImage::ForwardLoadingStateChanged);
	}

	return LoadGuardWidget;
}

void UUTLazyImage::OnWidgetRebuilt()
{
	Super::OnWidgetRebuilt();

	if (!LazyAsset.IsNull())
	{
		SetBrushFromLazyDisplayAsset(LazyAsset);
	}
}

void UUTLazyImage::SynchronizeProperties()
{
	Super::SynchronizeProperties();

#if WITH_EDITORONLY_DATA
	if (LoadGuard)
	{
		LoadGuard->SetIsLoading(bShowLoading);
	}
#endif
}

void UUTLazyImage::ForwardLoadingStateChanged(bool bIsLoading)
{
	OnLoadingStateChanged.Broadcast(bIsLoading);
}

void UUTLazyImage::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	LazyAsset.Reset();
	if (LoadGuard)
	{
		LoadGuard->ReleaseSlateResources(bReleaseChildren);
		LoadGuard = nullptr;
	}
}

void UUTLazyImage::SetBrushFromLazyTexture(const TAssetPtr<UTexture2D>& LazyTexture, bool bMatchSize)
{
	if (!LazyTexture.IsNull())
	{
		LazyAsset = LazyTexture;
		bMatchSizeIfTexture = bMatchSize;
		SetBrushFromTexture(nullptr);

		if (LoadGuard)
		{
			LoadGuard->GuardAndLoadAsset<UTexture2D>(LazyTexture, 
				[this] (UTexture2D* Texture)
				{
					SetBrushFromTexture(Texture, bMatchSizeIfTexture);
				});
		}
		else
		{
			UE_LOG(UT, Verbose, TEXT("Tried to set [%s] from a lazy texture before the load guard is valid"), *GetName());
		}
	}
	else
	{
		ShowDefaultTexture();
	}
}

void UUTLazyImage::SetBrushFromLazyMaterial(const TAssetPtr<UMaterialInterface>& LazyMaterial)
{
	if (!LazyMaterial.IsNull())
	{
		LazyAsset = LazyMaterial;
		bMatchSizeIfTexture = false;
		SetBrushFromTexture(nullptr);
		
		if (LoadGuard)
		{
			LoadGuard->GuardAndLoadAsset<UMaterialInterface>(LazyMaterial,
				[this] (UMaterialInterface* Material)
				{
					SetBrushFromMaterial(Material);
				});
		}
		else
		{
			UE_LOG(UT, Warning, TEXT("Tried to set [%s] from a lazy material before the load guard is valid"), *GetName());
		}
	}
	else
	{
		ShowDefaultTexture();
	}
}

void UUTLazyImage::SetBrushFromItemDefinition(UUtMcpDefinition* ItemDefinition, bool bMatchTextureSize)
{
#if WITH_PROFILE
	if (ItemDefinition)
	{
		SetBrushFromLazyDisplayAsset(ItemDefinition->GetIconAsset(), bMatchTextureSize);
	}
	else
	{
		ShowDefaultTexture();
	}
#endif
}

void UUTLazyImage::SetBrushFromItemDefinition(const UUtMcpDefinition* ItemDefinition, bool bMatchTextureSize)
{
#if WITH_PROFILE
	if (ItemDefinition)
	{
		SetBrushFromLazyDisplayAsset(ItemDefinition->GetIconAsset(), bMatchTextureSize);
	}
	else
	{
		ShowDefaultTexture();
	}
#endif
}

void UUTLazyImage::SetBrushFromLazyDisplayAsset(const TAssetPtr<UObject>& LazyObject, bool bMatchTextureSize)
{
#if WITH_PROFILE
	if (!LazyObject.IsNull())
	{
		// If the display asset is an item definition, we can expect that it's already loaded
		// What we really want is the display asset within the item definition
		if (UUtMcpDefinition* AsItemDefinition = Cast<UUtMcpDefinition>(LazyObject.Get()))
		{
			LazyAsset = AsItemDefinition->GetIconAsset();
		}
		else
		{
			LazyAsset = LazyObject;
		}
		
		bMatchSizeIfTexture = bMatchTextureSize;
		SetBrushFromTexture(nullptr);

		if (LoadGuard)
		{
			LoadGuard->GuardAndLoadAsset<UObject>(LazyAsset,
				[this] (UObject* Object)
				{
					if (UTexture2D* AsTexture = Cast<UTexture2D>(Object))
					{
						SetBrushFromTexture(AsTexture, bMatchSizeIfTexture);
					}
					else if (UMaterialInterface* AsMaterial = Cast<UMaterialInterface>(Object))
					{
						SetBrushFromMaterial(AsMaterial);
					}
				});
		}
		else
		{
			UE_LOG(UT, Warning, TEXT("Tried to set [%s] from a display asset before the load guard is valid"), *GetName());
		}
	}
	else
	{
		ShowDefaultTexture();
	}
#endif
}

void UUTLazyImage::SetTextureParamFromLazyAsset(const FName& TextureParamName, const TAssetPtr<UObject>& LazyObject)
{
#if WITH_PROFILE
	if (!LazyObject.IsNull())
	{
		// If the display asset is an item definition, we can expect that it's already loaded
		// What we really want is the display asset within the item definition
		if (UUtMcpDefinition* AsItemDefinition = Cast<UUtMcpDefinition>(LazyObject.Get()))
		{
			LazyAsset = AsItemDefinition->GetIconAsset();
		}
		else
		{
			LazyAsset = LazyObject;
		}

		if (LoadGuard)
		{
			LoadGuard->GuardAndLoadAsset<UObject>(LazyAsset,
				[this, TextureParamName](UObject* Object)
			{
				if (UTexture2D* AsTexture = Cast<UTexture2D>(Object))
				{
					if (UMaterialInstanceDynamic* DynamicBrushMaterial = GetDynamicMaterial())
					{
						DynamicBrushMaterial->SetTextureParameterValue(TextureParamName, AsTexture);
					}
				}
			});
		}
		else
		{
			UE_LOG(UT, Warning, TEXT("Tried to set [%s] from a display asset before the load guard is valid"), *GetName());
		}
	}
	else
	{
		ShowDefaultTexture();
	}
#endif
}

void UUTLazyImage::ShowDefaultTexture()
{
	SetBrushFromTexture(UUTGameUIData::Get().DefaultIcon);
}