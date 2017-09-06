#include "Logger.h"
#include "RequestLogger.h"

extern Logger logger;

bool
RequestLogger::canHandle(
		HTTPMethod requestMethod, String requestUri) {
	logger.log(formatString(
			"HTTP %s to %s",
			httpMethodNames[requestMethod].c_str(),
			requestUri.c_str()));

	return false;
}

bool
RequestLogger::canUpload(String requestUri) {
	logger.log(formatString("HTTP UPLOAD to %s", requestUri.c_str()));

	return false;
}

std::vector<std::string>
RequestLogger::httpMethodNames {
	"ANY", "GET", "POST", "PUT", "PATCH", "DELETE", "OPTIONS"
};
