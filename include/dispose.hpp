#pragma once

#include <iostream>
#include <stdint.h>
#include <mutex>

/*
*   Interface used to monitor how many other entitys are disposing of this resource.
*	A typical use is to avoid deletion of disposed resources.
*	default maximum concurent dispose : UINT32_MAX
*/
class IDisposable {
public:
	/* it uses mutex::lock() */
	bool			dispose();
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
