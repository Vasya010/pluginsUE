#include "ZonefallShaderCacheSubsystem.h"

#include "HAL/IConsoleManager.h"
#include "Misc/App.h"
#include "Misc/CoreDelegates.h"
#include "PipelineFileCache.h"
#include "RHI.h"
#include "ShaderPipelineCache.h"

DEFINE_LOG_CATEGORY_STATIC(LogZonefallShaderCache, Log, All);

namespace
{
	void SetCVarSafe(const TCHAR* Name, int32 Value)
	{
		if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(Name))
		{
			CVar->Set(Value, ECVF_SetByCode);
		}
	}

	void SetCVarSafe(const TCHAR* Name, float Value)
	{
		if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(Name))
		{
			CVar->Set(Value, ECVF_SetByCode);
		}
	}
}

void UZonefallShaderCacheSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (CacheKey.IsEmpty())
	{
		CacheKey = FApp::GetProjectName();
	}

	if (bApplyAntiStutterCVars)
	{
		ApplyAntiStutterCVars();
	}

	OpenPipelineCache();

	PostShutdownHandle = FCoreDelegates::OnExit.AddUObject(this, &UZonefallShaderCacheSubsystem::HandleExitRequested);

	LastAutoSaveTime = FPlatformTime::Seconds();
}

void UZonefallShaderCacheSubsystem::Deinitialize()
{
	if (bAutoSaveOnShutdown)
	{
		SaveUsageCacheNow();
	}

	if (PostShutdownHandle.IsValid())
	{
		FCoreDelegates::OnExit.Remove(PostShutdownHandle);
		PostShutdownHandle.Reset();
	}

	FShaderPipelineCache::CloseUserPipelineFileCache();

	Super::Deinitialize();
}

void UZonefallShaderCacheSubsystem::ApplyAntiStutterCVars() const
{
	// Compile materials immediately on load instead of when first rendered.
	SetCVarSafe(TEXT("r.CreateShadersOnLoad"), 1);

	// Allow PSO precaching to actually do work.
	SetCVarSafe(TEXT("r.PSOPrecaching"), 1);
	SetCVarSafe(TEXT("r.PSOPrecache.GlobalShaders"), 1);
	SetCVarSafe(TEXT("r.PSOPrecache.Validation"), 0);
	SetCVarSafe(TEXT("r.PSOPrecache.ProxyMaterials"), 1);
	SetCVarSafe(TEXT("r.PSOPrecache.PruneOldPSOs"), 1);

	// Block draws on missing PSO (forces compile before render).
	SetCVarSafe(TEXT("r.PSOPrecaching.PreventDrawMaterialPSO"), 1);

	// Allow multi-threaded shader compilation when possible.
	SetCVarSafe(TEXT("r.Shaders.AllowCompilingThroughWorkers"), 1);

	// Drives FShaderPipelineCache batch behavior.
	SetCVarSafe(TEXT("r.ShaderPipelineCache.Enabled"), 1);
	SetCVarSafe(TEXT("r.ShaderPipelineCache.StartupMode"), 1);
	SetCVarSafe(TEXT("r.ShaderPipelineCache.LogPSO"), 1);
	SetCVarSafe(TEXT("r.ShaderPipelineCache.SaveAfterPSOsLogged"), 256);
	SetCVarSafe(TEXT("r.ShaderPipelineCache.BatchSize"), static_cast<int32>(BatchSizePerFrame));
	SetCVarSafe(TEXT("r.ShaderPipelineCache.LazyLoadShadersWhenPSOCacheIsPresent"), 1);

	UE_LOG(LogZonefallShaderCache, Log, TEXT("Applied anti-stutter CVars (PSO precaching enabled)."));
}

void UZonefallShaderCacheSubsystem::OpenPipelineCache()
{
	if (FShaderPipelineCache::IsBatchingPaused())
	{
		FShaderPipelineCache::ResumeBatching();
	}

	FShaderPipelineCache::SetBatchMode(FShaderPipelineCache::BatchMode::Fast);

	const bool bBundledOpened = FShaderPipelineCache::OpenPipelineFileCache(GMaxRHIShaderPlatform);
	const bool bUserOpened = FShaderPipelineCache::OpenUserPipelineFileCache(GMaxRHIShaderPlatform);
	UE_LOG(LogZonefallShaderCache, Log, TEXT("OpenPipelineFileCache(platform=%d) bundled=%s user=%s"),
		static_cast<int32>(GMaxRHIShaderPlatform),
		bBundledOpened ? TEXT("OK") : TEXT("none"),
		bUserOpened ? TEXT("OK") : TEXT("none"));

	InitialPSOCount = static_cast<int32>(FShaderPipelineCache::NumPrecompilesRemaining());
	if (InitialPSOCount > 0)
	{
		UE_LOG(LogZonefallShaderCache, Log, TEXT("PSO precompile starting with %d entries"), InitialPSOCount);
	}
}

void UZonefallShaderCacheSubsystem::HandleExitRequested()
{
	if (bAutoSaveOnShutdown)
	{
		SaveUsageCacheNow();
	}
}

bool UZonefallShaderCacheSubsystem::SaveUsageCacheNow()
{
	const bool bSaved = FShaderPipelineCache::SavePipelineFileCache(FPipelineFileCacheManager::SaveMode::Incremental);
	UE_LOG(LogZonefallShaderCache, Log, TEXT("SaveUsageCacheNow -> %s"), bSaved ? TEXT("OK") : TEXT("no-op"));
	LastAutoSaveTime = FPlatformTime::Seconds();
	return bSaved;
}

bool UZonefallShaderCacheSubsystem::IsPrecompiling() const
{
	return FShaderPipelineCache::IsPrecompiling();
}

float UZonefallShaderCacheSubsystem::GetPrecompileProgress() const
{
	if (InitialPSOCount <= 0)
	{
		return 1.0f;
	}
	const int32 Remaining = static_cast<int32>(FShaderPipelineCache::NumPrecompilesRemaining());
	return FMath::Clamp(1.0f - (static_cast<float>(Remaining) / static_cast<float>(InitialPSOCount)), 0.0f, 1.0f);
}

int32 UZonefallShaderCacheSubsystem::GetNumPSOsRemaining() const
{
	return static_cast<int32>(FShaderPipelineCache::NumPrecompilesRemaining());
}

int32 UZonefallShaderCacheSubsystem::GetInitialPSOCount() const
{
	return InitialPSOCount;
}
