#ifndef LOGGER_H
#define LOGGER_H

#include <string>

#include "common.h"

class Logger {
public:
	typedef std::function<void(std::string const &)> MESSAGE_ACCEPTOR;

public:
	Logger(
			MESSAGE_ACCEPTOR logTimeAcceptor,
			int maxMessageLength,
			int capacity) :
		logTimeAcceptor(logTimeAcceptor),
		maxMessageLength(maxMessageLength),
		capacity(capacity),
		messages(new std::string[capacity]),
		nextMessageNumber(0) {}

	void log(std::string const &message);

	template <typename ... ARGS>
	void log(std::string const &format, ARGS ... args) {
		log(formatString(format, args ...));
	}

	void readStoredMessages(MESSAGE_ACCEPTOR messageAcceptor);

private:
	MESSAGE_ACCEPTOR logTimeAcceptor;
	int const maxMessageLength;
	int const capacity;
	std::unique_ptr<std::string[]> messages;
	int nextMessageNumber;
};

#endif
