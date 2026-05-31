#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "CombatAnimSetupFactory.generated.h"

UCLASS()
class COMBATCOREEDITOR_API UCombatAnimSetupFactory : public UFactory
{
    GENERATED_BODY()

public:
    UCombatAnimSetupFactory();

    virtual UObject* FactoryCreateNew(
        UClass* Class,
        UObject* InParent,
        FName Name,
        EObjectFlags Flags,
        UObject* Context,
        FFeedbackContext* Warn
    ) override;

    virtual bool ShouldShowInNewMenu() const override { return true; }
};