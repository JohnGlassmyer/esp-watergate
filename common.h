#ifndef COMMON_H
#define COMMON_H

#include <cstdio>
#include <memory>
#include <vector>

int const HOURS_PER_DAY = 24;
int const MINUTES_PER_HOUR = 60;
int const SECONDS_PER_MINUTE = 60;
int const MILLIS_PER_SECOND = 1000;

int const SECONDS_PER_DAY =
		SECONDS_PER_MINUTE * MINUTES_PER_HOUR * HOURS_PER_DAY;

constexpr auto APPLICATION_JAVASCRIPT = "application/javascript";
constexpr auto TEXT_HTML = "text/html";
constexpr auto TEXT_PLAIN = "text/plain";

std::vector<std::string> splitString(
		std::string const &string, char const *delimiterChars);

template<typename ... ARGS>
std::string
formatString(std::string const &format, ARGS ... args) {
	size_t size = snprintf(nullptr, 0, format.c_str(), args ...) + 1;
	std::unique_ptr<char[]> buffer(new char[size]);
	snprintf(buffer.get(), size, format.c_str(), args ...);
	return std::string(buffer.get());
}

#endif
