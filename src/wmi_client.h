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

class WmiClient {

private:
	bool createLocator() {
		HRESULT hres = CoInitializeEx(NULL, COINIT_MULTITHREADED);

		if (FAILED(hres)) {
			std::cout << "Failed to initialize COM library. Error code = 0x"
			          << std::hex << hres << std::endl;
			return false;
		}

		hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, reinterpret_cast<LPVOID *>(&locator));

		if (FAILED(hres)) return false;
		return true;
	}

	bool createService() {
		if (locator == nullptr) return false;

		HRESULT hres = locator->ConnectServer(_bstr_t(L"ROOT\\WMI"), NULL, NULL, 0, NULL, 0, 0, &service);

		if (FAILED(hres)) return false;

		hres = CoSetProxyBlanket(service, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);

		if (FAILED(hres)) return false;

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

	void init() {
		createLocator();
		createService();
	}

	void destroy() {
		destroyService();
		destroyLocator();
		CoUninitialize();
	}

public:
	IWbemServices *service = nullptr;
	IWbemLocator *locator = nullptr;

	WmiClient() {
		init();
	}

	~WmiClient() {
		destroy();
	}

	HRESULT execQuery(const std::string &query, IEnumWbemClassObject **enumObj) {
		std::string escapedQuery = EscapeString(query);
		std::wstring wideQuery(escapedQuery.begin(), escapedQuery.end());

		HRESULT hres = service->ExecQuery(
		        _bstr_t(L"WQL"),
		        _bstr_t(wideQuery.c_str()),
		        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
		        nullptr,
		        enumObj);

		if (FAILED(hres)) {
			_com_error err(hres);
			LPCTSTR errMsg = err.ErrorMessage();
			std::cout << "Failed to execute WMI query: " << errMsg << std::endl;
			return hres;
		}

		return S_OK;
	}

	BSTR *pathForInstance(const std::string &instanceName, const std::string className) {
		IEnumWbemClassObject *enumObj = nullptr;

		HRESULT hres = execQuery("SELECT * FROM " + className + " WHERE InstanceName='" + instanceName + "'", &enumObj);

		if (FAILED(hres)) {
			std::cout << "Failed to execute " + className + " query." << std::endl;
			return nullptr;
		}

		IWbemClassObject *pObj = nullptr;
		ULONG numReturned = 0;

		hres = enumObj->Next(WBEM_INFINITE, 1, &pObj, &numReturned);

		enumObj->Release();

		if (FAILED(hres) || numReturned == 0) {
			std::cout << "Failed retrieving object." << std::endl;
			return nullptr;
		}

		VARIANT pathVariant;
		VariantInit(&pathVariant);

		hres = pObj->Get(_bstr_t(L"__PATH"),
		                 0,
		                 &pathVariant,
		                 NULL,
		                 NULL);

		pObj->Release();

		if (FAILED(hres)) {
			std::cout << "Failed retrieving object path." << std::endl;
			return nullptr;
		}

		BSTR *path = new BSTR;

		*path = SysAllocString(pathVariant.bstrVal);

		VariantClear(&pathVariant);

		return path;
	}
};

extern WmiClient client;

#endif// WMI_CLIENT_H
