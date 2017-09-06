#include <vector>

#include "WifiConfig.h"

Result<WifiConfig>
WifiConfig::forSsidAndPassphrase(
		std::string const &ssid,
		std::string const &passphrase) {
	if (ssid.length() != 0 && ssid.length() > 31) {
		return Result<WifiConfig>::failure("ssid too long");
	} else if (passphrase.length() < 8) {
		return Result<WifiConfig>::failure("passphrase too short");
	} else if (passphrase.length() > 63) {
		return Result<WifiConfig>::failure("passphrase too long");
	}

	return Result<WifiConfig>::success(
			std::unique_ptr<WifiConfig>(new WifiConfig(ssid, passphrase)));
}
