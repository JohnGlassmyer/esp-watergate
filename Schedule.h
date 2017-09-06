#ifndef SCHEDULE_H
#define SCHEDULE_H

#include <vector>

#include "Result.h"

class Schedule {
public:
	static Result<Schedule> parseFromString(std::string string);

public:
	Schedule() {}

	std::string toString() const;

	bool hasReachedTime(int previousSecondsInDay, int secondsInDay) const;

private:
	std::vector<int> times;

	Schedule(std::vector<int> &times) : times(times) {}
};

#endif
