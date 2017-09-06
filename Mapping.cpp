#include <algorithm>
#include <cstdio>
#include <memory>

#include "common.h"
#include "Logger.h"
#include "Mapping.h"

extern Logger logger;

Result<Mapping>
Mapping::parseFromString(std::string mappingString) {
	// e.g. "5:30, 10:60, 40:420"

	std::vector<MappingPoint> points;
	auto segments = splitString(mappingString, ", ");
	for (auto &segment : segments) {
		int degreesC;
		int seconds;
		if (std::sscanf(segment.c_str(), "%d:%d", &degreesC, &seconds) != 2) {
			return Result<Mapping>::failure(
					"Bad mapping point: " + segment);
		}

		Temperature temperature = Temperature::forDegreesC(degreesC);

		if (!(MIN_TEMPERATURE <= temperature
				&& temperature <= MAX_TEMPERATURE)) {
			return Result<Mapping>::failure(formatString(
					"Temperature out of range: %s",
					temperature.toString().c_str()));
		}

		if (!(MIN_DURATION <= seconds && seconds <= MAX_DURATION)) {
			return Result<Mapping>::failure(formatString(
					"Duration out of range: %d", seconds));
		}

		if (!points.empty() && temperature <= points.back().temperature) {
			return Result<Mapping>::failure(
					"Mapping points not in increasing order: " + segment);
		}

		points.push_back({temperature , seconds });
	}

	return Result<Mapping>::success(
			std::unique_ptr<Mapping>(new Mapping(points)));
}

int
Mapping::mapTemperatureToSeconds(Temperature const &temperature) const {
	auto lesserIt = std::find_if(
			points.rbegin(), points.rend(), [&] (MappingPoint const &point) {
		return point.temperature <= temperature;
	});
	MappingPoint const *lesser =
			lesserIt != points.rend() ? &(*lesserIt) : nullptr;

	auto greaterIt = std::find_if(
			points.begin(), points.end(), [&] (MappingPoint const &point) {
		return point.temperature >= temperature;
	});
	MappingPoint const *greater =
			greaterIt != points.end() ? &(*greaterIt) : nullptr;

	if (lesser && greater) {
		int temperatureDelta = temperature.degree16thsC
				- lesser->temperature.degree16thsC;
		int timeSpan = greater->seconds - lesser->seconds;
		int temperatureSpan = greater->temperature.degree16thsC
				- lesser->temperature.degree16thsC;
		int timeDelta = temperatureDelta * timeSpan / temperatureSpan;
		return lesser->seconds + timeDelta;
	} else if (greater) {
		return greater->seconds;
	} else if (lesser) {
		return lesser->seconds;
	} else {
		return 0;
	}
}

std::string
Mapping::toString() const {
	std::string string;

	for (auto it = points.begin(); it != points.end(); it++) {
		if (it != points.begin()) {
			string += ", ";
		}

		char segment[10];
		snprintf(segment, sizeof(segment), "%d:%d",
				it->temperature.degreesC(), it->seconds);
		string += segment;
	}

	return string;
}
