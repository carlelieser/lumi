#include "monitors.h"
#include "utils.h"
#include "workers/get_brightness.h"
#include "workers/set_brightness.h"
#include <napi.h>

Napi::Promise SetBrightness(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();

	Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
	Napi::Function callback = Napi::Function::New(env, [deferred](const Napi::CallbackInfo& info) {});

	std::string monitorId;
	int brightness;

	if (!info[0].IsUndefined() && info[1].IsUndefined()) {

		if (info[0].IsObject()) {
			Napi::Object configListRef = info[0].As<Napi::Object>();
			Napi::Array monitorIdList = configListRef.GetPropertyNames();
			std::vector<MonitorBrightnessConfiguration> configList;

			if (monitorIdList.Length() == 0) {
				deferred.Resolve(RetrieveSetBrightnessResult(env, false, "No config provided."));
				return deferred.Promise();
			}

			for (size_t i = 0; i < monitorIdList.Length(); i++) {
				monitorId = monitorIdList.Get(i).As<Napi::String>().Utf8Value();
				Napi::Value brightnessRef = configListRef.Get(monitorId);

				if (brightnessRef.IsNumber()) {
					MonitorBrightnessConfiguration config;
					config.monitorId = monitorId;
					config.brightness = brightnessRef.As<Napi::Number>().Int32Value();
					configList.emplace_back(config);
				} else {
					deferred.Resolve(RetrieveSetBrightnessResult(env, false, "Invalid config. Brightness value must be a number"));
					return deferred.Promise();
				}
			}

			SetBrightnessWorker* worker = new SetBrightnessWorker(callback, deferred, configList);
			worker->Queue();
		} else if (info[0].IsNumber()) {
			brightness = info[0].As<Napi::Number>().Uint32Value();
			SetBrightnessWorker* worker = new SetBrightnessWorker(callback, deferred, std::string(""), brightness);
			worker->Queue();
		} else {
			deferred.Resolve(RetrieveSetBrightnessResult(env, false, "Invalid config. Value must be an object"));
		}
	} else {
		monitorId = info[0].As<Napi::String>().Utf8Value();
		brightness = info[1].As<Napi::Number>().Uint32Value();
		SetBrightnessWorker* worker = new SetBrightnessWorker(callback, deferred, monitorId, brightness);
		worker->Queue();
	}

	return deferred.Promise();
}

Napi::Promise GetBrightness(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();

	Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
	Napi::Function callback = Napi::Function::New(env, [deferred](const Napi::CallbackInfo& info) {});

	std::string monitorId = "";

	if (!info[0].IsUndefined() && !info[0].IsString()) {
		deferred.Resolve(RetrieveGetBrightnessResult(env, false, 0));
		return deferred.Promise();
	}

	if (!info[0].IsUndefined()) {
		monitorId = info[0].As<Napi::String>().Utf8Value();
	}

	GetBrightnessWorker* worker = new GetBrightnessWorker(callback, deferred, monitorId);
	worker->Queue();

	return deferred.Promise();
}

Napi::Value GetMonitors(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();

	std::vector<Monitor> monitors;

	std::thread thread([&monitors]() {
		MonitorService* service = new MonitorService();
		monitors = service->GetAvailableMonitors();
	});

	thread.join();

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