#ifndef MAPPING_H
#define MAPPING_H

#include <string>
#include <vector>

#include "common.h"
#include "Result.h"
#include "Temperature.h"

struct MappingPoint {
	Temperature temperature;
	int seconds;
};

class Mapping {
public:
	static Result<Mapping> parseFromString(std::string string);

private:
	constexpr static Temperature MIN_TEMPERATURE = Temperature::forDegreesC(0);
	constexpr static Temperature MAX_TEMPERATURE = Temperature::forDegreesC(50);
	constexpr static int MIN_DURATION = 30;
	constexpr static int MAX_DURATION = 15 * SECONDS_PER_MINUTE;

public:
	Mapping() {}

	int mapTemperatureToSeconds(Temperature const &temperature) const;

	std::string toString() const;

private:
	std::vector<MappingPoint> points;

	Mapping(std::vector<MappingPoint> const &points) : points(points) {}
};

#endif
