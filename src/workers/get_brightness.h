#ifndef GET_BRIGHTNESS_WORKER_H
#define GET_BRIGHTNESS_WORKER_H

#include "../monitor_service.h"
#include "../utils.h"
#include <napi.h>
#include <unordered_map>

class GetBrightnessWorker : public Napi::AsyncWorker {
public:
	GetBrightnessWorker(Napi::Env &env, const std::string &monitorId)
	    : Napi::AsyncWorker(env), deferred(Napi::Promise::Deferred::New(env)), monitorId(monitorId), success(false), brightness(-1) {}

	~GetBrightnessWorker() override {}

	void Execute() override {
		std::thread thread([&]() {
			MonitorService *service = new MonitorService();
			auto monitors = service->GetMonitorRefs();

			if (monitors.empty()) {
				return;
			}

			if (monitorId.empty()) {
				brightness = service->GetMonitorBrightness(monitors.begin()->second);
				return;
			}

			auto it = monitors.find(monitorId);
			if (it != monitors.end()) {
				brightness = service->GetMonitorBrightness(it->second);
			}
		});

		thread.join();
	}

	void OnOK() override {
		Napi::HandleScope scope(Env());
		Napi::Object result = Napi::Object::New(Env());

		success = brightness != -1;

		result.Set("success", Napi::Boolean::New(Env(), success));
		result.Set("brightness", success ? Napi::Number::New(Env(), brightness) : Env().Null());

		deferred.Resolve(result);
	}

	Napi::Promise GetPromise() {
		return deferred.Promise();
	}

private:
	Napi::Promise::Deferred deferred;
	std::string monitorId;
	bool success;
	int brightness;
};

#endif// GET_BRIGHTNESS_WORKER_H
