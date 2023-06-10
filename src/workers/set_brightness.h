#ifndef SET_BRIGHTNESS_WORKER_H
#define SET_BRIGHTNESS_WORKER_H

#include "../monitor_service.h"
#include "../utils.h"
#include <napi.h>
#include <unordered_map>

class SetBrightnessWorker : public Napi::AsyncWorker {
public:
	SetBrightnessWorker(Napi::Env &env, std::vector<MonitorBrightnessConfiguration> &configurations)
	    : Napi::AsyncWorker(env), deferred(Napi::Promise::Deferred::New(env)), configurations(configurations), success(false), message("") {}

	~SetBrightnessWorker() override {}

	void Execute() override {
		MonitorService service;
		auto monitors = service.GetMonitorRefs();

		if (monitors.empty()) {
			message = "No monitors available.";
			return;
		}

		if (!configurations.empty()) {
			if (configurations.size() == 1) {
				auto monitorId = configurations[0].monitorId;
				if (monitorId == "primary") monitorId = monitors.begin()->first;
				if (monitorId == ALL_MONITORS) {
					success = service.SetGlobalBrightness(configurations[0].brightness);
					return;
				}

				auto it = monitors.find(monitorId);
				if (it != monitors.end()) {
					success = service.SetMonitorBrightness(it->second, configurations[0].brightness);
				}
			} else {
				std::vector<bool> results;
				for (const auto& config: configurations) {
					auto it = monitors.find(config.monitorId);
					if (it != monitors.end()) {
						results.emplace_back(service.SetMonitorBrightness(it->second, config.brightness));
					}
				}
				success = Every(results);
			}
		}
	}

	void OnOK() override {
		Napi::HandleScope scope(Env());
		Napi::Object result = Napi::Object::New(Env());

		result.Set("success", Napi::Boolean::New(Env(), success));
		result.Set("message", message.empty() ? Env().Null() : Napi::String::New(Env(), message));

		deferred.Resolve(result);
	}

	Napi::Promise GetPromise() {
		return deferred.Promise();
	}

private:
	Napi::Promise::Deferred deferred;
	std::vector<MonitorBrightnessConfiguration> configurations;
	bool success;
	std::string message;
};

#endif // SET_BRIGHTNESS_WORKER_H
