#ifndef GET_BRIGHTNESS_WORKER_H
#define GET_BRIGHTNESS_WORKER_H

#include "../monitors.h"
#include "../utils.h"
#include <iostream>
#include <napi.h>

class GetBrightnessWorker : public Napi::AsyncWorker {
public:
	GetBrightnessWorker(Napi::Function &callback, Napi::Promise::Deferred &deferred, std::string &monitorId)
	    : Napi::AsyncWorker(callback), deferred(deferred), monitorId(monitorId), success(false), brightness(0) {}

	~GetBrightnessWorker() {
		delete service;
	}

	void Execute() override {
		std::vector<Monitor> monitors = service->GetAvailableMonitors();
		for (const Monitor &monitor: monitors) {
			if (monitor.id == monitorId) {
				brightness = service->GetMonitorBrightness(monitor.id);
				success = brightness != -1;
				return;
			}
		}
	}

	void OnOK() override {
		Napi::HandleScope scope(Env());
		deferred.Resolve(RetrieveGetBrightnessResult(Env(), success, brightness));
	}

private:
	MonitorService *service = new MonitorService();
	Napi::Promise::Deferred deferred;
	std::string monitorId;
	bool success;
	int brightness;
};

#endif// GET_BRIGHTNESS_WORKER_H
