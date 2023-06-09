#include "monitors.h"
#include "utils.h"
#include "wmi_client.h"
#include <highlevelmonitorconfigurationapi.h>
#include <iostream>
#include <optional>
#include <physicalmonitorenumerationapi.h>
#include <string>
#include <sysinfoapi.h>
#include <thread>
#include <tuple>
#include <vector>

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

std::optional<PHYSICAL_MONITOR> GetPhysicalMonitorFromHMONITOR(HMONITOR hMonitor) {
	DWORD monitorCount;
	if (!GetNumberOfPhysicalMonitorsFromHMONITOR(hMonitor, &monitorCount)) {
		return std::nullopt;
	}

	if (monitorCount == 0) {
		return std::nullopt;
	}

	std::unique_ptr<PHYSICAL_MONITOR[]> physicalMonitors(new PHYSICAL_MONITOR[monitorCount]);

	if (!GetPhysicalMonitorsFromHMONITOR(hMonitor, monitorCount, physicalMonitors.get())) {
		return std::nullopt;
	}

	return physicalMonitors[0];
}

std::vector<MonitorRef> GetMonitorRefs() {
	UINT pathCount;
	UINT modeCount;
	if (GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &pathCount, &modeCount))
		return {};

	std::vector<DISPLAYCONFIG_PATH_INFO> paths(pathCount);
	std::vector<DISPLAYCONFIG_MODE_INFO> modes(modeCount);
	if (QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &pathCount, paths.data(), &modeCount, modes.data(), nullptr))
		return {};

	std::vector<std::tuple<HANDLE, std::wstring>> handles;
	EnumDisplayMonitors(
	        nullptr,
	        nullptr,
	        [](HMONITOR hMonitor, HDC hdc, LPRECT rc, LPARAM lp) {
		        auto handles = reinterpret_cast<std::vector<std::tuple<HANDLE, std::wstring>> *>(lp);
		        std::optional<PHYSICAL_MONITOR> physicalMonitor = GetPhysicalMonitorFromHMONITOR(hMonitor);

		        if (!physicalMonitor.has_value()) return TRUE;

		        MONITORINFOEXW info = {};
		        info.cbSize = sizeof(MONITORINFOEXW);
		        GetMonitorInfoW(hMonitor, &info);
		        handles->emplace_back(static_cast<HANDLE>(physicalMonitor.value().hPhysicalMonitor), std::wstring(info.szDevice));
		        return TRUE;
	        },
	        reinterpret_cast<LPARAM>(&handles));

	std::vector<MonitorRef> monitors;
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
			monitors.emplace_back(monitor);
		}
	}

	return monitors;
}

std::optional<MonitorRef> FindMonitorRefById(const std::string id) {
	std::vector<MonitorRef> refs = GetMonitorRefs();
	for (const auto ref: refs) {
		if (ref.id == id) {
			return ref;
		}
	}
	return std::nullopt;
}

BOOL AttemptToGetMonitorBrightness(const HANDLE &monitorHandle, DWORD &minBrightness, DWORD &currentBrightness,
                                   DWORD &maxBrightness, int maxRetries = 10) {
	BOOL success = false;
	int tries = 0;

	while (!success && tries < maxRetries) {
		success = GetMonitorBrightness(monitorHandle, &minBrightness, &currentBrightness, &maxBrightness);
		tries = tries + 1;
	}

	return success;
}

BOOL AttemptToSetMonitorBrightness(const HANDLE &monitorHandle, int brightness, int maxRetries = 10) {
	BOOL success = false;
	int tries = 0;

	while (!success && tries < maxRetries) {
		success = SetMonitorBrightness(monitorHandle, static_cast<DWORD>(brightness));
		tries = tries + 1;
	}

	return success;
}

bool IsMonitorInternal(const std::string &instanceName) {
	bool internal = false;

	std::thread worker([instanceName, &internal]() {
		IEnumWbemClassObject *enumerator = nullptr;
		HRESULT hres = client.execQuery("SELECT * FROM WmiMonitorConnectionParams WHERE InstanceName='" +
		                                        instanceName + "'",
		                                &enumerator);

		if (FAILED(hres)) {
			std::cout << "Failed to execute WMI query. Error code = 0x" << std::hex << hres << std::endl;
			return;
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
				internal = (videoOutputTechVariant.intVal == 11);
				VariantClear(&videoOutputTechVariant);
				return;
			}

			VariantClear(&videoOutputTechVariant);
			wmiObject->Release();
		}

		enumerator->Release();
	});

	worker.join();

	return internal;
}

std::vector<Monitor> GetAvailableMonitors(bool excludeLaptopMonitors) {
	std::vector<Monitor> monitors = {};

	std::thread worker([&]() {
		IEnumWbemClassObject *pEnumerator = NULL;

		HRESULT hres = client.execQuery("SELECT * FROM WmiMonitorID", &pEnumerator);

		if (FAILED(hres)) return;

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
			std::optional<MonitorRef> ref = FindMonitorRefById(id);

			monitorInfo.id = id;
			monitorInfo.internal = IsMonitorInternal(monitorInfo.id);
			monitorInfo.name = ref.has_value() ? monitorInfo.internal ? "Internal" : ref.value().name
			                                   : "Unknown";
			monitorInfo.manufacturer = ConvertVariantToString(manufacturerVariant);
			monitorInfo.serial = ConvertVariantToString(serialVariant);
			monitorInfo.productCode = ConvertVariantToString(productCodeVariant);

			if (ref.has_value()) monitorInfo.handle = ref.value().handle;

			VariantClear(&instanceNameVariant);
			VariantClear(&manufacturerVariant);
			VariantClear(&serialVariant);
			VariantClear(&productCodeVariant);

			pclsObj->Release();

			if (excludeLaptopMonitors && monitorInfo.internal) continue;

			monitors.push_back(monitorInfo);
		}

		pEnumerator->Release();
	});

	worker.join();

	return monitors;
}

int WMIGetMonitorBrightness(const std::string monitorId) {
	int brightness = -1;

	std::thread worker([&]() {
		IEnumWbemClassObject *pEnumerator = nullptr;

		HRESULT hres = client.execQuery("SELECT * FROM WmiMonitorBrightness", &pEnumerator);

		if (FAILED(hres)) return;

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
				pclsObj->Get(L"CurrentBrightness", 0, &currentBrightness, nullptr, nullptr);

				if (currentBrightness.vt == VT_UI1) {
					brightness = currentBrightness.bVal;
				}

				VariantClear(&instanceName);
				VariantClear(&currentBrightness);

				pclsObj->Release();
				pEnumerator->Release();
				return;
			}

			VariantClear(&instanceName);
			pclsObj->Release();
		}

		pEnumerator->Release();
	});

	worker.join();

	return brightness;
}

bool InternalDisplayConnected() {
	double size;
	HRESULT result = GetIntegratedDisplaySize(&size);
	return SUCCEEDED(result);
}

int GetMonitorBrightness(const std::string monitorId) {
	std::vector<MonitorRef> monitors = GetMonitorRefs();

	for (const auto monitor: monitors) {
		if (monitor.id == monitorId) {
			if (InternalDisplayConnected()) {
				int brightness = WMIGetMonitorBrightness(monitorId);
				if (brightness != -1) return brightness;
			}

			DWORD minBrightness = 0;
			DWORD currentBrightness = 0;
			DWORD maxBrightness = 0;

			if (AttemptToGetMonitorBrightness(monitor.handle, minBrightness, currentBrightness, maxBrightness)) return static_cast<int>(currentBrightness);
		}
	}

	return -1;
}

BOOL WMISetMonitorBrightness(const std::string monitorId, const int brightness) {
	bool success = false;

	std::thread worker([&]() {
		BSTR *path = client.pathForInstance(monitorId, "WmiMonitorBrightnessMethods");

		if (path == nullptr) return;

		IWbemClassObject *pClass = NULL;
		HRESULT hres = client.service->GetObject(_bstr_t(L"WmiMonitorBrightnessMethods"), 0, NULL, &pClass, NULL);

		if (FAILED(hres)) {
			std::cout << "Failed to get WmiMonitorBrightnessMethods class." << std::endl;
			return;
		}

		IWbemClassObject *pInParamsDefinition = NULL;
		hres = pClass->GetMethod(_bstr_t(L"WmiSetBrightness"), 0, &pInParamsDefinition, NULL);

		if (FAILED(hres)) {
			std::cout << "Failed to get WmiSetBrightness method." << std::endl;
			return;
		}

		IWbemClassObject *pClassInstance = NULL;
		hres = pInParamsDefinition->SpawnInstance(0, &pClassInstance);

		if (FAILED(hres)) {
			std::cout << "Failed to spawn instance." << std::endl;
			return;
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
			return;
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
			return;
		}

		hres = client.service->ExecMethod(_bstr_t(*path),
		                                  _bstr_t(L"WmiSetBrightness"),
		                                  0,
		                                  NULL,
		                                  pClassInstance,
		                                  NULL,
		                                  NULL);

		if (FAILED(hres)) {
			std::cout << "Failed to execute WmiSetBrightness method." << std::endl;
			return;
		}

		VariantClear(&timeoutVariant);
		VariantClear(&brightnessVariant);
		pClassInstance->Release();
		pClass->Release();

		success = true;
	});

	worker.join();

	return success;
}

BOOL SetMonitorBrightness(const std::string monitorId, const int brightness) {
	std::vector<MonitorRef> monitors = GetMonitorRefs();

	for (const auto monitor: monitors) {
		if (monitor.id == monitorId) {
			if (InternalDisplayConnected()) {
				bool success = WMISetMonitorBrightness(monitorId, brightness);
				if (success) return success;
			}
			return AttemptToSetMonitorBrightness(monitor.handle, brightness);
		}
	}

	return false;
}

BOOL SetBrightnessForMonitors(const std::vector<MonitorBrightnessConfiguration> configurations) {
	std::vector<bool> results;

	for (auto config: configurations) {
		results.emplace_back(SetMonitorBrightness(config.monitorId, config.brightness));
	}

	return Every(results);
}

BOOL SetGlobalBrightness(int brightness) {
	std::vector<Monitor> monitors = GetAvailableMonitors();

	for (size_t i = 0; i < monitors.size(); ++i) {
		SetMonitorBrightness(monitors[i].id, brightness);
	}

	return true;
}

Napi::Object RetrieveSetBrightnessResult(Napi::Env env, bool success, std::string message) {
	Napi::Object result = Napi::Object::New(env);

	result.Set("success", Napi::Boolean::New(env, success));
	result.Set("message", message.empty() ? env.Null() : Napi::String::New(env, message));

	return result;
}

Napi::Object RetrieveGetBrightnessResult(Napi::Env env, bool success, int brightness) {
	Napi::Object result = Napi::Object::New(env);

	result.Set("success", Napi::Boolean::New(env, success));
	result.Set("brightness", success ? Napi::Number::New(env, brightness) : env.Null());

	return result;
}