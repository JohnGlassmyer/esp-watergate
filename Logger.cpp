#include "Logger.h"

void
Logger::log(std::string const &message) {
	std::string shortMessage = message.length() <= maxMessageLength
			? message : message.substr(0, maxMessageLength);

	std::string formatted =
			formatString("%08d %s", nextMessageNumber, message.c_str());

	logTimeAcceptor(formatted);

	messages[nextMessageNumber % capacity] = formatted;

	nextMessageNumber++;
}

void
Logger::readStoredMessages(MESSAGE_ACCEPTOR messageAcceptor) {
	int count = (nextMessageNumber < capacity) ? nextMessageNumber : capacity;
	int startIndex = (nextMessageNumber - count) % capacity;

	for (int i = 0; i < count; i++) {
		int index = (startIndex + i) % capacity;
		messageAcceptor(messages[index]);
	}
}
