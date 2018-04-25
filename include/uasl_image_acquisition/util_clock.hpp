#ifndef UASL_IMAGE_ACQUISITION_UTIL_CLOCK_HPP
#define UASL_IMAGE_ACQUISITION_UTIL_CLOCK_HPP

#include <chrono>
#include <iostream>
#include <iomanip>

namespace cam
{
//Print the characteristics of the clock on a specific system
template <typename C>
void printClockData ()
{
    using namespace std;

    cout << "- precision: ";
    // if time unit is less or equal one millisecond
    typedef typename C::period P;// type of time unit
    if (ratio_less_equal<P,milli>::value) {
       // convert to and print as milliseconds
       typedef typename ratio_multiply<P,kilo>::type TT;
       cout << fixed << double(TT::num)/TT::den
            << " milliseconds" << endl;
    }
    else {
        // print as seconds
        cout << fixed << double(P::num)/P::den << " seconds" << endl;
    }
    cout << "- is_steady: " << boolalpha << C::is_steady << endl;
}

inline void print_clock_carac()
{
	std::cout << "system_clock: " << std::endl;
  printClockData<std::chrono::system_clock>();
  std::cout << "\nhigh_resolution_clock: " << std::endl;
  printClockData<std::chrono::high_resolution_clock>();
  std::cout << "\nsteady_clock: " << std::endl;
  printClockData<std::chrono::steady_clock>();
}

typedef std::chrono::steady_clock clock_type;
} //namespace cam
#endif
