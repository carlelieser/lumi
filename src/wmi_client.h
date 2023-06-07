#ifndef WMI_CLIENT_H
#define WMI_CLIENT_H

#include "utils.h"
#include <combaseapi.h>
#include <comdef.h>
#include <iostream>
#include <string>
#include <vector>
#include <wbemidl.h>
#include <windows.h>

struct ConnectionAttempt {
	GUID id;
};

class WmiClient {

private:
	std::vector<ConnectionAttempt> connectionAttempts;
	std::thread comThread;

	bool initLocator() {
		HRESULT hres = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

		if (FAILED(hres)) {
			std::cout << "Failed to initialize COM library. Error code = 0x"
			          << std::hex << hres << std::endl;
			if (hres != 0x80010106) {
				disconnect();
				return false;
			}
		}

		hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, reinterpret_cast<LPVOID *>(&locator));

		if (FAILED(hres)) {
			disconnect();
			return false;
		}
		return true;
	}

	bool initService() {
		if (locator == nullptr) return false;

		HRESULT hres = locator->ConnectServer(_bstr_t(L"ROOT\\WMI"), NULL, NULL, 0, NULL, 0, 0, &service);

		if (FAILED(hres)) {
			disconnect();
			return false;
		}

		hres = CoSetProxyBlanket(service, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);

		if (FAILED(hres)) {
			disconnect();
			return false;
		}

		return true;
	}

	void destroyLocator() {
		if (locator != nullptr) locator->Release();
		locator = nullptr;
	}

	void destroyService() {
		if (service != nullptr) service->Release();
		service = nullptr;
	}

	bool isConnected() {
		return connectionAttempts.size() > 0;
	}

	void removeConnectionAttempt(const GUID &id) {
		connectionAttempts.erase(std::remove_if(connectionAttempts.begin(), connectionAttempts.end(),
		                                        [&](const ConnectionAttempt &attempt) { return attempt.id == id; }),
		                         connectionAttempts.end());
	}

	GUID storeConnectionAttempt() {
		ConnectionAttempt attempt;
		CoCreateGuid(&(attempt.id));
		connectionAttempts.push_back(attempt);
		return attempt.id;
	}

	GUID initComLibrary() {
		if (initLocator() && initService()) {
			return storeConnectionAttempt();
		}
		return GUID_NULL;
	}

public:
	IWbemServices *service = nullptr;
	IWbemLocator *locator = nullptr;

	GUID connect() {
		if (isConnected()) {
			return storeConnectionAttempt();
		} else {
			return initComLibrary();
		}
	}

	void disconnect(GUID connectionId = GUID_NULL) {
		if (connectionId != GUID_NULL) {
			removeConnectionAttempt(connectionId);
			if (connectionAttempts.size() != 0) return;
		}
		destroyService();
		destroyLocator();
		CoUninitialize();
		delete this;
	}

	IWbemClassObject *getBrightnessMethods(std::string instanceName) {
		std::wstring objectPath = L"ROOT\\WMI\\WmiMonitorBrightnessMethods";
		IEnumWbemClassObject *pEnumInstances = nullptr;
		HRESULT hres = service->CreateInstanceEnum(_bstr_t(objectPath.c_str()), WBEM_FLAG_FORWARD_ONLY, nullptr, &pEnumInstances);
		if (FAILED(hres)) {
			return nullptr;
		}
		IWbemClassObject *pBrightnessMethodsObj = nullptr;
		ULONG returnedCount = 0;
		while (pEnumInstances->Next(WBEM_INFINITE, 1, &pBrightnessMethodsObj, &returnedCount) == S_OK) {
			VARIANT varInstanceName;
			hres = pBrightnessMethodsObj->Get(L"InstanceName", 0, &varInstanceName, nullptr, nullptr);
			if (SUCCEEDED(hres) && varInstanceName.vt == VT_BSTR) {
				std::wstring wstrInstanceName = std::wstring(varInstanceName.bstrVal);
				VariantClear(&varInstanceName);
				if (wstrInstanceName == NarrowStringToWideString(instanceName)) {
					pEnumInstances->Release();
					return pBrightnessMethodsObj;
				}
			}
			pBrightnessMethodsObj->Release();
			pBrightnessMethodsObj = nullptr;
		}
		pEnumInstances->Release();
		return nullptr;
	}
};

#endif// WMI_CLIENT_H
