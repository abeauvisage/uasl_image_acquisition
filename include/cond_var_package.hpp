#ifndef IMAGE_ACQUISITION_COND_VAR_PACKAGE_HPP
#define IMAGE_ACQUISITION_COND_VAR_PACKAGE_HPP

#include <mutex>
#include <condition_variable>

namespace cam {

class Acquisition;

struct Cond_var_package
{
	Cond_var_package(Acquisition& acq_ref_) : acq_ref(acq_ref_), value(true) {}
	Acquisition& acq_ref;
	std::condition_variable cv;
	bool value;
	std::mutex mtx;
}; //struct Cond_var_package
} //cam namespace

#endif
