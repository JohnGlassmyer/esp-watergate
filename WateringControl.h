#ifndef WATERING_CONTROL_H
#define WATERING_CONTROL_H

#include <functional>

class WateringControl {
public:
	WateringControl(
			std::function<void()> powerRelay,
			std::function<void()> unpowerRelay) :
			powerRelay(powerRelay),
			unpowerRelay(unpowerRelay),
			remainingSeconds(0) {}

	bool isWatering() const { return remainingSeconds > 0; }

	int getRemainingSeconds() const { return remainingSeconds; }

	void startWatering(int seconds);

	void stopWatering();

	void elapseSeconds(int seconds);

private:
	std::function<void()> powerRelay;
	std::function<void()> unpowerRelay;
	int remainingSeconds;
};

#endif
