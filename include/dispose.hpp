#pragma once

#include <iostream>
#include <stdint.h>
#include <mutex>

/*
	default maximum disposed time: UINT32_MAX
*/
class IDisposable {
public:
	bool			tryDispose();
	void			unDispose();
	unsigned int	getdisposedCount() const;
	unsigned int	getMaxdisposedCount() const;
protected:
	IDisposable();
	IDisposable(unsigned int max_disposed);
	virtual ~IDisposable();
private:
	unsigned int	_disposed = 0;
	unsigned int	_max = UINT32_MAX;// std::pow(2, sizeof(unsigned int) * 8) - 1;
	std::mutex		_access;
	/*
		we could create a bool for when we want to make it infinitly disposable and adapt tryDispose().
		this would add more data in memory for this interface that can be used in a large number of objects
		std::mutex is 80 bytes...
		hoping UINTN_MAX is enough
	*/
};
