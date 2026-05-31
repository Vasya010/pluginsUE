# Zonefall DLSS — интеграция и обновления

| Поле | Значение |
|------|----------|
| Базовый плагин | NVIDIA DLSS **8.6.0** (NGX 310.6.0) |
| Патч Zonefall | **zonefall-2** |
| Движок | UE **5.8.0** |
| Платформа | Win64 |

## Рекомендуемые CVars

См. `Config/ZonefallDLSSRecommended.ini`.

- `r.NGX.Enable=1` — загрузка NGX / DLSS SR
- `r.Streamline.DLSSG.Enable=0` по умолчанию в Zonefall (включать на RTX 40+ осознанно)
- `r.FidelityFX.FI.Enabled=0` — не смешивать с DLSS-G
- `r.Zonefall.Graphics.PreferDLSSGOverFSRFI=1` — при включении DLSS-G автоматически выключает FSR FI

## Правки Zonefall (патч zonefall-1)

| Файл | Назначение |
|------|------------|
| `Source/DLSS/Private/ZonefallDLSSCompat.cpp` | Лог версии патча, проверка FI + DLSS-G после `PostEngineInit` |

Ядро upscaler/denoiser — без форка; при обновлении NVIDIA обычно достаточно заменить бинарники и `Source/`, **сохранив** `ZonefallDLSSCompat.cpp` и конфиги.

## Обновление с новой версии NVIDIA

1. Экспорт текущего `DLSS/` + `ZONEFALL.md`.
2. Установить новый DLSS plugin из NVIDIA / Marketplace.
3. Вернуть `ZonefallDLSSCompat.cpp`, `Config/ZonefallDLSSRecommended.ini`, `ZONEFALL.md`, `CHANGELOG-ZONEFALL.md`.
4. Проверить `DLSS.uplugin` → `Plugins` → `StreamlineNGXCommon`.
5. Сборка + тест SR и (если нужно) DLSS-G через StreamlineDLSSG.
6. Обновить `VersionName` / `ZONEFALL_DLSS_PATCH_VERSION`.

## Связанные плагины (отдельные папки)

`Streamline`, `StreamlineCore`, `StreamlineDLSSG`, `StreamlineReflex`, `NIS` — обновлять согласованно с DLSS SDK.
