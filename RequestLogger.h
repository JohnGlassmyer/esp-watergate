#ifndef REQUEST_LOGGER_H
#define REQUEST_LOGGER_H

#include <vector>
#include <ESP8266WebServer.h>

class RequestLogger : public RequestHandler {
private:
	static std::vector<std::string> httpMethodNames;

public:
	bool canHandle(HTTPMethod requestMethod, String requestUri) override;

	bool canUpload(String requestUri) override;
};

#endif
