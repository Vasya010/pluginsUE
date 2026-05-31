// This file is part of the FSR Upscaling Unreal Engine Plugin.
//
// Copyright (c) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#pragma once

#include "Engine/Engine.h"
#include "SceneViewExtension.h"
#include "Misc/EngineVersionComparison.h"
#ifndef UE_VERSION_AT_LEAST
#define UE_VERSION_AT_LEAST(MajorVersion, MinorVersion, PatchVersion) \
	UE_VERSION_NEWER_THAN_OR_EQUAL(MajorVersion, MinorVersion, PatchVersion)
#endif
#if UE_VERSION_AT_LEAST(5, 8, 0)
#include <atomic>
#endif

#include "IFFXFrameInterpolation.h"

//-------------------------------------------------------------------------------------
// Forward declarations.
//-------------------------------------------------------------------------------------
class FRDGBuilder;
class SWindow;
class FViewport;
class FFXFrameInterpolationViewExtension;
class FFXFrameInterpolationCustomPresent;
struct FfxFrameInterpolationContext;

//-------------------------------------------------------------------------------------
// Class that actually implements frame interpolation.
//-------------------------------------------------------------------------------------
class FFXFrameInterpolation : public IFFXFrameInterpolation
{
public:
	FFXFrameInterpolation();
	virtual ~FFXFrameInterpolation();

	void OnPostEngineInit();
	void OnViewportCreatedHandler_SetCustomPresent();
	void OnBeginDrawHandler();
    void SetupView(const FSceneView& View, const FPostProcessingInputs& Inputs);
	void InterpolateFrame(FRDGBuilder& GraphBuilder);
#if UE_VERSION_AT_LEAST(5, 8, 0)
	void OnSlateWindowRendered(SWindow& SlateWindow);
	void OnBackBufferReadyToPresentCallback(class SWindow& SlateWindow, class ISlateViewportProvider& ViewportProvider);
#else
	void OnSlateWindowRendered(SWindow& SlateWindow, void* ViewportRHIPtr);
#if UE_VERSION_AT_LEAST(5, 5, 0)
	void OnBackBufferReadyToPresentCallback(class SWindow& SlateWindow, const FTextureRHIRef& BackBuffer);
#else
	void OnBackBufferReadyToPresentCallback(class SWindow& SlateWindow, const FTexture2DRHIRef& BackBuffer);
#endif
#endif

	IFFXFrameInterpolationCustomPresent* CreateCustomPresent(IFFXSharedBackend* Backend, uint32_t Flags, FIntPoint RenderSize, FIntPoint DisplaySize, FfxSwapchain RawSwapChain, FfxCommandQueue Queue, FfxApiSurfaceFormat Format, EFFXBackendAPI Api) final;
	bool GetAverageFrameTimes(float& AvgTimeMs, float& AvgFPS) final;
	void InvalidateRenderState();

private:
	struct FFXFrameInterpolationView
	{
		FRDGTextureRef ViewFamilyTexture;
		FRDGTextureRef SceneDepth;
		FRDGTextureRef SceneVelocity;
		FIntRect ViewRect;
		FIntPoint InputExtentsQuantized;
		FIntPoint OutputExtents;
		FVector2D TemporalJitterPixels;
		float CameraNear;
		float CameraFOV;
		float GameTimeMs;
		bool bReset;
		bool bEnabled;
	};
	void CalculateFPSTimings();
	bool InterpolateView(FRDGBuilder& GraphBuilder, FFXFrameInterpolationCustomPresent* Presenter, const FSceneView* View, FFXFrameInterpolationView const& ViewDesc, FRDGTextureRef FinalBuffer, FRDGTextureRef InterpolatedRDG, FRDGTextureRef BackBufferRDG, uint32 Index);
	TMap<const FSceneView*, FFXFrameInterpolationView> Views;
	TSharedPtr<FFXFrameInterpolationViewExtension, ESPMode::ThreadSafe> ViewExtension;
    TRefCountPtr<IPooledRenderTarget> BackBufferRT;
	TRefCountPtr<IPooledRenderTarget> InterpolatedRT;
	TRefCountPtr<IPooledRenderTarget> AsyncBufferRT[2];
	TMap<FfxSwapchain, FFXFrameInterpolationCustomPresent*> SwapChains;
	TMap<SWindow*, FRHIViewport*> Windows;
	float GameDeltaTime;
	double LastTime;
	float AverageTime;
	float AverageFPS;
	uint64 InterpolationCount;
	uint64 PresentCount;
	uint32 Index;
	uint32 ResetState;
	bool bInterpolatedFrame;
#if UE_VERSION_AT_LEAST(5, 8, 0)
	std::atomic<FRHIViewport*> CachedGameViewportRHI;
	std::atomic<FFXFrameInterpolationCustomPresent*> CachedFrameInterpolationPresenter;
	std::atomic<int32> CachedViewportSizeX;
	std::atomic<int32> CachedViewportSizeY;

	static bool IsSafePresenterPointer(const FFXFrameInterpolationCustomPresent* Presenter);
	bool IsKnownFrameInterpolationPresenter(const FFXFrameInterpolationCustomPresent* Presenter) const;
	void ClearCachedGameViewportState();
	void UpdateCachedGameViewportState(FViewport* Viewport, FRHIViewport* ViewportRHI = nullptr);
	FRHIViewport* GetActiveGameViewportRHIRenderThread() const;
	FFXFrameInterpolationCustomPresent* GetFrameInterpolationPresenterRenderThread() const;
	FIntPoint GetCachedViewportSizeRenderThread() const;
#endif
};
