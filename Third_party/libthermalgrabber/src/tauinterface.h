#ifndef TAUINTERFACE_H
#define TAUINTERFACE_H

#include <thermograbber.h>
#include <taucom.h>
#include <tauimagedecoder.h>
#include <vector>
#include <thread>

class TauInterface : public ThermoGrabber, private TauCom, private TauImageDecoder
{

public:

    /**
     * Type definition of callback
     */
    typedef void (*callbackTauRawBitmapUpdate)(TauRawBitmap& tRawBitmap, void* caller);

    /**
     * @brief TauInterface needs a callback ptr for delivering the TauRawBitmaps.
     * Additionally the instance of the caller is needed in order to call the callback.
     * @param ptr Callback pointer
     * @param caller Calling instance
     */
    TauInterface(callbackTauRawBitmapUpdate ptr, void* caller);
    ~TauInterface();

    /**
      * Connect the ThermalCapture GrabberUSB hardware.
    */
    bool connect();
    bool connect(char* iSerialUSB);

    /**
      * Sending a byte based command to the tau core.
      * Please see "FLIR TAU2/QUARK2 SOFTWARE IDD" for options.
      * First byte is the function byte.
      * Optional following bytes are commands and command parameter.
     */
    void sendCommand(char cmd, char* data, unsigned int data_len);
    void frameDecoded(TauRawBitmap *frame);

    //Called by ThermoGrabber
    void processUartData(uint8_t* buffer,uint32_t size);

    //Called by TauCom
    void processSerialPacket(uint8_t* buffer,uint32_t size);

    //Called by ThermoGrabber
    void processVideoData(uint16_t* buffer, uint32_t size);

    void serialPackageReceived(uint8_t* buffer, uint32_t size);

    //Getter functions
    unsigned int getTLinearEnableStatus();
    unsigned int getCameraSerialNumber();
    unsigned int getSensorSerialNumber();
    const char* getCameraPartNumber() const;
    unsigned int getWidth();
    unsigned int getHeight();

    // Flat field correction - shutter
    void doFFC();

    void enableGainModeAutomatic();
    void enableGainModeHigh();
    void enableGainModeLow();
    void enableGainModeManual();

    void enableTLinear();
    void disableTLinear();

    void disableTrigger();
    void enableTriggerSlave();
    void enableTriggerMaster();

    void setTLinearHighResolution();
    void setTlinearLowResolution();

    void enableDigitalOutputMode();
    void disableDigitalOutputMode();

    void enableDigitalOutputMode_XPModeCMOS14Bit();
    void disableDigitalOutputMode_XPMode();
    void setDigitalOutputMode_XPModeCMOSBitDepth14();

    // If watchDogCnt becomes zero, connection gets resetted
    static unsigned int watchDogCnt;
    void reenableFTDIConnection();

    std::thread threadTauConnection; // reference to the tau connection thread
    char mISerialUSB[32]={0};

private:

    TauRawBitmap* mTauRawBitmap;
    callbackTauRawBitmapUpdate mCallback;// callback for received images
    void* mCallbackInstance;
    bool isConnected();
    bool isConfigUpdated();

    bool mVideoProcessingEnabled;

    unsigned int mTLinearEnableStatus = 0;

    unsigned int mCameraSerial = 0;
    unsigned int mSensorSerial = 0;

    unsigned int mSWMajorVersion = 0;
    unsigned int mSWMinorVersion = 0;
    unsigned int mFWMajorVersion = 0;
    unsigned int mFWMinorVersion = 0;

    char mCameraPartNumber[32] = {0};

    bool mDigitalOutputEnabledChecked = false;
    bool mDigitalOutputEnabledStatus = false;

    bool mDigitalOutputXPMode14bitChecked = false;
    bool mDigitalOutputXPMode14bitStatus = false;

    bool mDigitalOutputCMOSBitDepth14bitChecked = false;
    bool mDigitalOutputCMOSBitDepth14bitStatus = false;

    bool mTLinearEnabledChecked = false;
    bool mTLinearEnabledStatus = false;

    void identifyTauCore();
    bool checkSettings();

    enum {
        REQ_DIGITAL_OUTPUT_ENABLED,
        REQ_DIGITAL_OUTPUT_XP_MODE,
        REQ_DIGITAL_OUTPUT_CMOS_MODE
    } mPresentRequestDigitalOutput;

    enum {
        REQ_SET_TLIN_ENABLED,
        REQ_GET_TLIN_ENABLED,
        REQ_SET_TLIN_MODE,
        REQ_GET_TLIN_MODE
    } mPresentRequestTLin;

    std::thread threadWatchdog;

};

#endif // TAUINTERFACE_H
