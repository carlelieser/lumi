#include "monitors.h"
#include <napi.h>

std::optional<PHYSICAL_MONITOR> GetPhysicalMonitorFromHMONITOR(HMONITOR hMonitor) {
	DWORD monitorCount;
	if (!GetNumberOfPhysicalMonitorsFromHMONITOR(hMonitor, &monitorCount)) {
		return std::nullopt;
	}

	if (monitorCount == 0) {
		return std::nullopt;
	}

	std::unique_ptr<PHYSICAL_MONITOR[]> monitors(new PHYSICAL_MONITOR[monitorCount]);

	if (!GetPhysicalMonitorsFromHMONITOR(hMonitor, monitorCount, monitors.get())) {
		return std::nullopt;
	}

	return monitors[0];
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