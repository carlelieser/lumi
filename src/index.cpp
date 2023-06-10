#include "monitor_service.h"
#include "utils.h"
#include "workers/get_brightness.h"
#include "workers/set_brightness.h"
#include <napi.h>

Napi::Promise SetBrightness(const Napi::CallbackInfo &info) {
	Napi::Env env = info.Env();

	std::vector<MonitorBrightnessConfiguration> configList = {};

	if (!info[0].IsUndefined() && info[1].IsUndefined()) {
		if (info[0].IsObject()) {
			Napi::Object providedConfig = info[0].As<Napi::Object>();
			Napi::Array monitors = providedConfig.GetPropertyNames();
			for (size_t i = 0; i < monitors.Length(); i++) {
				std::string monitorId = monitors.Get(i).As<Napi::String>().Utf8Value();
				Napi::Value providedBrightness = providedConfig.Get(monitorId);
				if (providedBrightness.IsNumber()) {
					MonitorBrightnessConfiguration config = {};
					config.monitorId = monitorId;
					config.brightness = providedBrightness.As<Napi::Number>().Int32Value();
					configList.emplace_back(config);
				}
			}
		} else if (info[0].IsNumber()) {
			MonitorBrightnessConfiguration config = {};
			config.monitorId = "primary";
			config.brightness = info[0].As<Napi::Number>().Int32Value();
			configList.emplace_back(config);
		}
	} else {
		MonitorBrightnessConfiguration config = {};
		config.monitorId = info[0].As<Napi::String>().Utf8Value();
		config.brightness = info[1].As<Napi::Number>().Uint32Value();
		configList.emplace_back(config);
	}

	SetBrightnessWorker *worker = new SetBrightnessWorker(env, configList);
	auto promise = worker->GetPromise();
	worker->Queue();

	return promise;
}

Napi::Promise GetBrightness(const Napi::CallbackInfo &info) {
	Napi::Env env = info.Env();

	std::string monitorId = "";

	if (!info[0].IsUndefined()) monitorId = info[0].As<Napi::String>().Utf8Value();

	GetBrightnessWorker *worker = new GetBrightnessWorker(env, monitorId);
	auto promise = worker->GetPromise();
	worker->Queue();

	return promise;
}

Napi::Value GetMonitors(const Napi::CallbackInfo &info) {
	Napi::Env env = info.Env();

	MonitorService *service = new MonitorService();
	std::vector<Monitor> monitors = service->GetAvailableMonitors();

	Napi::Array jsArray = Napi::Array::New(env, monitors.size());

	for (size_t i = 0; i < monitors.size(); ++i) {
		Napi::Object monitor = Napi::Object::New(env);
		monitor.Set(Napi::String::New(env, "id"), monitors[i].id);
		monitor.Set(Napi::String::New(env, "name"), ToNapiString(env, monitors[i].name));
		monitor.Set(Napi::String::New(env, "manufacturer"), ToNapiString(env, monitors[i].manufacturer));
		monitor.Set(Napi::String::New(env, "serial"), ToNapiString(env, monitors[i].serial));
		monitor.Set(Napi::String::New(env, "productCode"), ToNapiString(env, monitors[i].productCode));
		monitor.Set(Napi::String::New(env, "internal"), Napi::Boolean::New(env, monitors[i].internal));
		jsArray.Set(i, monitor);
	}

	return jsArray;
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
	Napi::HandleScope scope(env);

	exports.Set(Napi::String::New(env, "GLOBAL"), Napi::String::New(env, ALL_MONITORS));
	exports.Set(Napi::String::New(env, "get"), Napi::Function::New(env, GetBrightness));
	exports.Set(Napi::String::New(env, "set"), Napi::Function::New(env, SetBrightness));
	exports.Set(Napi::String::New(env, "monitors"), Napi::Function::New(env, GetMonitors));

	return exports;
}

NODE_API_MODULE(Lumi, Init);