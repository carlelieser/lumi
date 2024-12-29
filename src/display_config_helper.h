#ifndef DISPLAY_CONFIG_HELPER_H
#define DISPLAY_CONFIG_HELPER_H

#include <windows.h>

#include <optional>

#include "display_export.h"

DISPLAY_EXPORT std::optional<DISPLAYCONFIG_PATH_INFO> GetDisplayConfigPathInfoFromMonitor(HMONITOR monitor);

DISPLAY_EXPORT std::optional<DISPLAYCONFIG_PATH_INFO> GetDisplayConfigPathInfo(const MONITORINFOEX& monitor_info);


DISPLAY_EXPORT UINT16
GetDisplayManufacturerId(const std::optional<DISPLAYCONFIG_PATH_INFO>& path);

DISPLAY_EXPORT UINT16
GetDisplayProductCode(const std::optional<DISPLAYCONFIG_PATH_INFO>& path);


#endif  // DISPLAY_CONFIG_HELPER_H