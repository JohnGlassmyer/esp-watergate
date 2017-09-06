#ifndef TIME_H
#define TIME_H

#include "Result.h"

class Time {
public:
	static Result<Time> parseFromString(std::string const &string);

public:
	int const year;
	uint8_t const month;
	uint8_t const day;
	uint8_t const weekday;
	uint8_t const hour;
	uint8_t const minute;
	uint8_t const second;

	Time(
			int year,
			uint8_t month,
			uint8_t day,
			uint8_t weekday,
			uint8_t hour,
			uint8_t minute,
			uint8_t second) :
			year(year),
			month(month),
			day(day),
			weekday(weekday),
			hour(hour),
			minute(minute),
			second(second) {}

	int secondsInDay() const {
        auto minutesInDay = hour * MINUTES_PER_HOUR + minute;
		return minutesInDay * SECONDS_PER_MINUTE + second;
	}

    std::string toString() const {
        return formatString("%04d%02d%02d-%1d-%02d%02d%02d",
                year, month, day, weekday, hour, minute, second);
    }
};

#endif
