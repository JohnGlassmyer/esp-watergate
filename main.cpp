#include "main.h"
#include "RequestLogger.h"

extern "C" {
	#include "user_interface.h"
}

void writeToLogFile(std::string const &message);

constexpr int LOGGER_LINES_TO_STORE = 100;
constexpr int LOGGER_MAX_LINE_LENGTH = 100;

constexpr int MAX_LOG_FILE_SIZE = 20000;

constexpr int RECORDING_INTERVAL_SECONDS = 15 * SECONDS_PER_MINUTE;

constexpr int WATERING_RELAY_PIN = 9;
constexpr int ERROR_STATUS_PIN = 10;

constexpr int SDA_PIN = 4;
constexpr int SCL_PIN = 5;

constexpr int RESET_WIFI_BUTTON_PIN = 14;
constexpr int RESET_WIFI_LED_PIN = 16;

constexpr int RESET_WIFI_MILLISECONDS = 2000;

constexpr int TEMPERATURE_SENSOR_ADDRESS = 0b0011000 + 0b000;
constexpr int REAL_TIME_CLOCK_ADDRESS = 0b1101000;

constexpr auto WIFI_STATION_SSID = "esp-watergate";
constexpr auto WIFI_DEFAULT_PASSPHRASE = "johnwdean";

IPAddress const AP_IP_ADDRESS(192, 168, 4, 1);
IPAddress const AP_NETMASK(255, 255, 255, 0);

Logger logger([] (std::string const &formattedMessage) {
	Serial.println(formattedMessage.c_str());
	writeToLogFile(formattedMessage);
		},
		LOGGER_MAX_LINE_LENGTH,
		LOGGER_LINES_TO_STORE);

WateringControl wateringControl(
		[] { digitalWrite(WATERING_RELAY_PIN, HIGH); },
		[] { digitalWrite(WATERING_RELAY_PIN, LOW); });

std::unique_ptr<Time const> timePtr;
std::unique_ptr<Time const> previousTimePtr;

std::unique_ptr<Temperature const> temperaturePtr;

std::unique_ptr<Schedule const> schedulePtr;

std::unique_ptr<Mapping const> mappingPtr;

ESP8266WebServer webServer(80);

bool wifiNeedsUpdating;

int accumulatedWateringSeconds;

std::string
to_string(String arduinoString) {
	return std::string(arduinoString.c_str());
}

void
rotateLogFiles() {
	for (int i = 8; i >= 0; i--) {
		auto oldName = formatString("/log%d", i);
		auto newName = formatString("/log%d", i + 1);
		SPIFFS.rename(oldName.c_str(), newName.c_str());
	}
}

int
sizeOfFile(std::string const &path) {
	auto file = SPIFFS.open(String(path.c_str()), "r");
	if (file) {
		auto size = file.size();
		file.close();
		return size;
	} else {
		return 0;
	}
}

void
writeToLogFile(std::string const &message) {
	if (sizeOfFile("/log0") >= MAX_LOG_FILE_SIZE) {
		rotateLogFiles();
	}

	File logFile = SPIFFS.open(String("/log0"), "a");
	if (logFile) {
		logFile.println(message.c_str());
		logFile.close();
	}
}

std::vector<int>
readI2cBytes(int i2cAddress, int startRegister, int count) {
	Wire.beginTransmission(i2cAddress);
	Wire.write(startRegister);
	Wire.endTransmission(false);
	Wire.requestFrom(i2cAddress, count);

	std::vector<int> bytes;
	while (Wire.available()) {
		bytes.push_back(Wire.read());
	}

	return bytes;
}

void
writeI2cBytes(
		int i2cAddress, int startRegister, std::vector<int> const &bytes) {
	Wire.beginTransmission(i2cAddress);
	Wire.write(startRegister);

	for (auto byte : bytes) {
		Wire.write(byte);
	}

	Wire.endTransmission(true);
}

int
fromBcd(int bcdValue) {
	return (bcdValue / 16) * 10 + bcdValue % 16;
}

int
toBcd(int value) {
	return (value / 10) * 16 + value % 10;
}

void
setTime(Time const &time) {
	std::vector<int> bytes;

	bytes.push_back(time.second);
	bytes.push_back(time.minute);
	bytes.push_back(time.hour);
	bytes.push_back(time.weekday);
	bytes.push_back(time.day);

	int centuryBit = ((time.year / 100) - 19) ? 0x80 : 0;
	bytes.push_back(toBcd(time.month) + centuryBit);

	bytes.push_back(toBcd(time.year % 100));

	writeI2cBytes(REAL_TIME_CLOCK_ADDRESS, 0, bytes);

	logger.log("set time to %s", time.toString().c_str());

	previousTimePtr.reset();
}

Result<Time>
readTime() {
	auto timeBytes = readI2cBytes(REAL_TIME_CLOCK_ADDRESS, 0, 7);

	if (timeBytes.size() != 7) {
		return Result<Time>::failure("real-time clock unavailable");
	}

	int second = fromBcd(timeBytes[0]);
	int minute = fromBcd(timeBytes[1]);
	int hour = fromBcd(timeBytes[2]);
	int weekday = fromBcd(timeBytes[3]);
	int day = fromBcd(timeBytes[4]);

	int centuryMonthByte = timeBytes[5];

	int month = fromBcd(centuryMonthByte & ~0x80);

	int year = (centuryMonthByte & 0x80 ? 2000 : 1900) + fromBcd(timeBytes[6]);

	return Result<Time>::success(std::unique_ptr<Time>(
			new Time(year, month, day, weekday, hour, minute, second)));
}

Result<Temperature>
readTemperature() {
	auto temperatureBytes = readI2cBytes(TEMPERATURE_SENSOR_ADDRESS, 5, 2);

	if (temperatureBytes.size() != 2) {
		return Result<Temperature>::failure("temperature sensor unavailable");
	}

	int tempRaw = (temperatureBytes[0] & 0b00011111) * 256
			+ temperatureBytes[1];

	int temp16ths = tempRaw & 0b0000111111111111;
	if (tempRaw & 0b0001000000000000) {
		temp16ths = -temp16ths;
	}

	return Result<Temperature>::success(std::unique_ptr<Temperature>(
			new Temperature(Temperature::forDegree16thsC(temp16ths))));
}

void
withFileContents(
		std::string const &filename,
		std::function<void(std::string const &)> useContents) {
	auto filenameCStr = filename.c_str();
	if (SPIFFS.exists(filenameCStr)) {
		File file = SPIFFS.open(filenameCStr, "r");
		if (file) {
			std::string contents(to_string(file.readString()));
			file.close();
			logger.log("read %s (length: %d)", filenameCStr, contents.length());
			useContents(contents);
		} else {
			logger.log("failed to open file " + filename);
		}
	} else {
		logger.log("no file named " + filename);
	}
}

template<typename T>
void
withParsedFile(
		std::string const &filename,
		std::function<void(std::unique_ptr<T const>)> handleContents) {
	withFileContents(filename, [&] (std::string const &contents) {
		T::parseFromString(contents).handle(
				handleContents,
				[&filename] (std::string const &explanation) {
					logger.log(
							"failed to parse file '%s'; %s",
							filename.c_str(),
							explanation.c_str());
				});
	});
}

void
updateWifi() {
	WiFi.persistent(false);
	WiFi.disconnect();
	WiFi.softAPdisconnect(false);
	WiFi.enableAP(false);
	WiFi.mode(WIFI_OFF);
	WiFi.setAutoConnect(false);
	WiFi.setAutoReconnect(true);

	std::string ssid;
	std::string passphrase(WIFI_DEFAULT_PASSPHRASE);

	withFileContents("/wifiConfig", [&] (std::string const &contents) {
		auto lines = splitString(contents, "\n");
		ssid = lines[0];
		passphrase = lines[1];
		logger.log("restored wifiConfig (ssid: %s)", ssid.c_str());
	});

	if (ssid.empty()) {
		logger.log("starting WiFi station %s", WIFI_STATION_SSID);
		WiFi.mode(WIFI_AP);
		WiFi.softAPConfig(AP_IP_ADDRESS, AP_IP_ADDRESS, AP_NETMASK);
		WiFi.softAP(WIFI_STATION_SSID, passphrase.c_str());
		wifi_softap_dhcps_start();
	} else {
		logger.log("connecting to WiFi station %s", ssid.c_str());
		WiFi.mode(WIFI_STA);
		WiFi.begin(ssid.c_str(), passphrase.c_str());
	}

	WiFi.printDiag(Serial);
}

void
resetWifiConfig() {
	for (int i = 0; i < 8; i++) {
		digitalWrite(RESET_WIFI_LED_PIN, i % 2 ? HIGH : LOW);
		delay(500);
	}

	logger.log("resetting WiFi config to default");

	if (SPIFFS.remove("/wifiConfig")) {
		logger.log("removed /wifiConfig");
	} else {
		logger.log("could not remove /wifiConfig");
	}

	wifiNeedsUpdating = true;
}

void
setWifiConfig(std::string const &ssid, std::string const &passphrase) {
	File wifiConfigFile = SPIFFS.open("/wifiConfig", "w");
	if (wifiConfigFile) {
		wifiConfigFile.print(ssid.c_str());
		wifiConfigFile.print("\n");
		wifiConfigFile.print(passphrase.c_str());
		wifiConfigFile.close();

		logger.log("saved new wifiConfig");
	} else {
		logger.log("failed to save new wifiConfig");
	}

	wifiNeedsUpdating = true;
}

void
triggerWatering() {
	if (!temperaturePtr) {
		logger.log("triggerWatering, but temperature unavailable");
	} else if (!mappingPtr) {
		logger.log("triggerWatering, but no mapping set");
	} else {
		int mappedSeconds =
					mappingPtr->mapTemperatureToSeconds(*temperaturePtr);
		logger.log("triggerWatering: mapped %s°C to %ds",
				temperaturePtr->toString().c_str(),
				mappedSeconds);

		// don't water for <30s at a time,
		// but do account for previous <30s chunks
		mappedSeconds += accumulatedWateringSeconds;
		if (mappedSeconds >= 30) {
			wateringControl.startWatering(mappedSeconds);
		} else {
			accumulatedWateringSeconds = mappedSeconds;
			logger.log("duration too short; %d seconds accumulated",
					accumulatedWateringSeconds);
		}
	}
}

std::function<void(void)>
streamFromSpiffs(
		std::string const &uri,
		std::function<std::string(void)> const &pathProvider,
		std::string const &contentType) {
	webServer.on(uri.c_str(), HTTP_GET, [=] {
		File file = SPIFFS.open(pathProvider().c_str(), "r");
		if (file) {
			webServer.streamFile(file, contentType.c_str());
			file.close();
		} else {
			serve_notFound();
		}
	});
}

std::function<std::string(void)>
uri() {
	return [] {
		return std::string(webServer.uri().c_str());
	};
}

std::function<std::string(void)>
path(std::string const &path) {
	return [=] {
		return path;
	};
}

void
doSetup() {
	Serial.begin(9600);
	Serial.setDebugOutput(true);

	SPIFFS.begin();

	logger.log("ESP-Watergate start");

	pinMode(WATERING_RELAY_PIN, OUTPUT);
	pinMode(ERROR_STATUS_PIN, OUTPUT);

	pinMode(RESET_WIFI_BUTTON_PIN, INPUT);
	pinMode(RESET_WIFI_LED_PIN, OUTPUT);

	wateringControl.stopWatering();

	accumulatedWateringSeconds = 0;

	Wire.begin(SDA_PIN, SCL_PIN);

	// set CONFIG to 0, 0 (default)
	writeI2cBytes(TEMPERATURE_SENSOR_ADDRESS, 0, {0, 0});

	// set temperature resolution to 0.0625 C (default)
	writeI2cBytes(TEMPERATURE_SENSOR_ADDRESS, 8, {0b11});

	withParsedFile<Mapping>(
			"/mapping", [] (std::unique_ptr<Mapping const> savedMappingPtr) {
		mappingPtr.reset(savedMappingPtr.release());
		logger.log("restored mapping: " + mappingPtr->toString());
	});

	withParsedFile<Schedule>(
			"/schedule", [] (std::unique_ptr<Schedule const> savedSchedulePtr) {
		schedulePtr.reset(savedSchedulePtr.release());
		logger.log("restored schedule: " + schedulePtr->toString());
	});

	updateWifi();
	wifiNeedsUpdating = false;

	webServer.addHandler(new RequestLogger());

	webServer.onNotFound(serve_notFound);

	streamFromSpiffs("/", path("/index.html"), TEXT_HTML);
	streamFromSpiffs("/jquery-min.js", uri(), APPLICATION_JAVASCRIPT);
	streamFromSpiffs("/svg.min.js", uri(), APPLICATION_JAVASCRIPT);

	for (int i = 0; i <= 9; i++) {
		streamFromSpiffs(formatString("/log%d", i), uri(), TEXT_PLAIN);
	}

	webServer.on("/log", HTTP_GET, serve_get_log);
	webServer.on("/time", HTTP_GET, serve_get_time);
	webServer.on("/setTime", HTTP_POST, serve_post_setTime);
	webServer.on("/temperature", HTTP_GET, serve_get_temperature);
	webServer.on("/wateringSeconds", HTTP_GET, serve_get_wateringSeconds);
	webServer.on("/startWatering", HTTP_POST, serve_post_startWatering);
	webServer.on("/stopWatering", HTTP_POST, serve_post_stopWatering);
	webServer.on("/schedule", HTTP_GET, serve_get_schedule);
	webServer.on("/setSchedule", HTTP_POST, serve_post_setSchedule);
	webServer.on("/mapping", HTTP_GET, serve_get_mapping);
	webServer.on("/setMapping", HTTP_POST, serve_post_setMapping);
	webServer.on("/setWifiConfig", HTTP_POST, serve_post_setWifiConfig);
	webServer.on("/temperatureHistory", HTTP_GET, serve_get_temperatureHistory);
	webServer.on("/deleteLogs", HTTP_POST, serve_post_deleteLogs);
	webServer.on("/ls", HTTP_GET, serve_get_ls);

	webServer.begin();
}

void
doLoop() {
	static unsigned long lastMillis = 0;
	if (millis() - lastMillis > MILLIS_PER_SECOND) {
		lastMillis += MILLIS_PER_SECOND;
		if (wateringControl.isWatering()) {
			wateringControl.elapseSeconds(1);
		}
	}

	readTemperature().handle(
			[&] (std::unique_ptr<Temperature const> newTemperaturePtr) {
		temperaturePtr.swap(newTemperaturePtr);
	}, [&] (std::string const &explanation) {
		temperaturePtr.reset();
	});

	previousTimePtr.reset(timePtr.release());

	readTime().handle(
			[&] (std::unique_ptr<Time const> newTimePtr) {
		timePtr.reset(newTimePtr.release());

		if (!previousTimePtr) {
			logger.log("time now available: %s", timePtr->toString().c_str());
		}
	}, [] (std::string const &explanation) {
		if (previousTimePtr) {
			logger.log("time unavailable; previous time: %s",
					previousTimePtr->toString().c_str());
		}
	});

	if (timePtr && previousTimePtr
			&& timePtr->secondsInDay() != previousTimePtr->secondsInDay()) {
		// start watering if scheduled time has been crossed
		if (schedulePtr && schedulePtr->hasReachedTime(
				previousTimePtr->secondsInDay(), timePtr->secondsInDay())) {
			logger.log("reached scheduled time; previous: %s, now: %s",
					previousTimePtr->toString().c_str(),
					timePtr->toString().c_str());
			triggerWatering();
		}

		// record time and temperature
		if (timePtr->secondsInDay() % RECORDING_INTERVAL_SECONDS == 0) {
			auto temperatureString = temperaturePtr
					? temperaturePtr->toString() : "xx";

			logger.log("%s -- %s",
					timePtr->toString().c_str(),
					temperatureString.c_str());
		}
	}

	digitalWrite(ERROR_STATUS_PIN, (timePtr && temperaturePtr) ? LOW : HIGH);

	// respond to state of reset-wifi button
	static unsigned long lastResetWifiLowMillis = 0;
	if (digitalRead(RESET_WIFI_BUTTON_PIN) == HIGH) {
		if (millis() - lastResetWifiLowMillis > RESET_WIFI_MILLISECONDS) {
			resetWifiConfig();
		}
		digitalWrite(RESET_WIFI_LED_PIN, LOW);
	} else {
		lastResetWifiLowMillis = millis();
		digitalWrite(RESET_WIFI_LED_PIN, HIGH);
	}

	if (wifiNeedsUpdating) {
		updateWifi();
		wifiNeedsUpdating = false;
	}

	webServer.handleClient();

	// maybe prevent overheating
	delay(10);
}
