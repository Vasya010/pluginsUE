// Zonefall Protocol — слой совместимости поверх NVIDIA DLSS (UE 5.8).

#pragma once

#define ZONEFALL_DLSS_PATCH_VERSION TEXT("zonefall-2")

namespace ZonefallDLSSCompat
{
	/** Регистрация логов и проверок после загрузки модуля DLSS. */
	void Register();
}
