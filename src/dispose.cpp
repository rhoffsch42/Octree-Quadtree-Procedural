#include "dispose.hpp"
#include "misc.hpp"

#ifdef TREES_DEBUG
 #define TREES_DISPOSE_DEBUG
#endif
#ifdef TREES_DISPOSE_DEBUG 
 #define D(x) std::cout << "[Dispose] " << x ;
 #define D_(x) x ;
 #define D_SPACER "-- dispose.cpp -------------------------------------------------\n"
 #define D_SPACER_END "----------------------------------------------------------------\n"
#else 
 #define D(x)
 #define D_(x)
 #define D_SPACER ""
 #define D_SPACER_END ""
#endif

IDisposable::IDisposable() {}
IDisposable::IDisposable(unsigned int max_disposed) : _max(max_disposed) {}
IDisposable::~IDisposable() {
	if (this->_disposed != 0) {//can happen when exiting the builder threads while there are jobs and freeing everything... -> exit when all jobs are finished
		std::cerr << "Destructor called on a object disposed " << this->_disposed << " times.\n";
		Misc::breakExit(87);
	}
}

bool	IDisposable::dispose() {
	this->_access.lock();
	if (this->_disposed == this->_max) {
		this->_access.unlock();
		return false;
	} else {
		this->_disposed++;
		this->_access.unlock();
		return true;
	}
}

void	IDisposable::unDispose() {
	this->_access.lock();
	if (this->_disposed == 0) {
		std::cerr << "Warning: undisposing an already undisposed object, this is unlikely to happen with a good usage\n";
		//Misc::breakExit(87);
	} else {
		this->_disposed--;
	}
	this->_access.unlock();
}

unsigned int	IDisposable::getdisposedCount() const { return this->_disposed; }
unsigned int	IDisposable::getMaxdisposedCount() const { return this->_max; }
