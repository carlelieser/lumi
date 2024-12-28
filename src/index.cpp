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
	std::vector<Monitor> monitors = {};

	MonitorService *service = new MonitorService();
	monitors = service->GetAvailableMonitors();

	Napi::Array jsArray = Napi::Array::New(env, monitors.size());

	for (size_t i = 0; i < monitors.size(); ++i) {
		auto monitor = monitors[i];
		Napi::Object result = Napi::Object::New(env);
		Napi::Object size = Napi::Object::New(env);
		Napi::Object position = Napi::Object::New(env);
		result.Set(Napi::String::New(env, "id"), monitor.id);
		result.Set(Napi::String::New(env, "name"), ToNapiString(env, monitor.name));
		result.Set(Napi::String::New(env, "manufacturer"), ToNapiString(env, monitor.manufacturer));
		result.Set(Napi::String::New(env, "serialNumber"), ToNapiString(env, monitor.serialNumber));
		result.Set(Napi::String::New(env, "productCode"), ToNapiString(env, monitor.productCode));
		result.Set(Napi::String::New(env, "internal"), Napi::Boolean::New(env, monitor.internal));
		size.Set(Napi::String::New(env, "width"), Napi::Number::New(env, monitor.size.width));
		size.Set(Napi::String::New(env, "height"), Napi::Number::New(env, monitor.size.height));
		position.Set(Napi::String::New(env, "x"), Napi::Number::New(env, monitor.position.x));
		position.Set(Napi::String::New(env, "y"), Napi::Number::New(env, monitor.position.y));
		result.Set(Napi::String::New(env, "size"), size);
		result.Set(Napi::String::New(env, "position"), position);
		jsArray.Set(i, result);
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