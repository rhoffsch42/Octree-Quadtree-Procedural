#pragma once

#include <string>
#include <chrono>

#define FOR(X, START, END)	for(size_t X = START; X < END; X++)
#define LF	"\n"

#define IND_3D_TO_1D(x, y, z, sizeX, sizeY)	size_t(z*sizeY*sizeX + y*sizeX + x)
#define IND_2D_TO_1D(x, y, sizeX)			size_t(y*sizeX + x)

#define FLAG_IS(X, Y)           (((X) & (Y)) == (X))
#define FLAG_HAS(X, Y)          (((X) & (Y)) != 0)
#define D_VALUE_NAME(x)			x << "\t" << #x << "\n"


class Timer {
public:
	void		setBegin();
	void		setEnd();

	template<typename T>
	long long	duration() const;

	long long	nanoseconds() const;
	long long	microseconds() const;
	long long	milliseconds() const;
	long long	seconds() const;
	long long	minutes() const;
	long long	hours() const;
private:
	std::chrono::steady_clock::time_point _begin;
	std::chrono::steady_clock::time_point _end;
};
