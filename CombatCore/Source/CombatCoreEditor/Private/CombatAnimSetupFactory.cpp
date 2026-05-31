#include "CombatAnimSetupFactory.h"
#include "CombatAnimSetup.h"

UCombatAnimSetupFactory::UCombatAnimSetupFactory()
{
    SupportedClass = UCombatAnimSetup::StaticClass();
    bCreateNew = true;
    bEditAfterNew = true;
}

UObject* UCombatAnimSetupFactory::FactoryCreateNew(
    UClass* Class,
    UObject* InParent,
    FName Name,
    EObjectFlags Flags,
    UObject* Context,
    FFeedbackContext* Warn)
{
    return NewObject<UCombatAnimSetup>(InParent, Class, Name, Flags);
}