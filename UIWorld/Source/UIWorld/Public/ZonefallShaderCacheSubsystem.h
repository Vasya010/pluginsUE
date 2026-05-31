#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "ZonefallShaderCacheSubsystem.generated.h"

/**
 * Drives Unreal's pipeline state cache (PSO) so the game pre-compiles and re-uses
 * shader pipelines across launches. This is the real anti-stutter solution:
 *  - On first launch the cache records what pipelines the game uses.
 *  - On every subsequent launch those pipelines are compiled up front during the
 *    shader loading screen, eliminating runtime hitches when new materials appear.
 *
 * Everything is automatic — game just needs UIWorld plugin enabled.
 */
UCLASS()
class UIWORLD_API UZonefallShaderCacheSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Triggers an explicit save of the runtime usage cache to disk. */
	UFUNCTION(BlueprintCallable, Category = "Zonefall|Shader Cache")
	bool SaveUsageCacheNow();

	/** Returns true if the PSO cache is currently precompiling pipelines. */
	UFUNCTION(BlueprintPure, Category = "Zonefall|Shader Cache")
	bool IsPrecompiling() const;

	/** Returns 0..1 progress of PSO precompile (1.0 means done or no cache). */
	UFUNCTION(BlueprintPure, Category = "Zonefall|Shader Cache")
	float GetPrecompileProgress() const;

	/** How many PSOs are still being compiled. */
	UFUNCTION(BlueprintPure, Category = "Zonefall|Shader Cache")
	int32 GetNumPSOsRemaining() const;

	/** Total PSOs in the cache that this subsystem began with. */
	UFUNCTION(BlueprintPure, Category = "Zonefall|Shader Cache")
	int32 GetInitialPSOCount() const;

	/** Game name used to scope the cache file. Defaults to FApp::GetProjectName(). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Shader Cache")
	FString CacheKey;

	/** Number of pipelines to precompile per frame during warmup. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Shader Cache", meta = (ClampMin = "1", ClampMax = "200"))
	int32 BatchSizePerFrame = 60;

	/** Auto-save cache on application shutdown. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Shader Cache")
	bool bAutoSaveOnShutdown = true;

	/** Auto-save cache periodically while playing (every N seconds). 0 disables. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Shader Cache", meta = (ClampMin = "0.0"))
	float AutoSaveIntervalSeconds = 180.0f;

	/** If true, applies recommended r.* CVars on startup that improve shader robustness. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Shader Cache")
	bool bApplyAntiStutterCVars = true;

private:
	int32 InitialPSOCount = 0;
	double LastAutoSaveTime = 0.0;
	FDelegateHandle PostShutdownHandle;

	void ApplyAntiStutterCVars() const;
	void OpenPipelineCache();
	void HandleExitRequested();
};
