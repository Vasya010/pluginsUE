# Плагины Unreal Engine — Zonefall Protocol

Репозиторий: **[github.com/Vasya010/pluginsUE](https://github.com/Vasya010/pluginsUE)**

Набор из **22 плагинов** для **Unreal Engine 5.8.0** (**Win64**), собранных для проекта **Zonefall Protocol**. Каждая подпапка (`UIWorld/`, `DLSS/`, …) — отдельный плагин с файлом `.uplugin`.

## Установка

1. Склонируйте репозиторий или скопируйте содержимое этой папки в `ВашПроект/Plugins/` (имена папок должны совпадать с таблицей ниже).
2. Включите нужные плагины в **Edit → Plugins** или в `.uproject` → `"Plugins"`.
3. Пересоберите проект (Visual Studio / Unreal Build Tool).
4. Не коммитьте `Binaries/`, `Intermediate/`, `_BuildOutput/` — они в `.gitignore`.

## Общие требования

| Параметр | Значение |
|----------|----------|
| Движок | Unreal Engine **5.8** (`EngineVersion`: **5.8.0** в каждом `.uplugin`) |
| Платформа | **Win64** |
| Целевой проект | Zonefall Protocol и совместимые UE 5.8-проекты |
| Другие версии UE | 5.7, 5.6, 5.9+ — **не поддерживаются** в текущей ветке |

Артефакты сборки (`Binaries/`, `Intermediate/`, `UIWorld/_BuildOutput/` и т.п.) исключены через `.gitignore`.

Три крупных текстуры Sky Creator (>50 MB) хранятся через **Git LFS** (см. `.gitattributes`). После клонирования: `git lfs install` и `git lfs pull`.

---

## Zonefall — собственные и интеграция

### UIWorld

| | |
|---|---|
| **Папка** | `UIWorld/` |
| **Версия** | 1.1.0 |
| **Движок** | UE 5.8.0 |
| **Автор** | Кузьменко Василий |
| **Документация** | [UIWorld/README.md](UIWorld/README.md) |

**Описание:** Модульный UI-фреймворк для меню и онлайна. Включает `UUIWorldMenuGameInstance` — центр потока экранов (главное меню, пауза, загрузка, настройки, переходы между уровнями).

**Возможности:**
- Главное меню, пауза, мастер-настроек (графика, звук, управление)
- Загрузочные экраны (обычная загрузка и online travel)
- Сохранения, toast, локализация (`ZonefallLocalizationSubsystem`)
- **Онлайн-лобби** `UZonefallOnlineLobbyWidget`: HOST, BROWSE, QUICK JOIN, LAN и Steam
- Два режима сети: **LAN** (`OnlineSubsystemNull` + IpNetDriver) и **Steam** (`OnlineSubsystemSteam`)
- Виджеты: инвентарь, HUD, создатель персонажа, intro, shader loading и др.

**Зависимости (в `.uplugin`):** EnhancedInput, OnlineSubsystem, OnlineSubsystemNull, OnlineSubsystemUtils, OnlineSubsystemSteam (опционально DLSS/Streamline для настроек графики в UI).

---

### Zonefall AI

| | |
|---|---|
| **Папка** | `ZonefallAI/` |
| **Версия** | 1.0.0 |
| **Движок** | UE 5.8.0 |
| **Автор** | Zonefall |

**Описание:** C++-фреймворк ИИ для NPC: Behavior Tree, Blackboard, восприятие, патрули, тактика и отряды.

**Возможности:**
- `ZonefallAIController`, `ZonefallAICharacter`, компоненты восприятия и памяти
- BT-декораторы и задачи: линия видимости, роли отряда, подавление, фланг, укрытие, расследование
- Патрульные пути, тактические запросы, bark, vitals, виджет статуса NPC
- `ZonefallAISquadSubsystem` — координация группы

---

### Zonefall Weather

| | |
|---|---|
| **Папка** | `ZonefallWeather/` |
| **Версия** | 1.0.0 |
| **Движок** | UE 5.8.0 |
| **Автор** | Zonefall |

**Описание:** Суточный цикл и динамическая погода в рантайме.

**Возможности:**
- `ZonefallWeatherController`, `ZonefallWeatherSubsystem`, пресеты погоды
- Солнце, луна, атмосфера, облака, туман
- Молнии (`ZonefallLightningBolt`)
- Интеграция с Sky Creator (плагин в зависимостях)

---

### Zonefall DLSS

| | |
|---|---|
| **Папка** | `DLSS/` |
| **Версия** | 8.6.0-NGX310.6.0-zonefall.2 |
| **Движок** | UE 5.8.0 |
| **Автор** | NVIDIA · интеграция: Кузьменко Василий |
| **Документация** | [DLSS/ZONEFALL.md](DLSS/ZONEFALL.md) |

**Описание:** NVIDIA DLSS Super Resolution, Ray Reconstruction (RR) и DLAA для повышения производительности и качества картинки.

**Возможности:**
- DLSS Quality/Performance/Balanced, DLAA
- Модули NGX для D3D11/D3D12/Vulkan
- Настройки в Project Settings и CVars (`r.NGX.*`)
- Патч **zonefall-2**: авто-разрешение конфликта FI+DLSS-G, `Config/ZonefallDLSSRecommended.ini`
- В Zonefall Protocol — **основной** апскейлер

**Требования:** NVIDIA GPU с поддержкой NGX, DX12, Win64.

---

### Zonefall FSR

| | |
|---|---|
| **Папка** | `FSR/` |
| **Версия** | 4.0.4-zonefall.2 |
| **Движок** | UE 5.8.0 |
| **Автор** | AMD · интеграция: Кузьменко Василий |
| **Документация** | [FSR/ZONEFALL.md](FSR/ZONEFALL.md) |

**Описание:** AMD FidelityFX Super Resolution (пространственный/временной апскейл) и FidelityFX Frame Interpolation (генерация кадров).

**Возможности:**
- FSR 3/4 temporal upscaling, настройки качества и резкости
- Frame Interpolation (`r.FidelityFX.FI.Enabled`); патч **zonefall-2** — стабильность viewport/swap-chain на UE 5.8
- D3D12 backend, optical flow, shared RHI
- `Config/ZonefallFSRRecommended.ini` — FI выключен по умолчанию для Zonefall
- **Важно:** не включать FSR FI одновременно с NVIDIA DLSS-G

**Требования:** Win64, DX12 (для полного набора функций).

---

### Zonefall DLSS Movie Render

| | |
|---|---|
| **Папка** | `DLSSMoviePipelineSupport/` |
| **Версия** | 8.6.0 |
| **Движок** | UE 5.8.0 |
| **Автор** | NVIDIA · интеграция: Кузьменко Василий |

**Описание:** Поддержка DLSS и DLAA в **Movie Render Queue** для офлайн-рендера кинематиков и трейлеров.

---

### Zonefall FSR Movie Render

| | |
|---|---|
| **Папка** | `FSRMovieRenderPipeline/` |
| **Версия** | 4.0.4 |
| **Движок** | UE 5.8.0 |
| **Автор** | AMD · интеграция: Кузьменко Василий |

**Описание:** FSR upscaling для **Movie Render Pipeline** (MRQ).

---

## Геймплей и контент (Jakub / AGLS)

### AGLS Core (HelpfulFunctions)

| | |
|---|---|
| **Папка** | `HelpfulFunctions/` |
| **Версия** | 2.0 |
| **Движок** | UE 5.8.0 |
| **Автор** | JakubW |

**Описание:** Большая библиотека Blueprint и C++ для Adventure Game Locomotion System (AGLS): персонажи, движение, climbing, traversal, IK, вспомогательный AI.

**Возможности:**
- Компоненты: climbing, ladder, matched montage, traversal, cover, fall damage
- ALS-совместимые персонажи и AI, motion warping, focusing
- Интерактивные акторы, виджеты взаимодействия, async load

---

### CombatCore

| | |
|---|---|
| **Папка** | `CombatCore/` |
| **Версия** | 1.0 |
| **Движок** | UE 5.8.0 |
| **Автор** | JakubW |

**Описание:** Боевая система для AGLS: анимации ударов, реакции на урон, базы данных комбо и расширенный **GameplayTask**.

**Возможности:**
- Модули **Runtime** и **Editor**
- `CombatAnimSetup`, редактор viewport для настройки боевых анимаций
- Databases: hit reactions, dying sequences, paired attacks
- GameplayTask blueprint и extended component

---

### Advanced AI Logic (ClimbingNavigation)

| | |
|---|---|
| **Папка** | `ClimbingNavigation/` |
| **Версия** | 1.0 |
| **Движок** | UE 5.8.0 |
| **Автор** | JakubW |

**Описание:** Навигация и AI-логика для систем лазания и укрытий в AGLS.

**Возможности:**
- ClimbNav: генерация и поиск пути по поверхностям для лазания
- Автогенерация NavLinks, EnvQuery (укрытия, точки на рёбрах, custom score)
- Улучшенный модуль восприятия AI (`AISense_BetterSight`)
- `NavClimbingComponent`, smooth path following

---

### IWALS Ability System

| | |
|---|---|
| **Папка** | `IWALS_AbilitySystem/` |
| **Версия** | 1.0 |
| **Движок** | UE 5.8.0 |
| **Автор** | JakubW |

**Описание:** Интеграция **Gameplay Ability System (GAS)** для персонажа IWALS/AGLS.

**Возможности:**
- `GAS_MainCharacterCpp`, ability sets, overlay layers
- Ability tasks: move by input, delay with tick, timer
- Взаимодействия, anim instance, attribute set

---

### JakubAnimNodes

| | |
|---|---|
| **Папка** | `JakubAnimNodes/` |
| **Версия** | 1.0 |
| **Движок** | UE 5.8.0 |
| **Автор** | JakubW |

**Описание:** Кастомные ноды **Animation Blueprint** (AnimGraph).

**Возможности:**
- Layer blending, modify layering, curve smoother, pose snapshot
- Editor tool module `JakubAnimNodesTool` для логики нод

---

### JakubCableComponent

| | |
|---|---|
| **Папка** | `JakubCableComponent/` |
| **Версия** | 1.0 |
| **Движок** | UE 5.8.0 |
| **Автор** | JakubW |

**Описание:** Модифицированная версия Epic **Cable Component** и дополнительные компоненты для IWALS (верёвки, физика, particles).

---

### GraphDebugger

| | |
|---|---|
| **Папка** | `GraphDebbuger/` |
| **Версия** | 1.0 |
| **Движок** | UE 5.8.0 |
| **Автор** | JakubW |

**Описание:** Отладка числовых данных в виде **графиков** в редакторе и игре (удобно для C++ и Blueprint).

**Возможности:**
- `GraphDebuggerCore`, custom debug draw, visualizer
- Editor commands для включения отображения

---

## Окружение и визуал

### Sky Creator

| | |
|---|---|
| **Папка** | `SkyCreatorPlugin_UE57_Release/` |
| **Версия** | 1.40.2 |
| **Движок** | UE 5.8.0 (в `.uplugin`; имя папки — релиз под UE 5.7) |
| **Автор** | Dmitry Karpukhin |

**Описание:** Продвинутое динамическое небо: volumetric-облака, атмосфера, время суток, погодные и визуальные эффекты.

**Возможности:**
- `SkyCreatorActor`, weather presets, function library
- Editor plugin для настройки в редакторе
- Shaders module `SkyCreatorShaders`

---

## NVIDIA — Streamline / DLSS-G / NIS

### NVIDIA DLSS Frame Generation (StreamlineDLSSG)

| | |
|---|---|
| **Папка** | `StreamlineDLSSG/` |
| **Версия** | 8.6.0-SL2.11.1 |
| **Движок** | UE 5.8.0 |
| **Автор** | NVIDIA |

**Описание:** **DLSS-G** (Frame Generation) и **MFG** (Multi Frame Generation) — генерация промежуточных кадров ИИ для роста FPS.

**Требования:** NVIDIA RTX 40-серии и новее, Streamline Core, Reflex (рекомендуется).

**В проекте:** `r.Streamline.DLSSG.Enable` — по умолчанию выкл. на RTX 30; не совмещать с FSR Frame Interpolation.

---

### NVIDIA Reflex (StreamlineReflex)

| | |
|---|---|
| **Папка** | `StreamlineReflex/` |
| **Версия** | 8.6.0-SL2.11.1 |
| **Движок** | UE 5.8.0 |
| **Автор** | NVIDIA |

**Описание:** **NVIDIA Reflex** — снижение системной задержки (input lag), синхронизация с рендером.

---

### NVIDIA RTX Dynamic Vibrance (StreamlineDeepDVC)

| | |
|---|---|
| **Папка** | `StreamlineDeepDVC/` |
| **Версия** | 8.6.0-SL2.11.1 |
| **Движок** | UE 5.8.0 |
| **Автор** | NVIDIA |

**Описание:** **Deep DVC** — динамическая настройка насыщенности и контраста для большей читаемости картинки.

---

### NVIDIA Streamline Core

| | |
|---|---|
| **Папка** | `StreamlineCore/` |
| **Версия** | 8.6.0-SL2.11.1 |
| **Движок** | UE 5.8.0 |
| **Автор** | NVIDIA |

**Описание:** Ядро **Streamline** — низкоуровневая интеграция с DXGI/D3D12, RHI, шейдеры. Служебный плагин для DLSS-G, Reflex и др. (в UI может быть скрыт).

---

### NVIDIA NGX / Streamline Common

| | |
|---|---|
| **Папка** | `StreamlineNGXCommon/` |
| **Версия** | 8.6.0 |
| **Движок** | UE 5.8.0 |
| **Автор** | NVIDIA |

**Описание:** Общие библиотеки и зависимости **NGX/Streamline** для остальных NVIDIA-плагинов.

---

### NVIDIA Streamline (legacy)

| | |
|---|---|
| **Папка** | `Streamline/` |
| **Версия** | 8.6.0-SL2.11.1 |
| **Движок** | UE 5.8.0 |
| **Автор** | NVIDIA |

**Описание:** Плагин совместимости для проектов, собранных на **старых** версиях Streamline (deprecated wrapper).

---

### NVIDIA Image Scaling — NIS

| | |
|---|---|
| **Папка** | `NIS/` |
| **Версия** | 8.6.0 |
| **Движок** | UE 5.8.0 |
| **Автор** | NVIDIA |

**Описание:** **NIS** — пространственный апскейлинг и sharpening на GPU (альтернатива DLSS/FSR без temporal).

**В Zonefall Protocol:** рекомендуется `r.NIS.Enable=0`, чтобы не конфликтовать с DLSS/FSR.

---

## Сводная таблица

| № | Плагин | Папка | UE | Автор |
|---|--------|-------|-----|--------|
| 1 | UIWorld | `UIWorld` | 5.8 | Кузьменко Василий |
| 2 | Zonefall AI | `ZonefallAI` | 5.8 | Zonefall |
| 3 | Zonefall Weather | `ZonefallWeather` | 5.8 | Zonefall |
| 4 | Zonefall DLSS | `DLSS` | 5.8 | NVIDIA / Zonefall |
| 5 | Zonefall FSR | `FSR` | 5.8 | AMD / Zonefall |
| 6 | Zonefall DLSS Movie Render | `DLSSMoviePipelineSupport` | 5.8 | NVIDIA / Zonefall |
| 7 | Zonefall FSR Movie Render | `FSRMovieRenderPipeline` | 5.8 | AMD / Zonefall |
| 8 | AGLS Core | `HelpfulFunctions` | 5.8 | JakubW |
| 9 | CombatCore | `CombatCore` | 5.8 | JakubW |
| 10 | Advanced AI Logic | `ClimbingNavigation` | 5.8 | JakubW |
| 11 | IWALS Ability System | `IWALS_AbilitySystem` | 5.8 | JakubW |
| 12 | JakubAnimNodes | `JakubAnimNodes` | 5.8 | JakubW |
| 13 | JakubCableComponent | `JakubCableComponent` | 5.8 | JakubW |
| 14 | GraphDebugger | `GraphDebbuger` | 5.8 | JakubW |
| 15 | Sky Creator | `SkyCreatorPlugin_UE57_Release` | 5.8 | Dmitry Karpukhin |
| 16 | NVIDIA DLSS-G | `StreamlineDLSSG` | 5.8 | NVIDIA |
| 17 | NVIDIA Reflex | `StreamlineReflex` | 5.8 | NVIDIA |
| 18 | NVIDIA Deep DVC | `StreamlineDeepDVC` | 5.8 | NVIDIA |
| 19 | Streamline Core | `StreamlineCore` | 5.8 | NVIDIA |
| 20 | NGX Common | `StreamlineNGXCommon` | 5.8 | NVIDIA |
| 21 | Streamline legacy | `Streamline` | 5.8 | NVIDIA |
| 22 | NVIDIA NIS | `NIS` | 5.8 | NVIDIA |

---

## Графика (рекомендации для Zonefall Protocol)

| Технология | Рекомендация |
|------------|----------------|
| DLSS (Super Resolution) | Основной апскейл |
| DLSS-G (Frame Gen) | Опционально (RTX 40+) |
| FSR upscaling | По настройкам |
| FSR Frame Interpolation | Не включать вместе с DLSS-G; в FSR есть правки стабильности viewport на render thread |
| NIS | Выключить при использовании DLSS/FSR |

Настройки: **Edit → Project Settings → Plugins → FSR**, CVars `r.FidelityFX.*`, `r.NGX.*`, `r.Streamline.*`.

---

## Лицензии

Перед публикацией на GitHub проверьте лицензии **NVIDIA NGX**, **AMD FSR** и сторонних авторов (JakubW, Sky Creator). Бинарники SDK могут требовать отдельного разрешения на распространение.

---

## Публикация в pluginsUE

Содержимое **этой папки** кладётся в **корень** репозитория [pluginsUE](https://github.com/Vasya010/pluginsUE) (рядом с `LICENSE`, не во вложенную `Plugins/`):

```powershell
git clone https://github.com/Vasya010/pluginsUE.git
# скопировать все папки и README.md из ZonefallProtocol\Plugins\ в клон
cd pluginsUE
git add .
git commit -m "Add UE 5.8 plugins and documentation"
git push origin main
```

Файл `.gitignore` из этой папки копируется в корень репозитория вместе с плагинами.
