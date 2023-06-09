#ifndef MONITORS_H
#define MONITORS_H

#include <highlevelmonitorconfigurationapi.h>
#include <optional>
#include <string>
#include <napi.h>

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

std::optional<PHYSICAL_MONITOR> GetPhysicalMonitorFromHMONITOR(HMONITOR hMonitor);
Napi::Object RetrieveSetBrightnessResult(Napi::Env env, bool success, std::string message);
Napi::Object RetrieveGetBrightnessResult(Napi::Env env, bool success, int brightness);

#endif// MONITORS_H
