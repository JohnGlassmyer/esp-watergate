#include "Logger.h"
#include "WateringControl.h"

extern Logger logger;

void
WateringControl::startWatering(int seconds) {
	logger.log("startWatering(%d)", seconds);

	if (powerRelay) {
		powerRelay();
	}

	remainingSeconds = seconds;
}

void
WateringControl::stopWatering() {
	logger.log("stopWatering() (remainingSeconds: %d)", remainingSeconds);

	remainingSeconds = 0;

	if (unpowerRelay) {
		unpowerRelay();
	}
}

void
WateringControl::elapseSeconds(int seconds) {
	if (remainingSeconds > 0) {
		remainingSeconds -= seconds;

		if (remainingSeconds <= 0) {
			stopWatering();
		}
	}
}
