


#include "CombatAnimSetup.h"

TMap<FName, FCombatAnimTrajectorySavedData> UCombatAnimSetup::GetAssetCalculatedData() const
{
    return PreDefinedData;
}


#if WITH_EDITOR
void UCombatAnimSetup::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    const FName PropName = PropertyChangedEvent.GetPropertyName();

    // Broadcast do wszystkich (viewport client, toolkit, itd.)
    OnChangedDelegate.Broadcast(this, PropName);
}
#endif
