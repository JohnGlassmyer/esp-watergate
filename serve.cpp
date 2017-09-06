#ifndef SERVE_H
#define SERVE_H

#include <initializer_list>
#include <functional>
#include <string>
#include <vector>
#include <ESP8266WebServer.h>

#include "common.h"
#include "main.h"
#include "Logger.h"
#include "Temperature.h"
#include "Time.h"

void
withRequiredServerArgs(
		std::initializer_list<std::string const> argNames,
		std::function<void()> proceed) {
	for (auto &argName : argNames) {
		if (!webServer.hasArg(argName.c_str())) {
			std::string message = formatString(
					"missing arg '%s'",
					argName.c_str());
			logger.log(message);
			webServer.send(400, TEXT_PLAIN, message.c_str());

			return;
		}
	}

	proceed();
}

template<typename T>
void
withParsedServerArg(
		std::string const &argName,
		std::function<void(std::unique_ptr<T const>)> handleSuccess) {
	withRequiredServerArgs({argName}, [&] {
		T::parseFromString(to_string(webServer.arg(argName.c_str()))).handle(
				handleSuccess,
				[&argName] (std::string const &explanation) {
					std::string message = formatString(
							"failed to parse arg '%s'; %s",
							argName.c_str(),
							explanation.c_str());
					logger.log(message);
					webServer.send(400, TEXT_PLAIN, message.c_str());
				});
	});
}

void
serve_notFound() {
	webServer.send(404, TEXT_PLAIN, "404 Not Found Dumbass");
}

void
serve_get_log() {
	webServer.setContentLength(CONTENT_LENGTH_UNKNOWN);

	webServer.send(200, TEXT_PLAIN, "");

	logger.readStoredMessages([] (std::string const &message) {
		webServer.sendContent(message.c_str());
		webServer.sendContent("\n");
	});
}

void
serve_get_time() {
	if (timePtr) {
		webServer.send(200, TEXT_PLAIN, timePtr->toString().c_str());
	} else {
		webServer.send(503, TEXT_PLAIN, "time unavailable");
	}
}

void
serve_post_setTime() {
	withParsedServerArg<Time>(
			"time", [] (std::unique_ptr<Time const> newTimePtr) {
		setTime(*newTimePtr);
		webServer.sendHeader("Location", "/");
		webServer.send(303);
	});
}

void
serve_get_temperature() {
	if (temperaturePtr) {
		webServer.send(200, TEXT_PLAIN, temperaturePtr->toString().c_str());
	} else {
		webServer.send(503, TEXT_PLAIN, "temperature unavailable");
	}
}

void
serve_get_wateringSeconds() {
	webServer.send(200, TEXT_PLAIN,
			String(wateringControl.getRemainingSeconds()));
}

void
serve_post_startWatering() {
	withRequiredServerArgs({"seconds"}, [] {
		int seconds = webServer.arg("seconds").toInt();
		if (seconds < 30) {
			webServer.send(400, TEXT_PLAIN,
					formatString("seconds (%d) less than 30", seconds).c_str());
		} else {
			wateringControl.startWatering(seconds);

			webServer.sendHeader("Location", "/");
			webServer.send(303);
		}
	});
}

void
serve_post_stopWatering() {
	wateringControl.stopWatering();

	webServer.sendHeader("Location", "/");
	webServer.send(303);
}

void
serve_get_schedule() {
	std::string schedule = schedulePtr ? schedulePtr->toString() : "";
	webServer.send(200, TEXT_PLAIN, schedule.c_str());
}

void
serve_post_setSchedule() {
	withParsedServerArg<Schedule>(
			"schedule", [] (std::unique_ptr<Schedule const> newSchedulePtr) {
		schedulePtr.reset(newSchedulePtr.release());

		logger.log("set schedule: " + schedulePtr->toString());

		File scheduleFile = SPIFFS.open("/schedule", "w");
		if (scheduleFile) {
			scheduleFile.print(schedulePtr->toString().c_str());
			scheduleFile.close();
			logger.log("saved new schedule");
		} else {
			logger.log("failed to save new schedule");
		}

		webServer.sendHeader("Location", "/");
		webServer.send(303);
	});
}

void
serve_get_mapping() {
	std::string mapping = mappingPtr ? mappingPtr->toString() : "";
	webServer.send(200, TEXT_PLAIN, mapping.c_str());
}

void
serve_post_setMapping() {
	withParsedServerArg<Mapping>(
			"mapping", [] (std::unique_ptr<Mapping const> newMappingPtr) {
		mappingPtr.reset(newMappingPtr.release());

		logger.log("set mapping: " + mappingPtr->toString());

		File mappingFile = SPIFFS.open("/mapping", "w");
		if (mappingFile) {
			mappingFile.print(mappingPtr->toString().c_str());
			mappingFile.close();
			logger.log("saved new mapping");
		} else {
			logger.log("failed to save new mapping");
		}

		webServer.sendHeader("Location", "/");
		webServer.send(303);
	});
}

void
serve_post_setWifiConfig() {
	withRequiredServerArgs({"ssid", "passphrase"}, [] {
		auto result = WifiConfig::forSsidAndPassphrase(
				to_string(webServer.arg("ssid")),
				to_string(webServer.arg("passphrase")));
		result.handle([] (std::unique_ptr<WifiConfig const> wifiConfigPtr) {
			logger.log(
					"set wifiConfig (ssid: %s; passhrase length: %d)",
					wifiConfigPtr->ssid.c_str(),
					wifiConfigPtr->passphrase.length());

			setWifiConfig(wifiConfigPtr->ssid, wifiConfigPtr->passphrase);

			webServer.sendHeader("Location", "/");
			webServer.send(303);
		}, [] (std::string const &explanation) {
			std::string message = formatString(
					"failed to parse wifi config; %s",
					explanation.c_str());
			logger.log(message);
			webServer.send(400, TEXT_PLAIN, message.c_str());
		});
	});
}

void
serve_get_temperatureHistory() {
	webServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
	webServer.send(200, TEXT_PLAIN, "");

	for (int i = 9; i >= 0; i--) {
		auto logFilePath = formatString("/log%d", i);
		auto logFile = SPIFFS.open(logFilePath.c_str(), "r");
		while (logFile.available() > 0) {
			String line = logFile.readStringUntil('\n');
			if (line.indexOf("--") != -1) {
				// "yyyymmdd"
				webServer.sendContent(line.substring(9, 17));

				// "-hhmmss"
				webServer.sendContent(line.substring(19, 26));

				webServer.sendContent(",");

				// "nn.nnnn" (temperature)
				webServer.sendContent(line.substring(30));
			}
		}
		logFile.close();

		// maybe prevent watchdog timer from resetting system
		// not sure that this is necessary here
		yield();
	}
}

void
serve_post_deleteLogs() {
	std::vector<String> logFilenames;
	auto dir = SPIFFS.openDir("/");
	while (dir.next()) {
		if (dir.fileName().startsWith("/log")) {
			logFilenames.push_back(dir.fileName());
		}
	}

	for (String logFilename : logFilenames) {
		SPIFFS.remove(logFilename);
	}

	logger.log("deleted logs");

	webServer.sendHeader("Location", "/");
	webServer.send(303);
}

void
serve_get_ls() {
	webServer.setContentLength(CONTENT_LENGTH_UNKNOWN);

	webServer.send(200, TEXT_PLAIN, "");

	auto dir = SPIFFS.openDir("/");
	while (dir.next()) {
		auto file = dir.openFile("r");
		auto text = formatString("%10d %s\n", file.size(), file.name());
		webServer.sendContent(text.c_str());
	}
}

#endif
