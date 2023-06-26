#include "utils.hpp"
#include <sstream>

void	Timer::setBegin() {	this->_begin = std::chrono::high_resolution_clock::now(); }
void	Timer::setEnd() { this->_end = std::chrono::high_resolution_clock::now(); }

template<typename T>
long long	Timer::duration() const { return std::chrono::duration_cast<T>(this->_end - this->_begin).count(); }

// gets
long long	Timer::nanoseconds() const { return this->duration<std::chrono::nanoseconds>(); }
long long	Timer::microseconds() const { return this->duration<std::chrono::microseconds>(); }
long long	Timer::milliseconds() const { return this->duration<std::chrono::milliseconds>(); }
long long	Timer::seconds() const { return this->duration<std::chrono::seconds>(); }
long long	Timer::minutes() const { return this->duration<std::chrono::minutes>(); }
long long	Timer::hours() const { return this->duration<std::chrono::hours>(); }

// explicit declarations (required if the aboves 'gets' are removed
//template long long Timer::duration<std::chrono::nanoseconds>() const;
//template long long Timer::duration<std::chrono::microseconds>() const;
//template long long Timer::duration<std::chrono::milliseconds>() const;
//template long long Timer::duration<std::chrono::seconds>() const;
//template long long Timer::duration<std::chrono::minutes>() const;
//template long long Timer::duration<std::chrono::hours>() const;
