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

#endif// WMI_CLIENT_H
