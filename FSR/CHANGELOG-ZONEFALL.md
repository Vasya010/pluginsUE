# Zonefall FSR — changelog патчей

## zonefall-2 (2026-05)

- `IsPresenterUsableOnRenderThread`: проверка `RHIViewport::IsValid()` и совпадения с кэшем.
- `FCriticalSection` для `SwapChains` (game thread).
- `InvalidateRenderState` при `SetEnabled(false)`, resize, CVars FSR/FI.
- Безопасный `OnBackBufferResize` (null/valid viewport, копии указателей в lambda).
- `InitViewport`: проверки + обновление кэша; снятие делегатов в деструкторе.

## zonefall-1 (2026-05)

- Стабильность Frame Interpolation на UE 5.8: atomic-кэш presenter/viewport, `InvalidateRenderState()`.
- `GetRHIViewport()` на custom present; без `FViewportRHIRef` на render thread в hot path.
- Callback на `r.FidelityFX.FI.Enabled`, `r.FidelityFX.FI.OverrideSwapChainDX12`, `r.FidelityFX.FI.AllowAsyncWorkloads`.
- `OnBackBufferResize` сбрасывает кэш FI.
- `Config/ZonefallFSRRecommended.ini`, документация `ZONEFALL.md`.
