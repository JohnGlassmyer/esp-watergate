#include <string>

#include "common.h"

std::vector<std::string>
splitString(std::string const &string, char const *delimiterChars) {
	std::vector<std::string> segmentStrings;

	if (string.find_first_of(delimiterChars) == 0) {
		// recognize zero-length segment at start of string
		segmentStrings.push_back("");
	}

	size_t start = 0;
	size_t end = 0;
	while (start != std::string::npos) {
		start = string.find_first_not_of(delimiterChars, end);
		if (start != std::string::npos) {
			// recognize delimited segment
			end = string.find_first_of(delimiterChars, start);
			if (end == std::string::npos) {
				end = string.length();
			}

			segmentStrings.push_back(string.substr(start, end - start));
		}
	}

	if (string.find_last_of(delimiterChars) == string.length() - 1) {
		// recognize zero-length segment at end of string
		segmentStrings.push_back("");
	}

	return segmentStrings;
}
