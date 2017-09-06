#ifndef TEMPERATURE_H
#define TEMPERATURE_H

class Temperature {
public:
	constexpr static Temperature forDegreesC(int degreesC) {
		return Temperature(degreesC * 16);
	}

	constexpr static Temperature forDegree16thsC(int degree16thsC) {
		return Temperature(degree16thsC);
	}

public:
	int const degree16thsC;

	int degreesC() const {
		return degree16thsC / 16;
	}

	std::string toString() const {
		int tenThousandths = degree16thsC % 16 * 10000 / 16;
		return formatString("%d.%04d", degreesC(), tenThousandths);
	}

	// Too much boilerplate in the comparison overloads.

	bool operator<(Temperature const &other) const {
		return degree16thsC < other.degree16thsC;
	}

	bool operator<=(Temperature const &other) const {
		return degree16thsC <= other.degree16thsC;
	}

	bool operator!=(Temperature const &other) const {
		return degree16thsC != other.degree16thsC;
	}

	bool operator==(Temperature const &other) const {
		return degree16thsC == other.degree16thsC;
	}

	bool operator>=(Temperature const &other) const {
		return degree16thsC >= other.degree16thsC;
	}

	bool operator>(Temperature const &other) const {
		return degree16thsC > other.degree16thsC;
	}

private:
	constexpr Temperature(int degree16thsC) : degree16thsC(degree16thsC) {}
};

#endif
