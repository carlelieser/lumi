﻿#include "display_config_helper.h"

#include <vector>


std::vector<DISPLAYCONFIG_PATH_INFO> GetDisplayConfigPathInfos() {
  for (LONG result = ERROR_INSUFFICIENT_BUFFER;
       result == ERROR_INSUFFICIENT_BUFFER;) {
    uint32_t path_elements, mode_elements;
    if (::GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &path_elements,
                                      &mode_elements) != ERROR_SUCCESS) {
      return {};
    }
    std::vector<DISPLAYCONFIG_PATH_INFO> path_infos(path_elements);
    std::vector<DISPLAYCONFIG_MODE_INFO> mode_infos(mode_elements);
    result = ::QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &path_elements,
                                  path_infos.data(), &mode_elements,
                                  mode_infos.data(), nullptr);
    if (result == ERROR_SUCCESS) {
      path_infos.resize(path_elements);
      return path_infos;
    }
  }
  return {};
}

bool GetTargetDeviceName(const DISPLAYCONFIG_PATH_INFO& path,
                         DISPLAYCONFIG_TARGET_DEVICE_NAME* device_name) {
  device_name->header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
  device_name->header.size = sizeof(DISPLAYCONFIG_TARGET_DEVICE_NAME);
  device_name->header.adapterId = path.targetInfo.adapterId;
  device_name->header.id = path.targetInfo.id;
  LONG result = DisplayConfigGetDeviceInfo(&device_name->header);
  return result == ERROR_SUCCESS;
}


std::optional<DISPLAYCONFIG_PATH_INFO> GetDisplayConfigPathInfoFromMonitor(
    HMONITOR monitor) {
  MONITORINFOEX monitor_info = {};
  monitor_info.cbSize = sizeof(monitor_info);
  if (!::GetMonitorInfo(monitor, &monitor_info)) {
    return std::nullopt;
  }

  return GetDisplayConfigPathInfo(monitor_info);
}

DISPLAY_EXPORT std::optional<DISPLAYCONFIG_PATH_INFO> GetDisplayConfigPathInfo(const MONITORINFOEX& monitor_info) {
	wchar_t wideDeviceName[CCHDEVICENAME] = {};
	MultiByteToWideChar(CP_ACP, 0, monitor_info.szDevice, -1, wideDeviceName, CCHDEVICENAME);

	for (const auto& info : GetDisplayConfigPathInfos()) {
		DISPLAYCONFIG_SOURCE_DEVICE_NAME device_name = {};
		device_name.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
		device_name.header.size = sizeof(device_name);
		device_name.header.adapterId = info.sourceInfo.adapterId;
		device_name.header.id = info.sourceInfo.id;

		if ((::DisplayConfigGetDeviceInfo(&device_name.header) == ERROR_SUCCESS) &&
			(wcscmp(wideDeviceName, device_name.viewGdiDeviceName) == 0)) {
			return info;
			}
	}
	return std::nullopt;
}

UINT16 GetDisplayManufacturerId(
    const std::optional<DISPLAYCONFIG_PATH_INFO>& path) {
  DISPLAYCONFIG_TARGET_DEVICE_NAME targetName = {};
  if (path && GetTargetDeviceName(*path, &targetName)) {
    return targetName.edidManufactureId;
  }
  return 0;
}

UINT16 GetDisplayProductCode(
    const std::optional<DISPLAYCONFIG_PATH_INFO>& path) {
  DISPLAYCONFIG_TARGET_DEVICE_NAME targetName = {};
  if (path && GetTargetDeviceName(*path, &targetName)) {
    return targetName.edidProductCodeId;
  }
  return 0;
}

