// This file is part of the FSR Upscaling Unreal Engine Plugin.
//
// Copyright (c) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.

#pragma once

#include "Misc/EngineVersionComparison.h"
#ifndef UE_VERSION_AT_LEAST
#define UE_VERSION_AT_LEAST(MajorVersion, MinorVersion, PatchVersion) \
	UE_VERSION_NEWER_THAN_OR_EQUAL(MajorVersion, MinorVersion, PatchVersion)
#endif

#if UE_VERSION_AT_LEAST(5, 8, 0)

#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "Slate/SceneViewport.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"

inline FRHIViewport* FFXGetViewportRHIRaw(FViewport* Viewport)
{
	if (!Viewport || !FSlateApplication::IsInitialized())
	{
		return nullptr;
	}

	if (FSceneViewport* SceneViewport = static_cast<FSceneViewport*>(Viewport))
	{
		if (TSharedPtr<SWindow> Window = SceneViewport->FindWindow())
		{
			if (FSlateRenderer* SlateRenderer = FSlateApplication::Get().GetRenderer())
			{
				return static_cast<FRHIViewport*>(SlateRenderer->GetViewportResource(*Window));
			}
		}
	}

	return nullptr;
}

inline FViewportRHIRef FFXGetViewportRHI(FViewport* Viewport)
{
	return FViewportRHIRef(FFXGetViewportRHIRaw(Viewport));
}

inline FRHIViewport* FFXGetWindowViewportRHIRaw(SWindow& SlateWindow)
{
	if (FSlateRenderer* SlateRenderer = FSlateApplication::Get().GetRenderer())
	{
		return static_cast<FRHIViewport*>(SlateRenderer->GetViewportResource(SlateWindow));
	}
	return nullptr;
}

inline FViewportRHIRef FFXGetWindowViewportRHI(SWindow& SlateWindow)
{
	return FViewportRHIRef(FFXGetWindowViewportRHIRaw(SlateWindow));
}

inline bool FFXIsGameViewportRHIReady()
{
	return FFXGetViewportRHIRaw(GEngine && GEngine->GameViewport ? GEngine->GameViewport->Viewport : nullptr) != nullptr;
}

#define FFX_USE_INVERTED_DEPTH 1

#else

#define FFX_USE_INVERTED_DEPTH bool(ERHIZBuffer::IsInverted)

#endif
