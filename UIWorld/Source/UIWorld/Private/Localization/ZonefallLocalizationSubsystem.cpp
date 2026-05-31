#include "Localization/ZonefallLocalizationSubsystem.h"

#include "Engine/GameInstance.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/Culture.h"
#include "Misc/ConfigCacheIni.h"

void UZonefallLocalizationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	BuildDictionary();
	LoadSavedLanguage();

	// Apply the saved culture on boot so engine text matches the chosen language.
	FInternationalization::Get().SetCurrentCulture(CultureFor(CurrentLanguage));
}

UZonefallLocalizationSubsystem* UZonefallLocalizationSubsystem::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}
	if (const UWorld* World = WorldContextObject->GetWorld())
	{
		if (UGameInstance* GI = World->GetGameInstance())
		{
			return GI->GetSubsystem<UZonefallLocalizationSubsystem>();
		}
	}
	return nullptr;
}

FText UZonefallLocalizationSubsystem::L(const UObject* WorldContextObject, FName Key)
{
	if (UZonefallLocalizationSubsystem* Loc = Get(WorldContextObject))
	{
		return Loc->GetText(Key);
	}
	return FText::FromName(Key);
}

FString UZonefallLocalizationSubsystem::CultureFor(EZonefallLanguage Language)
{
	switch (Language)
	{
	case EZonefallLanguage::Russian: return TEXT("ru");
	case EZonefallLanguage::Chinese: return TEXT("zh-Hans");
	case EZonefallLanguage::English:
	default:                         return TEXT("en");
	}
}

EZonefallLanguage UZonefallLocalizationSubsystem::LanguageFromCulture(const FString& Culture)
{
	if (Culture.StartsWith(TEXT("ru"))) return EZonefallLanguage::Russian;
	if (Culture.StartsWith(TEXT("zh"))) return EZonefallLanguage::Chinese;
	return EZonefallLanguage::English;
}

TArray<FString> UZonefallLocalizationSubsystem::GetLanguageDisplayNames() const
{
	// Each language shown in its own script so it's recognisable whatever the current language.
	return { TEXT("English"), TEXT("Русский"), TEXT("中文") };
}

void UZonefallLocalizationSubsystem::SetLanguage(EZonefallLanguage NewLanguage)
{
	CurrentLanguage = NewLanguage;
	FInternationalization::Get().SetCurrentCulture(CultureFor(NewLanguage));
	SaveLanguage();
	OnLanguageChanged.Broadcast(CurrentLanguage);
}

FText UZonefallLocalizationSubsystem::GetText(FName Key) const
{
	if (const TArray<FString>* Entry = Dictionary.Find(Key))
	{
		const int32 Index = (int32)CurrentLanguage;
		if (Entry->IsValidIndex(Index) && !(*Entry)[Index].IsEmpty())
		{
			return FText::FromString((*Entry)[Index]);
		}
		// Fall back to English.
		if (Entry->IsValidIndex(0) && !(*Entry)[0].IsEmpty())
		{
			return FText::FromString((*Entry)[0]);
		}
	}
	// Last resort: the raw key, so nothing ever renders blank.
	return FText::FromName(Key);
}

void UZonefallLocalizationSubsystem::LoadSavedLanguage()
{
	FString SavedCulture;
	if (GConfig && GConfig->GetString(TEXT("ZonefallLocalization"), TEXT("Language"), SavedCulture, GGameUserSettingsIni)
		&& !SavedCulture.IsEmpty())
	{
		CurrentLanguage = LanguageFromCulture(SavedCulture);
		return;
	}

	// No saved choice — follow the OS/editor culture for a sensible first run.
	CurrentLanguage = LanguageFromCulture(FInternationalization::Get().GetCurrentCulture()->GetName());
}

void UZonefallLocalizationSubsystem::SaveLanguage() const
{
	if (GConfig)
	{
		GConfig->SetString(TEXT("ZonefallLocalization"), TEXT("Language"), *CultureFor(CurrentLanguage), GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
	}
}

void UZonefallLocalizationSubsystem::BuildDictionary()
{
	Dictionary.Reset();

	// Add(Key, English, Russian, Chinese).
	auto Add = [this](const TCHAR* Key, const TCHAR* En, const TCHAR* Ru, const TCHAR* Zh)
	{
		Dictionary.Add(FName(Key), TArray<FString>{ FString(En), FString(Ru), FString(Zh) });
	};

	// --- Common ---
	Add(TEXT("common.back"),    TEXT("Back"),    TEXT("Назад"),       TEXT("返回"));
	Add(TEXT("common.apply"),   TEXT("Apply"),   TEXT("Применить"),   TEXT("应用"));
	Add(TEXT("common.cancel"),  TEXT("Cancel"),  TEXT("Отмена"),      TEXT("取消"));
	Add(TEXT("common.confirm"), TEXT("Confirm"), TEXT("Подтвердить"), TEXT("确认"));
	Add(TEXT("common.close"),   TEXT("Close"),   TEXT("Закрыть"),     TEXT("关闭"));
	Add(TEXT("common.on"),      TEXT("On"),      TEXT("Вкл"),         TEXT("开"));
	Add(TEXT("common.off"),     TEXT("Off"),     TEXT("Выкл"),        TEXT("关"));
	Add(TEXT("common.default"), TEXT("Default"), TEXT("По умолчанию"),TEXT("默认"));

	// --- Main menu tabs / actions ---
	Add(TEXT("menu.home"),     TEXT("HOME"),       TEXT("ГЛАВНАЯ"),       TEXT("主页"));
	Add(TEXT("menu.story"),    TEXT("STORY"),      TEXT("СЮЖЕТ"),         TEXT("剧情"));
	Add(TEXT("menu.online"),   TEXT("ONLINE"),     TEXT("СЕТЕВАЯ ИГРА"),  TEXT("在线"));
	Add(TEXT("menu.whatsnew"), TEXT("WHAT'S NEW"), TEXT("ЧТО НОВОГО"),    TEXT("最新动态"));
	Add(TEXT("menu.dlc"),      TEXT("DLC"),        TEXT("ДОПОЛНЕНИЯ"),    TEXT("追加内容"));
	Add(TEXT("menu.settings"), TEXT("SETTINGS"),   TEXT("НАСТРОЙКИ"),     TEXT("设置"));
	Add(TEXT("menu.play"),     TEXT("PLAY"),       TEXT("ИГРАТЬ"),        TEXT("开始游戏"));
	Add(TEXT("menu.quit"),     TEXT("QUIT"),       TEXT("ВЫХОД"),         TEXT("退出"));

	// --- Online status / account (main-menu indicator) ---
	Add(TEXT("online.status.online"),  TEXT("ONLINE"),  TEXT("В СЕТИ"),     TEXT("在线"));
	Add(TEXT("online.status.offline"), TEXT("OFFLINE"), TEXT("НЕ В СЕТИ"),  TEXT("离线"));
	Add(TEXT("online.account"),        TEXT("Account"), TEXT("Аккаунт"),    TEXT("账户"));
	Add(TEXT("online.players"),        TEXT("Players"), TEXT("Игроки"),     TEXT("玩家"));

	// --- Online lobby ---
	Add(TEXT("online.title"),      TEXT("ONLINE"),       TEXT("СЕТЕВАЯ ИГРА"),  TEXT("在线游戏"));
	Add(TEXT("online.host"),       TEXT("HOST"),         TEXT("СОЗДАТЬ"),       TEXT("创建"));
	Add(TEXT("online.join"),       TEXT("JOIN"),         TEXT("ВОЙТИ"),         TEXT("加入"));
	Add(TEXT("online.refresh"),    TEXT("REFRESH"),      TEXT("ОБНОВИТЬ"),      TEXT("刷新"));
	Add(TEXT("online.leave"),      TEXT("LEAVE"),        TEXT("ПОКИНУТЬ"),      TEXT("离开"));
	Add(TEXT("online.servername"), TEXT("Server name"),  TEXT("Имя сервера"),   TEXT("服务器名称"));
	Add(TEXT("online.joinbyid"),   TEXT("Join by ID"),   TEXT("Войти по ID"),   TEXT("通过ID加入"));
	Add(TEXT("online.lan"),        TEXT("LAN"),          TEXT("Локальная сеть"),TEXT("局域网"));

	// --- Settings categories ---
	Add(TEXT("settings.title"),    TEXT("SETTINGS"), TEXT("НАСТРОЙКИ"),       TEXT("设置"));
	Add(TEXT("settings.graphics"), TEXT("GRAPHICS"), TEXT("ГРАФИКА"),         TEXT("图像"));
	Add(TEXT("settings.audio"),    TEXT("AUDIO"),    TEXT("ЗВУК"),            TEXT("音频"));
	Add(TEXT("settings.controls"), TEXT("CONTROLS"), TEXT("УПРАВЛЕНИЕ"),      TEXT("控制"));
	Add(TEXT("settings.language"), TEXT("LANGUAGE"), TEXT("ЯЗЫК"),            TEXT("语言"));
	Add(TEXT("settings.gameplay"), TEXT("GAMEPLAY"), TEXT("ИГРОВОЙ ПРОЦЕСС"), TEXT("游戏"));
	Add(TEXT("settings.language.label"), TEXT("Language"), TEXT("Язык игры"), TEXT("游戏语言"));

	// --- Graphics labels ---
	Add(TEXT("gfx.preset"),      TEXT("Quality Preset"),   TEXT("Пресет качества"),  TEXT("质量预设"));
	Add(TEXT("gfx.displaymode"), TEXT("Display Mode"),     TEXT("Режим экрана"),     TEXT("显示模式"));
	Add(TEXT("gfx.resolution"),  TEXT("Resolution"),       TEXT("Разрешение"),       TEXT("分辨率"));
	Add(TEXT("gfx.resscale"),    TEXT("Resolution Scale"), TEXT("Масштаб разрешения"),TEXT("分辨率缩放"));
	Add(TEXT("gfx.vsync"),       TEXT("V-Sync"),           TEXT("Верт. синхронизация"),TEXT("垂直同步"));
	Add(TEXT("gfx.fpslimit"),    TEXT("FPS Limit"),        TEXT("Ограничение FPS"),  TEXT("帧率限制"));
	Add(TEXT("gfx.lumen"),       TEXT("Lumen GI"),         TEXT("Lumen (освещение)"),TEXT("Lumen 全局光照"));
	Add(TEXT("gfx.clouds"),      TEXT("Volumetric Clouds"),TEXT("Объёмные облака"),  TEXT("体积云"));
	Add(TEXT("gfx.dlss"),        TEXT("Zonefall DLSS"),      TEXT("Zonefall DLSS"),      TEXT("Zonefall DLSS"));
	Add(TEXT("gfx.framegen"),    TEXT("Zonefall DLSS Frame Gen"), TEXT("Zonefall DLSS — генерация кадров"), TEXT("Zonefall DLSS 帧生成"));
	Add(TEXT("gfx.fsr"),         TEXT("Zonefall FSR"),          TEXT("Zonefall FSR"),          TEXT("Zonefall FSR"));
	Add(TEXT("gfx.fsrframegen"), TEXT("Zonefall FSR Frame Gen"), TEXT("Zonefall FSR — генерация кадров"), TEXT("Zonefall FSR 帧生成"));
	Add(TEXT("gfx.directx"),     TEXT("DirectX Version"),  TEXT("Версия DirectX"),   TEXT("DirectX 版本"));
	Add(TEXT("gfx.raytracing"),  TEXT("Ray Tracing"),      TEXT("Трассировка лучей"),TEXT("光线追踪"));
	Add(TEXT("gfx.brightness"),  TEXT("Brightness"),       TEXT("Яркость"),          TEXT("亮度"));
	Add(TEXT("gfx.fov"),         TEXT("Field of View"),    TEXT("Угол обзора"),      TEXT("视野"));
	Add(TEXT("gfx.advanced"),    TEXT("ADVANCED GRAPHICS"),TEXT("РАСШИРЕННАЯ ГРАФИКА"),TEXT("高级图像"));

	// --- Audio labels ---
	Add(TEXT("audio.master"), TEXT("Master Volume"), TEXT("Общая громкость"), TEXT("主音量"));
	Add(TEXT("audio.sfx"),    TEXT("Effects"),       TEXT("Эффекты"),         TEXT("音效"));
	Add(TEXT("audio.music"),  TEXT("Music"),         TEXT("Музыка"),          TEXT("音乐"));
	Add(TEXT("audio.voice"),  TEXT("Voice"),         TEXT("Голос"),           TEXT("语音"));

	// --- Controls labels ---
	Add(TEXT("controls.sensitivity"), TEXT("Mouse Sensitivity"),  TEXT("Чувствительность мыши"), TEXT("鼠标灵敏度"));
	Add(TEXT("controls.inverty"),     TEXT("Invert Look Y"),      TEXT("Инверсия по Y"),         TEXT("反转Y轴"));
	Add(TEXT("controls.rebind"),      TEXT("Rebind keys below"),  TEXT("Переназначьте клавиши"), TEXT("在下方重新绑定按键"));
	Add(TEXT("controls.gamepad"),     TEXT("Gamepad supported"),  TEXT("Поддержка геймпада"),    TEXT("支持手柄"));

	// --- Pause menu ---
	Add(TEXT("pause.title"),    TEXT("PAUSED"),           TEXT("ПАУЗА"),                  TEXT("暂停"));
	Add(TEXT("pause.resume"),   TEXT("Resume"),           TEXT("Продолжить"),             TEXT("继续"));
	Add(TEXT("pause.save"),     TEXT("Save Game"),        TEXT("Сохранить игру"),         TEXT("保存游戏"));
	Add(TEXT("pause.settings"), TEXT("Settings"),         TEXT("Настройки"),              TEXT("设置"));
	Add(TEXT("pause.mainmenu"), TEXT("Main Menu"),        TEXT("Главное меню"),           TEXT("主菜单"));
	Add(TEXT("pause.quit"),     TEXT("Quit to Desktop"),  TEXT("Выйти на рабочий стол"),  TEXT("退出到桌面"));

	// --- Toasts / misc ---
	Add(TEXT("toast.saved"),    TEXT("GAME SAVED"),       TEXT("ИГРА СОХРАНЕНА"),         TEXT("游戏已保存"));
}
