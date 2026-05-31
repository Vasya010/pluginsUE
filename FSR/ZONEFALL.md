# Zonefall FSR — интеграция и обновления

| Поле | Значение |
|------|----------|
| Базовый плагин | AMD FSR **4.0.4** |
| Патч Zonefall | **zonefall-2** |
| Движок | UE **5.8.0** |
| Платформа | Win64 |

## Рекомендуемые CVars (Zonefall Protocol)

См. `Config/ZonefallFSRRecommended.ini`. Кратко:

- `r.FidelityFX.FSR.Enabled=1` — апскейл FSR
- `r.FidelityFX.FI.Enabled=0` — **Frame Interpolation выключен** (основной путь — DLSS / DLSS-G)
- Не включать FI вместе с `r.Streamline.DLSSG.Enable`

## Правки Zonefall (патч zonefall-2)

См. также **zonefall-1** в `CHANGELOG-ZONEFALL.md`.

Искать в коде маркер `ZONEFALL_PATCH`.

| Проблема | Решение |
|----------|---------|
| AV при смене настроек FI / swap chain | Кэш presenter/viewport на atomic, без `FViewportRHIRef` на render thread |
| Устаревший `FRHIViewport*` в `CalculateFPSTimings` | Чтение viewport через `Presenter->GetRHIViewport()` + `IsValid()` |
| Смена `r.FidelityFX.FI.*` | `InvalidateRenderState()` + callback на CVar |
| Resize swap chain | `OnBackBufferResize` → `InvalidateRenderState()` |

Затронутые файлы: `FFXFrameInterpolation.cpp`, `.h`, `FFXFrameInterpolationCustomPresent.cpp`, `.h`.

## Обновление с новой версии AMD

1. Сохранить копию папки `FSR/` и этот `ZONEFALL.md`.
2. Подложить новый релиз AMD в отдельную ветку/папку.
3. Смержить **вручную** файлы из таблицы выше (по маркерам `ZONEFALL_PATCH`).
4. Пересобрать модуль `FFXFrameInterpolation`, прогнать PIE с FI=0 и (опционально) FI=1 без DLSS-G.
5. Обновить `VersionName` в `FSR.uplugin` и номер патча в `ZonefallFFXCompat.h`.
6. Записать изменения в `CHANGELOG-ZONEFALL.md`.

## Зависимости

- Streamline-плагины — отдельно, для DLSS-G в проекте Zonefall.
- Одновременно **FSR FI** и **DLSS-G** не использовать.
