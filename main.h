#ifndef MAIN_H
#define MAIN_H

#include <vector>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <FS.h>
#include <Wire.h>
#include <WString.h>

#include "serve.h"
#include "Mapping.h"
#include "Schedule.h"
#include "Time.h"
#include "Temperature.h"
#include "WateringControl.h"
#include "WifiConfig.h"

extern Logger logger;

extern WateringControl wateringControl;

extern std::unique_ptr<Time const> timePtr;

extern std::unique_ptr<Time const> previousTimePtr;

extern std::unique_ptr<Temperature const> temperaturePtr;

extern std::unique_ptr<Schedule const> schedulePtr;

extern std::unique_ptr<Mapping const> mappingPtr;

extern ESP8266WebServer webServer;

std::string to_string(String arduinoString);

void setTime(Time const &time);

void setWifiConfig(std::string const &ssid, std::string const &passphrase);

void doSetup();

void doLoop();

#endif
