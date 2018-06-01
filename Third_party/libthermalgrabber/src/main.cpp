#include "thermalgrabber.h"

#include <stdbool.h>
#include <cstdlib>
#include <iostream>
#include <chrono>
#include <thread>
#include <cstdint>
#include <cstring>

ThermalGrabber* tGr;

class Test
{
public:
    void test();
};

void callbackTauImage(TauRawBitmap& tauRawBitmap, void* caller)
{
    std::cout << "updateTauRawBitmap -> w/h: " << tauRawBitmap.width << "/" << tauRawBitmap.height << " min/max: " << tauRawBitmap.min << "/" << tauRawBitmap.max << std::endl;
}

void Test::test()
{
    std::cout << "Test" << std::endl;
    tGr = new ThermalGrabber(callbackTauImage, this);

    unsigned int mWidth = tGr->getResolutionWidth();
    unsigned int mHeight = tGr->getResolutionHeight();
    std::cout << "Resolution w/h " << mWidth << "/" << mHeight << std::endl;

    // enable TLinear in high resolution on TauCores
    //tGr->enableTLinearHighResolution();

    // run demo for 20 seconds
    std::this_thread::sleep_for(std::chrono::milliseconds(20000)); 
    delete tGr;
}
int main()
{
    std::cout << "main:" << std::endl;
    {
        Test* t = new Test();
        t->test();
        delete t;
    }
    return 0;
}

