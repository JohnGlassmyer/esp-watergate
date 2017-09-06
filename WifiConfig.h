#ifndef WifiConfig_H
#define WifiConfig_H

#include "common.h"
#include "Result.h"

class WifiConfig {
public:
	static Result<WifiConfig> forSsidAndPassphrase(
			std::string const &ssid,
			std::string const &passphrase);

public:
	std::string const ssid;
	std::string const passphrase;

private:
	WifiConfig(std::string const &ssid, std::string const &passphrase) :
			ssid(ssid), passphrase(passphrase) {}
};

#endif
