#ifndef SET_BRIGHTNESS_WORKER_H
#define SET_BRIGHTNESS_WORKER_H

#include "../monitors.h"
#include "../utils.h"
#include <iostream>
#include <napi.h>

class SetBrightnessWorker : public Napi::AsyncWorker {
public:
	SetBrightnessWorker(Napi::Function &callback, Napi::Promise::Deferred &deferred, std::string &monitorId,
	                    int brightness)
	    : Napi::AsyncWorker(callback), deferred(deferred), monitorId(monitorId), success(false), brightness(brightness), message("") {}

public:
	SetBrightnessWorker(Napi::Function &callback, Napi::Promise::Deferred &deferred, std::vector<MonitorBrightnessConfiguration> &configurations) : Napi::AsyncWorker(callback), deferred(deferred), success(false), configurations(configurations), message("") {}

	~SetBrightnessWorker() {
		delete service;
	}

	void Execute() override {
		if (configurations.size() > 0) {
			std::vector<bool> results;
			for (const MonitorBrightnessConfiguration config: configurations) {
				results.emplace_back(service->SetMonitorBrightness(config.monitorId, config.brightness));
			}
			success = Every(results);
		} else {
			if (monitorId == ALL_MONITORS) {
				success = service->SetGlobalBrightness(brightness);
				return;
			} else {
				std::vector<Monitor> monitors = service->GetAvailableMonitors();
				for (const Monitor &monitor: monitors) {
					if (monitor.id == monitorId) {
						success = service->SetMonitorBrightness(monitor.id, brightness);
						return;
					}
				}
				success = false;
				message = "Monitor not found";
			}
		}
	}

	void OnOK() override {
		Napi::HandleScope scope(Env());
		deferred.Resolve(RetrieveSetBrightnessResult(Env(), success, message));
	}

private:
	MonitorService *service = new MonitorService();
	Napi::Promise::Deferred deferred;
	std::string monitorId;
	std::string message;
	bool success;
	int brightness;
	std::vector<MonitorBrightnessConfiguration> configurations;
};

#endif// SET_BRIGHTNESS_WORKER_H
