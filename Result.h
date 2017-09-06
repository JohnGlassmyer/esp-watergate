#ifndef RESULT_H
#define RESULT_H

#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "Logger.h"

extern Logger logger;

template<typename T>
class Result {
	/* TODO: Probably there are better ways to do this in C++, but I'm not
            particularly familiar with them, and it seems that they require much
            more code. Probably would be easier to do this in C++17.
    */
public:
	static Result<T> success (std::unique_ptr<T const> valuePtr) {
		return Result(true, std::move(valuePtr), "");
	}

	static Result<T> failure(std::string const &explanation) {
		return Result(false, std::unique_ptr<T const>(), explanation);
	}

public:
	bool const isSuccess;

	void handle(
			std::function<void(std::unique_ptr<T const>)> handleSuccess,
			std::function<void(std::string const &)> handleFailure) {
		if (isSuccess) {
			handleSuccess(std::move(valuePtr));
		} else {
			handleFailure(explanation);
		}
	}

private:
	std::unique_ptr<T const> valuePtr;
	std::string const explanation;

	Result(
			bool isSuccess,
			std::unique_ptr<T const> valuePtr,
			std::string explanation) :
			isSuccess(isSuccess),
			valuePtr(std::move(valuePtr)),
			explanation(explanation) {}
};

#endif // RESULT_H
