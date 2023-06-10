#ifndef SET_BRIGHTNESS_WORKER_H
#define SET_BRIGHTNESS_WORKER_H

#include "../monitor_service.h"
#include "../utils.h"
#include <iostream>
#include <napi.h>
#include <thread>

class SetBrightnessWorker : public Napi::AsyncWorker {
public:
	SetBrightnessWorker(Napi::Function &callback, Napi::Promise::Deferred &deferred, std::string &monitorId,
	                    int brightness)
	    : Napi::AsyncWorker(callback), deferred(deferred), monitorId(monitorId), success(false), brightness(brightness), message("") {}

public:
	SetBrightnessWorker(Napi::Function &callback, Napi::Promise::Deferred &deferred, std::vector<MonitorBrightnessConfiguration> &configurations) : Napi::AsyncWorker(callback), deferred(deferred), success(false), configurations(configurations), message("") {}

	~SetBrightnessWorker() {}

	void Execute() override {
		std::thread thread([&]() {
			MonitorService *service = new MonitorService();
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

					if (monitors.size() == 0) {
						message = "No monitors available";
						return;
					}

					if (monitorId.empty()) {
						success = service->SetMonitorBrightness(monitors[0].id, brightness);
					} else {
						for (const Monitor &monitor: monitors) {
							if (monitor.id == monitorId) {
								success = service->SetMonitorBrightness(monitor.id, brightness);
								return;
							}
						}
						message = "Monitor not found";
					}
				}
			}
		});

		thread.join();
	}

	void OnOK() override {
		Napi::HandleScope scope(Env());
		deferred.Resolve(RetrieveSetBrightnessResult(Env(), success, message));
	}

private:
	Napi::Promise::Deferred deferred;
	std::string monitorId;
	std::string message;
	bool success;
	int brightness;
	std::vector<MonitorBrightnessConfiguration> configurations;
};

#endif// SET_BRIGHTNESS_WORKER_H
