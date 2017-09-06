# esp-watergate
An ESP8266-based, temperature-responsive electric-valve controller for plant irrigation. Set the time and date, specify a few start-times and a temperature-duration mapping, and the unit will power a relay at the specified times to control an electric valve to provide a temperature-dependent amount of water to garden plants.

Runs on the whizbang ESP8266 WiFi-capable microcontroller as contained within the (inexpensive and easy-to-use) NodeMCU 1.0 DevKit.

Makes use of the ESP8266 Arduino project. Firmware can be built and flashed from the Arduino environment. Static files (HTML and Javascript) can be flashed with the ESP8266 Arduino Sketch Data Uploader.

C++ code runs on the microcontroller and supports an HTML/Javascript web frontend (accessed over WiFi) for monitoring and configuration.

Out of the box, the device automatically hosts its own WiFi access point, but it can be configured to connect instead to an established (WPA/WPA2) access point.

Uses an I2C real-time clock, Maxim DS3231, and an I2C temperature sensor, Microchip MCP9808 (both common and inexpensive).

Required hardware setup details are listed in the frontend HTML document (data/index.html). All parts should total less than $30. Hobbyist electronics ability required to assemble. If intending to switch mains voltage, don't electrocute yourself.
