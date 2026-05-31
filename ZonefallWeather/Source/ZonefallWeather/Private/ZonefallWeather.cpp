#include "ZonefallWeather.h"

#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogZonefallWeather);

class FZonefallWeatherModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		UE_LOG(LogZonefallWeather, Log, TEXT("ZonefallWeather module started"));
	}

	virtual void ShutdownModule() override
	{
		UE_LOG(LogZonefallWeather, Log, TEXT("ZonefallWeather module shutdown"));
	}
};

IMPLEMENT_MODULE(FZonefallWeatherModule, ZonefallWeather)
