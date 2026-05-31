# UIWorld — плагин UI для Unreal Engine 5.8 (Zonefall Protocol)

Модульный UI-фреймворк с готовыми меню, загрузочными экранами и **онлайн-лобби** (LAN + Steam). Управление потоком экранов через `UUIWorldMenuGameInstance`.

**Разработчик:** Кузьменко Василий

---

## Что уже есть

### Меню и поток игры

| Модуль | Описание |
|--------|----------|
| **Главное меню** | `ZonefallMainMenuWidget` — переход в одиночную игру, онлайн, настройки |
| **Пауза** | `ZonefallPauseMenuWidget` — продолжить, настройки, выход |
| **Настройки** | `ZonefallMasterSettingsWidget` — графика, звук, управление (glass UI) |
| **Загрузка** | `ZonefallLoadingScreenWidget` — обычная загрузка уровня + **отдельный режим онлайн-travel** |
| **Старт игры** | Intro + компиляция шейдеров (`ZonefallStartupIntroWidget`, `ZonefallShaderLoadingWidget`) |
| **Сохранения** | Toast «сохранено», `UIWorldSaveManager` |
| **Локализация** | `ZonefallLocalizationSubsystem` |

`UUIWorldMenuGameInstance` (или Blueprint-наследник, например `GameinstanceP`) — центр логики: какой экран показать, когда грузить уровень, фокус ввода после загрузки.

### Онлайн (реализовано)

Два **раздельных** режима (не смешиваются драйверы):

| Режим | OSS | Net Driver | Когда использовать |
|-------|-----|------------|-------------------|
| **LOCAL LAN** | `OnlineSubsystemNull` | `LanGameNetDriver` (Ip) | Один ПК (PIE), локальная сеть, **Steam не нужен** |
| **Steam / Internet** | `OnlineSubsystemSteam` | `GameNetDriver` (SteamSockets) | Друзья через Steam, интернет |

#### Лобби `UZonefallOnlineLobbyWidget`

Полностью собирается в C++ (Blueprint не обязателен):

- **HOST** — имя сервера, карта, 2–32 игрока, LOCAL LAN, приватность (Public / Friends / Invite Only), пароль хоста
- **QUICK JOIN** — поиск лучшей открытой сессии (минимальный пинг, тот же `BuildId`, без пароля)
- **BROWSE** — список сессий с пингом, игроками, картой, метками FULL / LOCK / VER
- **REFRESH**, фильтр **Hide full**, **JOIN**, **LEAVE**, **BACK**
- **DIRECT CONNECT** — `127.0.0.1:7777`, Steam URL и т.д.

#### `UUIWorldMenuGameInstance` — API онлайн

- `HostOnlineSession`, `FindOnlineSessions`, `JoinOnlineSessionByIndex`, `QuickJoinOnlineSession`
- `JoinOnlineByAddress`, `LeaveOnlineSessionAndReturnToMenu`
- `GetLastFoundSessions`, `GetLastOnlineDiagnostic`, `GetOnlinePlayerNickname`
- События: `OnHostCompleted`, `OnSessionsFound`, `OnJoinCompleted`, `OnOnlineMatchReady`
- Защита от зависания: таймаут join/host, возврат в меню при `TravelFailure` / `NetworkFailure`
- Версия билда: `OnlineGameBuildId` + `BUILD_ID` в настройках сессии (фильтр несовместимых клиентов)

#### Конфиг проекта (`DefaultEngine.ini`)

В проекте, который использует плагин, должны быть (пример в Zonefallprotocol):

```ini
[OnlineSubsystem]
DefaultPlatformService=Steam

[OnlineSubsystemNull]
bEnabled=true

[/Script/Engine.GameEngine]
+NetDriverDefinitions=(DefName="LanGameNetDriver",DriverClassName="/Script/OnlineSubsystemUtils.IpNetDriver")
+NetDriverDefinitions=(DefName="GameNetDriver",DriverClassName="/Script/SteamSockets.SteamSocketsNetDriver")
```

LAN-хост: `?listen&NetDriver=LanGameNetDriver`  
Steam-хост: `?listen&NetDriver=GameNetDriver`

### Геймплей: HUD, сохранения, оружие

| Система | Где | Что делает |
|---------|-----|------------|
| **HUD «карточка героя»** | `ZonefallHUDWidget` | Справа снизу — богатая стеклянная карточка: **живой портрет персонажа** (он реально рендерится), полоса **здоровья** рядом с оружием, полоса стамины/Dead-Eye, имя оружия и патроны. Слева снизу — миникарта. |
| **Живой портрет** | `AZonefallCharacterPortraitCapture` | `SceneCapture2D` спереди от героя со show-only списком (рисуется только игрок) → render target в карточке. Спавнится автоматически. |
| **Миникарта** | `AZonefallMinimapCapture` | Ортокамера сверху. `OrthoWidth` уменьшен до 3600 — ближе к персонажу (стиль RDR2). Двойное кольцо + компас «N» + вращающийся блип. |
| **Колесо оружия** | `ZonefallWeaponWheelWidget` | Слоты-чипы с **цветовым кодом по классу** оружия, патроны в каждом слоте, центральный хаб: класс + название + патроны выбранного. |
| **Реальные сохранения** | `UUIWorldSaveGame` / `UUIWorldSaveManager` | Сохраняются здоровье, оружие (+патроны в обойме и запасе), экипированный слот, **подобранные предметы инвентаря** и позиция/поворот. |

**Поток сохранения / «Продолжить»:**

- В игре (пауза → Save или `GI->SaveGame()`) снимается полный снимок локального игрока.
- В главном меню **CONTINUE** → `GI->ContinueSavedGame()` грузит сохранённый уровень и помечает восстановление; заспавненный игрок в `BeginPlay` (на сервере/standalone) сам применяет снимок.
- ⚠️ Все поля сохранения помечены `UPROPERTY(SaveGame)` — без этого флага архив `ArIsSaveGame` ничего не пишет (старая версия сохраняла фактически пустой файл).

**Реальные модели оружия (без кода):** заполните `DefaultLoadout` на `UZonefallWeaponInventoryComponent` (в Blueprint-наследнике персонажа или в Details), назначив настоящие `WeaponMesh` и смещения в руке. Если массив пуст — подставляется заглушка из примитивов движка.

### Прочее

- Достижения (Steam + локальный fallback)
- Адаптивные задержки загрузочного экрана
- Поддержка Blueprint и C++

---

## Быстрый старт

1. Скопировать плагин в `YourProject/Plugins/UIWorld/`
2. Включить: **Edit → Plugins → UIWorld**
3. В **Project Settings → Maps & Modes** указать Game Instance на класс, наследующий `UUIWorldMenuGameInstance`
4. (Опционально) `OnlineMenuWidgetClass` = `UZonefallOnlineLobbyWidget` — если не задан, подставляется по умолчанию
5. Собрать C++ проект

### PIE: LAN на одном ПК (2 игрока)

1. **Play → Number of Players: 2**, **Net Mode: Play As Listen Server**
2. **Окно Player 1:** ONLINE → LOCAL LAN → **HOST** → дождаться загрузки карты (`Levelgames`)
3. **Окно Player 2:** LOCAL LAN → **REFRESH** или **CONNECT** `127.0.0.1:7777`

В логе хоста ожидается: `World->Listen ... OK`, `LAN listen server ready`.

---

## Структура плагина

```
UIWorld/
├── Source/UIWorld/
│   ├── Public/
│   │   ├── UIWorldMenuGameInstance.h
│   │   └── UI/
│   │       ├── ZonefallOnlineLobbyWidget.h
│   │       ├── ZonefallLoadingScreenWidget.h
│   │       └── ...
│   └── Private/
├── Config/          # при необходимости — шаблоны ini
├── Resources/
└── UIWorld.uplugin
```

---

## Настройки Game Instance (онлайн)

| Свойство | Назначение |
|----------|------------|
| `OnlineHostMapName` | Карта для хоста (например `Levelgames`) |
| `OnlineServerName` | Имя в списке сессий |
| `OnlineLanPort` | Порт LAN (по умолчанию `7777`) |
| `OnlineGameBuildId` | Должен совпадать с `BuildIdOverride` в ini |
| `MainMenuLevelName` | Куда возвращаться при ошибке / Leave |
| `OnlineJoinTimeoutSeconds` | Таймаут подключения |

---

## Roadmap — что добавим позже

Планируется развивать онлайн в сторону **AAA / GTA Online-подобного** опыта. Сейчас это **listen-server** (хост = игрок), не dedicated.

### Онлайн — ближайшие планы

- [ ] Инвайты друзей Steam из лобби (без ручного IP)
- [ ] Голосовой чат / текстовый чат в лобби
- [ ] Kick / Ban, передача хоста (host migration)
- [ ] Ready-check перед стартом матча («все готовы»)
- [ ] Фильтры браузера: регион, режим, только друзья
- [ ] Dedicated server (отдельный билд без UI)
- [ ] Matchmaking / очередь (не только ручной browse)
- [ ] EOS / кроссплей (если понадобится выход за Steam)

### UI — общие планы

- [ ] Система тем (Dark / Light / кастомные токены)
- [ ] Расширенные анимации переходов между экранами
- [ ] Сохранение состояния UI (последняя вкладка настроек и т.д.)
- [ ] Drag & Drop конструктор раскладки (опционально)

### Уже в roadmap (из старых заметок)

- [ ] UI Animations System
- [ ] Theme System
- [ ] Multiplayer UI Sync (синхронизация виджетов между клиентами)

---

## Ограничения текущей версии

- **Нет dedicated server** — только listen-server на машине хоста.
- **Steam Dev AppId 480** (Spacewar) в dev-сборках — для релиза нужен свой App ID.
- **PIE:** хост обязательно из окна **Player 1** при «Play As Listen Server»; одиночное окно PIE не заменяет второго клиента.
- Пароль сессии проверяется на клиенте при join; для production нужна серверная валидация в GameMode.
- Некоторые интеграции (DLSS / Streamline в UIWorld) могут быть отключены в конкретном проекте ради стабильности сборки.

---

## Требования

- Unreal Engine **5.7+** (в README проекта может быть указана 5.8 — проверьте `UIWorld.uplugin`)
- C++ проект (рекомендуется)
- Для Steam-онлайн: Steam client, плагин OnlineSubsystemSteam, SteamSockets

---

## Установка

```bash
git clone https://github.com/Vasya010/UIWorld-plugin-ue5.7.git
```

Положить в `YourProject/Plugins/UIWorld/`, включить плагин, пересобрать.

---

## Лицензия

MIT License

## Поддержка

Issues на GitHub — баги и предложения по функциям из roadmap.
