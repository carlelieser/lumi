#ifndef MONITOR_SERVICE_H
#define MONITOR_SERVICE_H

#include "wmi_client.h"
#include <algorithm>
#include <execution>
#include <highlevelmonitorconfigurationapi.h>
#include <physicalmonitorenumerationapi.h>
#include <string>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <vector>
#include <windows.h>

const std::string ALL_MONITORS = "GLOBAL";

struct MonitorRef {
	std::string id;
	std::string name;
	HANDLE handle;
};

struct Monitor {
	std::string id;
	std::string name;
	std::string manufacturer;
	std::string serial;
	std::string productCode;
	HANDLE handle;
	bool internal;
};

struct MonitorBrightnessConfiguration {
	std::string monitorId;
	int brightness;
};

class MonitorService {
private:
	WmiClient *client;

	std::string DeriveDisplayIdentifier(const std::string &displayDeviceString, const int occurrence) {
		std::vector<std::string> parts = SplitString(displayDeviceString, '#');
		return "DISPLAY\\" + parts[1] + "\\" + parts[2] + "_" + std::to_string(occurrence);
	}

	std::tuple<std::string, std::string> GetDeviceInfoFromPath(DISPLAYCONFIG_PATH_INFO path) {
		DISPLAYCONFIG_TARGET_DEVICE_NAME deviceName = {};
		deviceName.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
		deviceName.header.size = sizeof(DISPLAYCONFIG_TARGET_DEVICE_NAME);
		deviceName.header.adapterId = path.targetInfo.adapterId;
		deviceName.header.id = path.targetInfo.id;

		LONG result = DisplayConfigGetDeviceInfo((DISPLAYCONFIG_DEVICE_INFO_HEADER *) &deviceName);

		if (result == ERROR_SUCCESS) {
			return {ToUTF8(WideStringFromArray(deviceName.monitorDevicePath)), ToUTF8(WideStringFromArray(deviceName.monitorFriendlyDeviceName))};
		}

		return {"Unknown", "Unknown"};
	}

	std::string GetGDIDeviceNameFromPath(DISPLAYCONFIG_PATH_INFO path) {
		DISPLAYCONFIG_SOURCE_DEVICE_NAME sourceName = {};
		sourceName.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
		sourceName.header.size = sizeof(DISPLAYCONFIG_SOURCE_DEVICE_NAME);
		sourceName.header.adapterId = path.targetInfo.adapterId;
		sourceName.header.id = path.sourceInfo.id;

		LONG result = DisplayConfigGetDeviceInfo((DISPLAYCONFIG_DEVICE_INFO_HEADER *) &sourceName);

		if (result == ERROR_SUCCESS) {
			return ToUTF8(WideStringFromArray(sourceName.viewGdiDeviceName));
		}

		return "Unknown";
	}

	BOOL AttemptToGetMonitorBrightness(const HANDLE &monitorHandle, DWORD &minBrightness, DWORD &currentBrightness,
	                                   DWORD &maxBrightness, int maxRetries = 10) {
		BOOL success = false;
		int tries = 0;

		while (!success && tries < maxRetries) {
			success = HighLevelMonitorConfigurationAPI_h::GetMonitorBrightness(monitorHandle, &minBrightness, &currentBrightness, &maxBrightness);
			tries = tries + 1;
		}

		return success;
	}

	BOOL AttemptToSetMonitorBrightness(const HANDLE &monitorHandle, int brightness, int maxRetries = 10) {
		BOOL success = false;
		int tries = 0;

		while (!success && tries < maxRetries) {
			success = HighLevelMonitorConfigurationAPI_h::SetMonitorBrightness(monitorHandle, static_cast<DWORD>(brightness));
			tries = tries + 1;
		}

		return success;
	}

	bool IsMonitorInternal(const std::string &instanceName) {
		IEnumWbemClassObject *enumerator = nullptr;
		HRESULT hres = client->execQuery("SELECT * FROM WmiMonitorConnectionParams WHERE InstanceName='" +
		                                         instanceName + "'",
		                                 &enumerator);

		if (FAILED(hres)) {
			std::cout << "Failed to execute WMI query. Error code = 0x" << std::hex << hres << std::endl;
			return false;
		}

		IWbemClassObject *wmiObject = nullptr;
		ULONG numReturned = 0;

		while (enumerator->Next(WBEM_INFINITE, 1, &wmiObject, &numReturned) == S_OK && numReturned > 0) {
			VARIANT videoOutputTechVariant;
			VariantInit(&videoOutputTechVariant);

			hres = wmiObject->Get(L"VideoOutputTechnology", 0, &videoOutputTechVariant, nullptr, nullptr);

			if (SUCCEEDED(hres)) {
				wmiObject->Release();
				enumerator->Release();
				bool internal = (videoOutputTechVariant.intVal == 11);
				VariantClear(&videoOutputTechVariant);
				return internal;
			}

			VariantClear(&videoOutputTechVariant);
			wmiObject->Release();
		}

		enumerator->Release();

		return false;
	}

	int WMIGetMonitorBrightness(const std::string monitorId) {
		IEnumWbemClassObject *pEnumerator = nullptr;

		HRESULT hres = client->execQuery("SELECT * FROM WmiMonitorBrightness", &pEnumerator);

		if (FAILED(hres)) return -1;

		IWbemClassObject *pclsObj = nullptr;
		ULONG uReturn = 0;

		while (pEnumerator) {
			hres = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
			if (hres != S_OK) {
				if (hres == WBEM_S_FALSE) {
					break;
				} else {
					std::cout << "Failed to retrieve next object from enumerator. Error code = 0x"
					          << std::hex << hres << std::endl;
					break;
				}
			}

			VARIANT instanceName;
			pclsObj->Get(L"InstanceName", 0, &instanceName, nullptr, nullptr);

			if (ToUTF8(instanceName.bstrVal) == monitorId) {
				VARIANT currentBrightness;
				int brightness = -1;

				pclsObj->Get(L"CurrentBrightness", 0, &currentBrightness, nullptr, nullptr);

				if (currentBrightness.vt == VT_UI1) {
					brightness = currentBrightness.bVal;
				}

				VariantClear(&instanceName);
				VariantClear(&currentBrightness);

				pclsObj->Release();
				pEnumerator->Release();
				return brightness;
			}

			VariantClear(&instanceName);
			pclsObj->Release();
		}

		pEnumerator->Release();

		return -1;
	}

	BOOL WMISetMonitorBrightness(const std::string monitorId, const int brightness) {
		BSTR *path = client->pathForInstance(monitorId, "WmiMonitorBrightnessMethods");

		if (path == nullptr) return false;

		IWbemClassObject *pClass = NULL;
		HRESULT hres = client->getMethod("WmiMonitorBrightnessMethods", &pClass);

		if (FAILED(hres)) return false;

		IWbemClassObject *pInParamsDefinition = NULL;
		hres = pClass->GetMethod(_bstr_t(L"WmiSetBrightness"), 0, &pInParamsDefinition, NULL);

		if (FAILED(hres)) {
			std::cout << "Failed to get WmiSetBrightness method." << std::endl;
			return false;
		}

		IWbemClassObject *pClassInstance = NULL;
		hres = pInParamsDefinition->SpawnInstance(0, &pClassInstance);

		if (FAILED(hres)) {
			std::cout << "Failed to spawn instance." << std::endl;
			return false;
		}

		VARIANT timeoutVariant;
		VariantInit(&timeoutVariant);
		V_VT(&timeoutVariant) = VT_UI1;
		V_UI1(&timeoutVariant) = 0;

		hres = pClassInstance->Put(_bstr_t(L"Timeout"),
		                           0,
		                           &timeoutVariant,
		                           CIM_UINT32);

		if (FAILED(hres)) {
			std::cout << "Failed to insert timeout value." << std::endl;
			return false;
		}

		VARIANT brightnessVariant;
		VariantInit(&brightnessVariant);
		V_VT(&brightnessVariant) = VT_UI1;
		V_UI1(&brightnessVariant) = brightness;

		hres = pClassInstance->Put(_bstr_t(L"Brightness"),
		                           0,
		                           &brightnessVariant,
		                           CIM_UINT8);

		if (FAILED(hres)) {
			std::cout << "Failed to add brightness value." << std::endl;
			return false;
		}

		hres = client->execMethod(*path, "WmiSetBrightness", pClassInstance);

		if (FAILED(hres)) {
			std::cout << "Failed to execute WmiSetBrightness method." << std::endl;
			return false;
		}

		VariantClear(&timeoutVariant);
		VariantClear(&brightnessVariant);
		pClassInstance->Release();
		pClass->Release();

		return true;
	}

	PHYSICAL_MONITOR GetPhysicalMonitorFromHMONITOR(HMONITOR hMonitor) {
		PHYSICAL_MONITOR monitor = {};
		DWORD monitorCount;

		if (!GetNumberOfPhysicalMonitorsFromHMONITOR(hMonitor, &monitorCount)) return monitor;

		if (monitorCount == 0) return monitor;

		std::unique_ptr<PHYSICAL_MONITOR[]> monitors(new PHYSICAL_MONITOR[monitorCount]);

		if (!GetPhysicalMonitorsFromHMONITOR(hMonitor, monitorCount, monitors.get())) return monitor;

		return monitors[0];
	}

	std::vector<std::tuple<HANDLE, std::wstring>> handles;

	static BOOL CALLBACK MonitorEnumProcStatic(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
		MonitorService* monitorService = reinterpret_cast<MonitorService*>(dwData);
		return monitorService->callback(hMonitor, hdcMonitor, lprcMonitor);
	}

	bool callback(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor) {
		PHYSICAL_MONITOR physicalMonitor = GetPhysicalMonitorFromHMONITOR(hMonitor);
		MONITORINFOEXW info = {};
		info.cbSize = sizeof(MONITORINFOEXW);
		GetMonitorInfoW(hMonitor, &info);
		handles.emplace_back(static_cast<HANDLE>(physicalMonitor.hPhysicalMonitor), std::wstring(info.szDevice));
		return TRUE;
	}

public:
	std::unordered_map<std::string, MonitorRef> GetMonitorRefs() {
		UINT pathCount;
		UINT modeCount;
		if (GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &pathCount, &modeCount))
			return {};

		std::vector<DISPLAYCONFIG_PATH_INFO> paths(pathCount);
		std::vector<DISPLAYCONFIG_MODE_INFO> modes(modeCount);
		if (QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &pathCount, paths.data(), &modeCount, modes.data(), nullptr))
			return {};

		EnumDisplayMonitors(
		        nullptr,
		        nullptr,
		        &MonitorEnumProcStatic,
		        reinterpret_cast<LPARAM>(this));

		std::unordered_map<std::string, MonitorRef> monitors;
		std::vector<std::string> monitorIdList;

		for (UINT i = 0; i < pathCount; i++) {
			std::tuple<std::string, std::string> info = GetDeviceInfoFromPath(paths[i]);
			std::string GDIDeviceName = GetGDIDeviceNameFromPath(paths[i]);

			auto target = std::find_if(handles.begin(), handles.end(), [&GDIDeviceName](const std::tuple<HANDLE, std::wstring> &t) {
				return (ToUTF8(std::get<1>(t)) == GDIDeviceName);
			});

			if (target != handles.end()) {
				MonitorRef monitor;
				int instance = CountOccurrence(monitorIdList, std::get<0>(info));
				monitor.id = DeriveDisplayIdentifier(std::get<0>(info), instance);
				monitor.name = std::get<1>(info);
				monitor.handle = std::get<0>(*target);
				monitorIdList.emplace_back(std::get<0>(info));
				monitors[monitor.id] = monitor;
			}
		}

		return monitors;
	}

	std::vector<Monitor> GetAvailableMonitors() {
		std::unordered_map<std::string, MonitorRef> refs = GetMonitorRefs();
		std::vector<Monitor> monitors;
		IEnumWbemClassObject *pEnumerator = NULL;

		HRESULT hres = client->execQuery("SELECT * FROM WmiMonitorID", &pEnumerator);

		if (FAILED(hres)) return {};

		IWbemClassObject *pclsObj = NULL;
		ULONG uReturn = 0;

		while (pEnumerator) {
			Monitor monitorInfo;

			pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);

			if (0 == uReturn) break;

			VARIANT instanceNameVariant;
			VARIANT manufacturerVariant;
			VARIANT serialVariant;
			VARIANT productCodeVariant;

			pclsObj->Get(L"InstanceName", 0, &instanceNameVariant, 0, 0);
			pclsObj->Get(L"ManufacturerName", 0, &manufacturerVariant, 0, 0);
			pclsObj->Get(L"SerialNumberID", 0, &serialVariant, 0, 0);
			pclsObj->Get(L"ProductCodeID", 0, &productCodeVariant, 0, 0);

			std::string id = ToUTF8(instanceNameVariant.bstrVal);

			monitorInfo.id = id;
			monitorInfo.internal = IsMonitorInternal(monitorInfo.id);
			monitorInfo.manufacturer = ConvertVariantToString(manufacturerVariant);
			monitorInfo.serial = ConvertVariantToString(serialVariant);
			monitorInfo.productCode = ConvertVariantToString(productCodeVariant);

			auto it = refs.find(id);

			if (it != refs.end()) {
				monitorInfo.name = monitorInfo.internal ? "Built-in" : it->second.name;
				monitorInfo.handle = it->second.handle;
			}

			VariantClear(&instanceNameVariant);
			VariantClear(&manufacturerVariant);
			VariantClear(&serialVariant);
			VariantClear(&productCodeVariant);

			pclsObj->Release();
			monitors.push_back(monitorInfo);
		}

		pEnumerator->Release();

		return monitors;
	}

	int GetMonitorBrightness(const MonitorRef ref) {
		DWORD minBrightness = 0;
		DWORD currentBrightness = 0;
		DWORD maxBrightness = 0;

		if (AttemptToGetMonitorBrightness(ref.handle, minBrightness, currentBrightness, maxBrightness)) return static_cast<int>(currentBrightness);

		return WMIGetMonitorBrightness(ref.id);
	}

	BOOL SetMonitorBrightness(const MonitorRef monitor, const int brightness) {
		if (AttemptToSetMonitorBrightness(monitor.handle, brightness)) return true;
		return WMISetMonitorBrightness(monitor.id, brightness);
	}

	BOOL SetGlobalBrightness(int brightness) {
		std::unordered_map<std::string, MonitorRef> monitors = GetMonitorRefs();

		std::for_each(
		        std::execution::par_unseq,
		        monitors.begin(),
		        monitors.end(),
		        [&](auto &&monitor) {
			        SetMonitorBrightness(monitor.second, brightness);
		        });

		return true;
	}

	MonitorService() {
		client = new WmiClient();
	}

	~MonitorService() {
		delete client;
	}
};

#endif//MONITOR_SERVICE_H
