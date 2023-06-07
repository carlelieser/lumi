#include "monitors.h"
#include "utils.h"
#include "wmi_client.h"
#include <highlevelmonitorconfigurationapi.h>
#include <iostream>
#include <optional>
#include <physicalmonitorenumerationapi.h>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

bool BRIGHTNESS_AVAILABLE = true;

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

BOOL AttemptToGetMonitorBrightness(HANDLE &monitorHandle, DWORD &minBrightness, DWORD &currentBrightness,
                                   DWORD &maxBrightness, int maxRetries = 10) {
	while (!BRIGHTNESS_AVAILABLE) {}
	BRIGHTNESS_AVAILABLE = false;
	BOOL success = false;
	int tries = 0;

	while (!success && tries < maxRetries) {
		success = GetMonitorBrightness(monitorHandle, &minBrightness, &currentBrightness, &maxBrightness);
		tries = tries + 1;
	}

	BRIGHTNESS_AVAILABLE = true;
	return success;
}

BOOL AttemptToSetMonitorBrightness(HANDLE &monitorHandle, int brightness, int maxRetries = 10) {
	while (!BRIGHTNESS_AVAILABLE) {}
	BRIGHTNESS_AVAILABLE = false;
	BOOL success = false;
	int tries = 0;

	while (!success && tries < maxRetries) {
		success = SetMonitorBrightness(monitorHandle, static_cast<DWORD>(brightness));
		tries = tries + 1;
	}

	BRIGHTNESS_AVAILABLE = true;
	return success;
}

bool IsMonitorInternal(const std::string &instanceName) {
	bool internal = false;

	std::thread worker([instanceName, &internal]() {
		WmiClient *client = new WmiClient();
		GUID connectionId = client->connect();

		if (connectionId == GUID_NULL) return;

		std::wstring wmiQuery = L"SELECT * FROM WmiMonitorConnectionParams WHERE InstanceName='" +
		                        NarrowStringToWideString(instanceName) + L"'";

		IEnumWbemClassObject *enumerator = nullptr;
		HRESULT hres = client->service->ExecQuery(
		        bstr_t("WQL"),
		        bstr_t(wmiQuery.c_str()),
		        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
		        nullptr,
		        &enumerator);

		if (FAILED(hres)) {
			std::cout << "Failed to execute WMI query. Error code = 0x" << std::hex << hres << std::endl;
			client->service->Release();
			client->disconnect(connectionId);
			return;
		}

		IWbemClassObject *wmiObject = nullptr;
		ULONG numReturned = 0;

		while (enumerator->Next(WBEM_INFINITE, 1, &wmiObject, &numReturned) == S_OK && numReturned > 0) {
			VARIANT videoOutputTechVariant;
			VariantInit(&videoOutputTechVariant);

			hres = wmiObject->Get(L"VideoOutputTechnology", 0, &videoOutputTechVariant, nullptr, nullptr);

			if (SUCCEEDED(hres) && videoOutputTechVariant.vt == VT_UINT) {
				UINT videoOutputTech = videoOutputTechVariant.uintVal;
				VariantClear(&videoOutputTechVariant);
				wmiObject->Release();
				enumerator->Release();
				client->disconnect(connectionId);
				internal = (videoOutputTech == 0x80000000);
				return;
			}

			VariantClear(&videoOutputTechVariant);
			wmiObject->Release();
		}

		enumerator->Release();
		client->disconnect(connectionId);
	});

	worker.join();

	return internal;
}

std::vector<Monitor> GetAvailableMonitors(bool excludeLaptopMonitors) {
	std::vector<Monitor> monitors = {};

	std::thread worker([&]() {
		WmiClient *client = new WmiClient();
		GUID connectionId = client->connect();
		IEnumWbemClassObject *pEnumerator = NULL;

		if (connectionId == GUID_NULL) return;

		HRESULT hres = client->service->ExecQuery(
		        bstr_t("WQL"),
		        bstr_t("SELECT * FROM WmiMonitorID"),
		        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
		        NULL,
		        &pEnumerator);

		if (FAILED(hres)) {
			std::cout << "Query for WmiMonitorID failed. Error code = 0x"
			          << std::hex << hres << std::endl;
			if (pEnumerator != nullptr) pEnumerator->Release();
			client->disconnect(connectionId);
			return;
		}

		IWbemClassObject *pclsObj = NULL;
		ULONG uReturn = 0;

		while (pEnumerator) {
			Monitor monitorInfo;

			pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);

			if (0 == uReturn) {
				break;
			}

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
			monitorInfo.name = ref.has_value() ? ref.value().name : "Unknown";
			monitorInfo.manufacturer = ConvertVariantToString(manufacturerVariant);
			monitorInfo.serial = ConvertVariantToString(serialVariant);
			monitorInfo.productCode = ConvertVariantToString(productCodeVariant);
			monitorInfo.internal = IsMonitorInternal(monitorInfo.id);

			if (ref.has_value()) {
				monitorInfo.handle = ref.value().handle;
			}

			VariantClear(&instanceNameVariant);
			VariantClear(&manufacturerVariant);
			VariantClear(&serialVariant);
			VariantClear(&productCodeVariant);

			pclsObj->Release();

			if (excludeLaptopMonitors && monitorInfo.internal) break;

			monitors.push_back(monitorInfo);
		}

		pEnumerator->Release();
		client->disconnect(connectionId);
	});

	worker.join();

	return monitors;
}

int WMIGetMonitorBrightness(const std::string monitorId) {
	int brightness = -1;

	std::thread worker([&]() {
		WmiClient *client = new WmiClient();
		GUID connectionId = client->connect();
		IEnumWbemClassObject *pEnumerator = nullptr;

		if (connectionId == GUID_NULL) return;

		HRESULT hres = client->service->ExecQuery(
		        bstr_t("WQL"),
		        bstr_t("SELECT * FROM WmiMonitorBrightness"),
		        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
		        nullptr,
		        &pEnumerator);

		if (FAILED(hres)) {
			std::cout << "Query for WmiMonitorBrightness failed. Error code = 0x"
			          << std::hex << hres << std::endl;
			client->disconnect(connectionId);
			return;
		}

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
				client->disconnect(connectionId);
				return;
			}

			VariantClear(&instanceName);
			pclsObj->Release();
		}

		pEnumerator->Release();
		client->disconnect(connectionId);
	});

	worker.join();

	return brightness;
}

int GetMonitorBrightness(const std::string monitorId) {
	if (IsMonitorInternal(monitorId)) {
		return WMIGetMonitorBrightness(monitorId);
	} else {
		std::vector<Monitor> monitors = GetAvailableMonitors(true);

		for (const auto monitor: monitors) {
			if (monitor.id == monitorId) {
				if (!monitor.handle.has_value()) break;

				DWORD minBrightness = 0;
				DWORD currentBrightness = 0;
				DWORD maxBrightness = 0;

				BOOL result = AttemptToGetMonitorBrightness((HANDLE) monitor.handle.value(), minBrightness, currentBrightness, maxBrightness);

				if (result) {
					return static_cast<int>(currentBrightness);
				}
			}
		}

		return -1;
	}
}

BOOL WMISetMonitorBrightness(const std::string monitorId, const int brightness) {
	WmiClient *client = new WmiClient();
	GUID connectionId = client->connect();

	if (connectionId == GUID_NULL) return false;

	IWbemClassObject *methods = client->getBrightnessMethods(monitorId);
	std::wstring objectPath = L"ROOT\\WMI\\WmiMonitorBrightnessMethods.InstanceName='" + NarrowStringToWideString(monitorId) + L"'";

	if (methods == nullptr) {
		client->disconnect(connectionId);
		return false;
	}

	int normalizedBrightness = std::clamp(brightness, 0, 100);
	uint8_t wmiBrightness = static_cast<uint8_t>(normalizedBrightness);
	IWbemClassObject *pSetBrightnessMethodObj = nullptr;

	HRESULT hres = methods->GetMethod(
	        _bstr_t(L"SetBrightness"),
	        0,
	        &pSetBrightnessMethodObj,
	        nullptr);

	if (FAILED(hres)) {
		std::cout << "Failed to get the SetBrightness method. Error code = 0x"
		          << std::hex << hres << std::endl;
		methods->Release();
		client->disconnect(connectionId);
		return false;
	}

	IWbemClassObject *pInParams = nullptr;
	hres = pSetBrightnessMethodObj->SpawnInstance(0, &pInParams);
	if (FAILED(hres)) {
		std::cout << "Failed to spawn an instance of input parameters. Error code = 0x"
		          << std::hex << hres << std::endl;
		pSetBrightnessMethodObj->Release();
		methods->Release();
		client->disconnect(connectionId);
		return false;
	}

	VARIANT varBrightness;
	varBrightness.vt = VT_UI1;
	varBrightness.bVal = wmiBrightness;
	hres = pInParams->Put(L"Brightness", 0, &varBrightness, 0);

	if (FAILED(hres)) {
		std::cout << "Failed to set the Brightness input parameter value. Error code = 0x"
		          << std::hex << hres << std::endl;
		pInParams->Release();
		pSetBrightnessMethodObj->Release();
		methods->Release();
		client->disconnect(connectionId);
		return false;
	}

	IWbemClassObject *pOutParams = nullptr;
	hres = client->service->ExecMethod(
	        _bstr_t(objectPath.c_str()),
	        _bstr_t(L"SetBrightness"),
	        0,
	        nullptr,
	        pInParams,
	        &pOutParams,
	        nullptr);

	pInParams->Release();
	pSetBrightnessMethodObj->Release();
	methods->Release();

	if (FAILED(hres)) {
		std::cout << "Failed to execute the SetBrightness method. Error code = 0x"
		          << std::hex << hres << std::endl;
		client->disconnect(connectionId);
		return false;
	}

	std::cout << "Brightness set successfully." << std::endl;
	pOutParams->Release();
	client->disconnect(connectionId);

	return true;
}

BOOL SetMonitorBrightness(const std::string monitorId, const int brightness) {
	if (IsMonitorInternal(monitorId)) {
		return WMISetMonitorBrightness(monitorId, brightness);
	} else {
		std::vector<Monitor> monitors = GetAvailableMonitors();
		for (const auto monitor: monitors) {
			if (monitor.id == monitorId) {
				if (!monitor.handle.has_value()) return false;
				return AttemptToSetMonitorBrightness((HANDLE) monitor.handle.value(), brightness);
			}
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