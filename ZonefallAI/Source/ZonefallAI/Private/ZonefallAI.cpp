#include "ZonefallAI.h"

#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogZonefallAI);

class FZonefallAIModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		UE_LOG(LogZonefallAI, Log, TEXT("ZonefallAI module started"));
	}

	virtual void ShutdownModule() override
	{
		UE_LOG(LogZonefallAI, Log, TEXT("ZonefallAI module shutdown"));
	}
};

IMPLEMENT_MODULE(FZonefallAIModule, ZonefallAI)
