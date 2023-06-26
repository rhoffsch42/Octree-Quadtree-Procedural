#pragma once

#include <string>
#include <chrono>

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
