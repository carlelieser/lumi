#ifndef GET_BRIGHTNESS_WORKER_H
#define GET_BRIGHTNESS_WORKER_H

#include "../monitor_service.h"
#include "../utils.h"
#include <iostream>
#include <napi.h>
#include <thread>

class GetBrightnessWorker : public Napi::AsyncWorker {
public:
	GetBrightnessWorker(Napi::Function &callback, Napi::Promise::Deferred &deferred, std::string &monitorId)
	    : Napi::AsyncWorker(callback), deferred(deferred), monitorId(monitorId), success(false), brightness(0) {}

	~GetBrightnessWorker() {}

	void Execute() override {
		std::thread thread([&]() {
			MonitorService *service = new MonitorService();
			std::vector<Monitor> monitors = service->GetAvailableMonitors();

			if (monitorId.empty()) {
				if (monitors.size() > 0) {
					brightness = service->GetMonitorBrightness(monitors[0].id);
					success = brightness != -1;
					return;
				}
			}

			for (const Monitor &monitor: monitors) {
				if (monitor.id == monitorId) {
					brightness = service->GetMonitorBrightness(monitor.id);
					success = brightness != -1;
					return;
				}
			}
		});

		thread.join();
	}

	void OnOK() override {
		Napi::HandleScope scope(Env());
		deferred.Resolve(RetrieveGetBrightnessResult(Env(), success, brightness));
	}

private:
	Napi::Promise::Deferred deferred;
	std::string monitorId;
	bool success;
	int brightness;
};

#endif// GET_BRIGHTNESS_WORKER_H
