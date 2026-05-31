// Zonefall Protocol — локальные правки поверх AMD FSR (UE 5.8).
// При слиянии с новым релизом AMD ищите ZONEFALL_PATCH в FFXFrameInterpolation*.cpp/.h

#pragma once

#define ZONEFALL_FSR_PATCH_VERSION TEXT("zonefall-2")

/** Файлы с правками Zonefall (не перезаписывать слепо при merge upstream). */
#define ZONEFALL_FSR_PATCHED_SOURCE_FILES \
	TEXT("FFXFrameInterpolation.cpp"), \
	TEXT("FFXFrameInterpolation.h"), \
	TEXT("FFXFrameInterpolationCustomPresent.h"), \
	TEXT("FFXFrameInterpolationCustomPresent.cpp")
