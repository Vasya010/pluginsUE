// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AssetTypeActions_Base.h"
#include "AssetTypeCategories.h"

class FGameplayTaskCompAssetActions : public FAssetTypeActions_Base
{
public:
	explicit FGameplayTaskCompAssetActions(EAssetTypeCategories::Type InCategory)
		: MyCategory(InCategory)
	{
	}

	// --- wymagane override ---
	virtual FText GetName() const override;
	virtual FColor GetTypeColor() const override;
	virtual UClass* GetSupportedClass() const override;
	virtual uint32 GetCategories() override;

private:
	EAssetTypeCategories::Type MyCategory = EAssetTypeCategories::Gameplay;
};
