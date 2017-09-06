#include <algorithm>
#include <cstdio>
#include <memory>
#include <vector>

#include "common.h"
#include "Schedule.h"

Result<Schedule>
Schedule::parseFromString(std::string scheduleString) {
	// e.g. "1100, 1300, 1500, 1700"

	std::vector<int> times;
	auto segments = splitString(scheduleString, ", ");
	for (auto &segment : segments) {
		int hour;
		int minute;
		if (segment.length() != 4 ||
				std::sscanf(segment.c_str(), "%02d%02d", &hour, &minute) != 2) {
			return Result<Schedule>::failure(
					"Bad time: %s" + segment);
		}

		if (!(0 <= hour && hour < HOURS_PER_DAY)) {
			return Result<Schedule>::failure(
					"Hour out of range: " + segment);
		}

		if (!(0 <= minute && minute < MINUTES_PER_HOUR)) {
			return Result<Schedule>::failure(
					"Minute out of range: " + segment);
		}

		int time = ((hour * MINUTES_PER_HOUR) + minute) * SECONDS_PER_MINUTE;

		if (!times.empty() && time <= times.back()) {
			return Result<Schedule>::failure(
					"Not in increasing order: " + segment);
		}

		times.push_back(time);
	}

	return Result<Schedule>::success(
			std::unique_ptr<Schedule>(new Schedule(times)));
}

bool
Schedule::hasReachedTime(int previousSecondsInDay, int secondsInDay) const {
	if (previousSecondsInDay == secondsInDay) {
		return false;
	}

	bool crossedMidnight = secondsInDay < previousSecondsInDay;

	return std::find_if(times.begin(), times.end(), [=] (int time) {
		bool startWasLater = previousSecondsInDay < time;
		bool startIsEarlier = time <= secondsInDay;
		return (startWasLater && startIsEarlier
				|| (crossedMidnight && (startWasLater || startIsEarlier)));
	}) != times.end();
}

std::string
Schedule::toString() const {
	std::string string;

	for (auto it = times.begin(); it != times.end(); it++) {
		if (it != times.begin()) {
			string += ", ";
		}

		int minuteOfDay = *it / SECONDS_PER_MINUTE;
		int hour = minuteOfDay / MINUTES_PER_HOUR;
		int minute = minuteOfDay % MINUTES_PER_HOUR;

		char segment[5];
		snprintf(segment, sizeof(segment), "%02d%02d", hour, minute);
		string += segment;
	}

	return string;
}
