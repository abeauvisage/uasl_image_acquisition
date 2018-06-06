#include "thermalgrabber.h"
#include "tauinterface.h"
#include "tauimagedecoder.h"
#include "fastftdi.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>

TauInterface* mTauInterface;

ThermalGrabber::ThermalGrabber(callbackThermalGrabber ptr, void* caller): opened(false)
{
    mTauInterface = NULL;
    mCallbackThermalGrabber = ptr;
    mCallingInstance = caller;
    mTauInterface = new TauInterface(static_callbackTauData, this);
    opened = mTauInterface->connect();
}

ThermalGrabber::ThermalGrabber(callbackThermalGrabber ptr, void* caller, const char* iSerialUSB): opened(false)
{
    mTauInterface = NULL;
    mCallbackThermalGrabber = ptr;
    mCallingInstance = caller;
    mTauInterface = new TauInterface(static_callbackTauData, this);
    opened = mTauInterface->connect(iSerialUSB);
}

ThermalGrabber::~ThermalGrabber()
{
    if (mTauInterface != NULL)
        delete mTauInterface;
}

void ThermalGrabber::static_callbackTauData(TauRawBitmap& tauRawBitmap, void* caller)
{
    ThermalGrabber* tg=(ThermalGrabber*)caller;
    tg->callbackTauData(tauRawBitmap);
}

void ThermalGrabber::callbackTauData(TauRawBitmap& tauRawBitmap)
{
    mCallbackThermalGrabber(tauRawBitmap, mCallingInstance);
}

void ThermalGrabber::sendCommand(char cmd, char* data, unsigned int data_len)
{
    if (mTauInterface != NULL)
        mTauInterface->sendCommand(cmd, data, data_len);
}

unsigned int ThermalGrabber::scale(unsigned int value, unsigned int lowBound, unsigned int upBound , unsigned int minOutput, unsigned int maxOutput)
{
    if( upBound == lowBound )
    {
        std::cerr << "Error: Boundaries equal: " << lowBound << ", " << upBound << std::endl;
        return 0;
    }

    if( value > 0 )
    {
        if( value <= lowBound )
        {
            return minOutput;
        }

        if( value >= upBound )
        {
            return maxOutput;
        }

        int res = ((maxOutput-minOutput) * (value-lowBound) / (upBound-lowBound)) + minOutput;

        if( res <= (int)minOutput )
        {
            res = minOutput;
        }
        else if( res >= (int)maxOutput )
        {
            res = maxOutput;
        }
        return res;
    }
    return 0;
}

void ThermalGrabber::convertTauRawBitmapToTauRGBBitmap(TauRawBitmap &tauRawBitmap, TauRGBBitmap &tauRGBBitmap)
{

    if ((tauRawBitmap.data != NULL && tauRawBitmap.width > 0 && tauRawBitmap.height > 0)
            &&
            (tauRawBitmap.width == tauRGBBitmap.width && tauRawBitmap.height == tauRGBBitmap.height))
    {
        for (int i=0; i<tauRawBitmap.height*tauRawBitmap.width; i++)
        {
            unsigned int val = tauRawBitmap.data[i];
            unsigned int s = scale(val, tauRawBitmap.min, tauRawBitmap.max, 0, 255);
            tauRGBBitmap.data[i*3] = s;
            tauRGBBitmap.data[i*3+1] = s;
            tauRGBBitmap.data[i*3+2] = s;
        }
    }
    else
    {
        std::cerr << "Error: Cannot convert TauData to TauImage" << std::endl;
    }
}

unsigned int ThermalGrabber::getResolutionWidth()
{
    // Try to derive resolution details from the camera part number.
    // This is a string with the general type information of the tau core.
    // Parse the string and try to translate it to number.
    unsigned int result = 0;

    if (mTauInterface != NULL)
    {
        result = mTauInterface->getWidth();
//        const char* cameraPartNumber = mTauInterface->getCameraPartNumber();

//        if (cameraPartNumber[0] != 0) // string ok?
//        {
//            std::string str = std::string(cameraPartNumber);
//            std::size_t pos = str.find('^');

//            if (pos!=std::string::npos && (pos+4)<str.length())
//            {
//                std::string strRes = str.substr(pos+3, 3);
//                std::istringstream(strRes) >> result;

//                if (result != 640 && result != 336 && result != 324 && result != 160)
//                    result = 0;
//            }
//        }
    }

    return result;
}

unsigned int ThermalGrabber::getResolutionHeight()
{
    // Try to derive resolution details from the camera part number.
    // This is a string with the general type information of the tau core.
    // Parse the string and try to translate it to number.
    unsigned int result = 0;

    if (mTauInterface != NULL)
    {
        result = mTauInterface->getHeight();
//        const char* cameraPartNumber = mTauInterface->getCameraPartNumber();

//        if (cameraPartNumber[0] != 0) // string ok?
//        {
//            std::string str = std::string(cameraPartNumber);
//            std::size_t pos = str.find('^');

//            if (pos!=std::string::npos && (pos+4)<str.length())
//            {
//                std::string strRes = str.substr(pos+3, 3);
//                std::istringstream(strRes) >> result;

//                if (result == 640)  // type 640x512
//                    result = 512;
//                else if (result == 336 || result == 324) // type 336x256 or 324x256
//                    result =256;
//                else if (result == 160) // type 160x128
//                    result = 128;
//            }
//        }
    }
    return result;
}

unsigned int ThermalGrabber::getCameraSerialNumber()
{
    if (mTauInterface != NULL)
        return mTauInterface->getCameraSerialNumber();

    return 0;
}

const char* ThermalGrabber::getCameraPartNumber() const
{
    if (mTauInterface != NULL)
        return mTauInterface->getCameraPartNumber();
    return NULL;
}

void ThermalGrabber::doFFC()
{
    if (mTauInterface != NULL)
        mTauInterface->doFFC();
    else
        std::cerr << "doFFC failed: No connection to tau core" << std::endl;
}

void ThermalGrabber::setGainMode(thermal_grabber::GainMode gm)
{
    if (mTauInterface != NULL)
    {
        switch (gm)
        {
        case thermal_grabber::GainMode::Automatic:
//            std::cout << "setGainMode to Automatic" << std::endl;
            mTauInterface->enableGainModeAutomatic();
            break;

        case thermal_grabber::GainMode::HighGain:
//            std::cout << "setGainMode to High" << std::endl;
            mTauInterface->enableGainModeHigh();
            break;

        case thermal_grabber::GainMode::LowGain:
//            std::cout << "setGainMode to Low" << std::endl;
            mTauInterface->enableGainModeLow();
            break;

        case thermal_grabber::GainMode::Manual:
//            std::cout << "setGainMode to Manual" << std::endl;
            mTauInterface->enableGainModeManual();
            break;

        default:
            std::cerr << "setGainMode with invalid parameter" << std::endl;
        }
    }
    else
        std::cerr << "setGainMode failed: No connection to tau core" << std::endl;
}

void ThermalGrabber::setTriggerMode(thermal_grabber::TriggerMode tm)
{
    if (mTauInterface != NULL)
    {
        switch (tm)
        {
        case thermal_grabber::TriggerMode::disabled:
            mTauInterface->disableTrigger();
            break;
        case thermal_grabber::TriggerMode::slave:
            mTauInterface->enableTriggerSlave();
            break;
        case thermal_grabber::TriggerMode::master:
            mTauInterface->enableTriggerMaster();
            break;
        default:
            std::cerr << "setTriggerMode with invalid parameter" << std::endl;
        }
    }
    else
        std::cerr << "setGainMode failed: No connection to tau core" << std::endl;
}

void ThermalGrabber::enableTLinearHighResolution()
{
    if (mTauInterface != NULL)
    {
        mTauInterface->enableTLinear();
        mTauInterface->setTLinearHighResolution();
    }
    else
    {
        std::cerr << "setTLinearHighResolution failed: No connection to tau core" << std::endl;
    }
}

void ThermalGrabber::enableTlinearLowResolution()
{
    if (mTauInterface != NULL)
    {
        mTauInterface->enableTLinear();
        mTauInterface->setTlinearLowResolution();
    }
    else
    {
        std::cerr << "setTLinearLowResolution failed: No connection to tau core" << std::endl;
    }
}

void ThermalGrabber::disableTLinear()
{
    if (mTauInterface != NULL)
        mTauInterface->disableTLinear();
    else
        std::cerr << "disableTLinear failed: No connection to tau core" << std::endl;
}
