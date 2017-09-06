#include <cstdio>
#include <memory>

#include "common.h"
#include "Time.h"

Result<Time>
Time::parseFromString(std::string const &string) {
	int year, month, day, weekday, hour, minute, second;
	if (std::sscanf(string.c_str(), "%04d%02d%02d-%01d-%02d%02d%02d",
			&year, &month, &day, &weekday, &hour, &minute, &second) == 7) {
		return Result<Time>::success(std::unique_ptr<Time>(
				new Time(year, month, day, weekday, hour, minute, second)));
	} else {
		return Result<Time>::failure("could not parse Time");
	}
}
