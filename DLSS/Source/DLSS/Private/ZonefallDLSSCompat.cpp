// Zonefall Protocol — проверки политики графики при крупных обновлениях DLSS/FSR/Streamline.

#include "ZonefallDLSSCompat.h"
#include "DLSSUpscalerPrivate.h"
#include "HAL/IConsoleManager.h"
#include "Misc/CoreDelegates.h"

namespace ZonefallDLSSCompat
{
	static TAutoConsoleVariable<int32> CVarZonefallPreferDLSSGOverFSRFI(
		TEXT("r.Zonefall.Graphics.PreferDLSSGOverFSRFI"),
		1,
		TEXT("When 1, enabling DLSS-G while FSR Frame Interpolation is on will disable r.FidelityFX.FI.Enabled (Zonefall stability)."),
		ECVF_Default);

	static void LogPatchVersion()
	{
		UE_LOG(LogDLSS, Log, TEXT("[Zonefall] DLSS integration patch %s (UE 5.8). See Plugins/DLSS/ZONEFALL.md"), ZONEFALL_DLSS_PATCH_VERSION);
	}

	static void ResolveFrameGenerationConflict()
	{
		IConsoleVariable* FICVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.FidelityFX.FI.Enabled"));
		const IConsoleVariable* DLSSGCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Streamline.DLSSG.Enable"));
		if (!FICVar || !DLSSGCVar)
		{
			return;
		}

		const int32 FIEnabled = FICVar->GetInt();
		const int32 DLSSGEnabled = DLSSGCVar->GetInt();
		if (FIEnabled == 0 || DLSSGEnabled == 0)
		{
			return;
		}

		if (CVarZonefallPreferDLSSGOverFSRFI.GetValueOnAnyThread() != 0)
		{
			FICVar->Set(0, ECVF_SetByConsole);
			UE_LOG(LogDLSS, Warning,
				TEXT("[Zonefall] Disabled FSR Frame Interpolation because DLSS-G is enabled (r.Zonefall.Graphics.PreferDLSSGOverFSRFI=1)."));
		}
		else
		{
			UE_LOG(LogDLSS, Warning,
				TEXT("[Zonefall] FSR FI and DLSS-G are both enabled — disable one (see DLSS/ZONEFALL.md). Set r.Zonefall.Graphics.PreferDLSSGOverFSRFI=1 for auto-fix."));
		}
	}

	static void OnGraphicsPolicyCVarChanged(IConsoleVariable*)
	{
		ResolveFrameGenerationConflict();
	}

	static void RegisterGraphicsPolicyCallbacks()
	{
		const FConsoleVariableDelegate Delegate = FConsoleVariableDelegate::CreateStatic(&OnGraphicsPolicyCVarChanged);
		if (IConsoleVariable* FICVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.FidelityFX.FI.Enabled")))
		{
			FICVar->SetOnChangedCallback(Delegate);
		}
		if (IConsoleVariable* DLSSGCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Streamline.DLSSG.Enable")))
		{
			DLSSGCVar->SetOnChangedCallback(Delegate);
		}
	}

	void Register()
	{
		LogPatchVersion();
		RegisterGraphicsPolicyCallbacks();
		FCoreDelegates::OnPostEngineInit.AddLambda([]()
		{
			ResolveFrameGenerationConflict();
		});
	}
}
