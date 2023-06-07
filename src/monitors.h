#ifndef MONITORS_H
#define MONITORS_H

#include <napi.h>
#include <string>
#include <vector>
#include <optional>
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
	std::optional<HANDLE> handle;
	bool internal;
};

struct MonitorBrightnessConfiguration {
	std::string monitorId;
	int brightness;
};

std::vector<Monitor> GetAvailableMonitors(bool excludeLaptopMonitors = false);
int GetMonitorBrightness(const std::string monitorId);
BOOL SetMonitorBrightness(const std::string monitorId, const int brightness);
BOOL SetBrightnessForMonitors(const std::vector<MonitorBrightnessConfiguration> configurations);
BOOL SetGlobalBrightness(const int brightness);
Napi::Object RetrieveSetBrightnessResult(Napi::Env env, bool success, std::string message);
Napi::Object RetrieveGetBrightnessResult(Napi::Env env, bool success, int brightness);

#endif// MONITORS_H
