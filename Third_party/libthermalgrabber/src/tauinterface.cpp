#include "tauinterface.h"

#include <iostream>
#include <string.h>

bool runWatchDog = false;
void watchDog(TauInterface* instance)
{
    TauInterface* ti = instance;

    while (runWatchDog && TauInterface::watchDogCnt)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        if (TauInterface::watchDogCnt < 9)
//            std::cerr << "watchdog " << TauInterface::watchDogCnt << std::endl;

        TauInterface::watchDogCnt--;

        if (TauInterface::watchDogCnt == 0)
        {

            std::cerr << "Stopping unresponsive ThermalCapture GrabberUSB" << std::endl;

            ti->stopGrabber();

            if (ti->threadTauConnection.joinable())
            {
                ti->threadTauConnection.join();
            }

//            std::cerr << "Resetting usb connection" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));

//            std::cerr << "Connecting ThermalCapture GrabberUSB" << std::endl;

            if (!strcmp(ti->mISerialUSB, ""))
            {
                std::cerr << "Trying to connect first available ThermalCapture GrabberUSB" << std::endl;
                ti->reenableFTDIConnection();
                ti->connect();
                TauInterface::watchDogCnt = 10; // reset watchdog
            }
            else
            {
                std::cerr << "Trying to connect ThermalCapture GrabberUSB " << ti->mISerialUSB << std::endl;
                ti->reenableFTDIConnection();
                ti->connect(ti->mISerialUSB);
                TauInterface::watchDogCnt = 10; // reset watchdog
            }
        }
    }
}
unsigned int TauInterface::watchDogCnt = 10;

//
//--- SET function codes without command/data bytes ----
//

// no operation  just verify proper communication
static constexpr char NO_OP[1] = {0x00};

// safe current settings to default on start up
static constexpr char SET_DEFAULTS[1] = {0x01};

// camera reset
static constexpr char CAMERA_RESET[1] = {0x02};

// reset to factory defaults (volatile)
static constexpr char RESTORE_FACTORY_DEFAULTS[1] = {0x03};

// get camera part number (general camera type with resolution info) ---
static constexpr char CAMERA_PART[1] = {0x66};

// get camera/sensor serial (unique)
static constexpr char SERIAL_NUMBER[1] = {0x04};

// get software/firmware revision (unique)
static constexpr char GET_REVISION[1] = {0x05};

// FFC (flat field correction)
static constexpr char DO_FFC[1] = {0x0C};

//
//--- SET function codes with command/data bytes ------
//

// GAIN_MODE 0x0A
// 0x0000 = Automatic
// 0x0001 = Low Gain Only
// 0x0002 = High Gain Only
// 0x0003 = Manual
static constexpr char GAIN_MODE_Automatic[3] = {0x0A, 0x00, 0x00};
static constexpr char GAIN_MODE_High[3] = {0x0A, 0x00, 0x02};
static constexpr char GAIN_MODE_Low[3] = {0x0A, 0x00, 0x01};
static constexpr char GAIN_MODE_Manual[3] = {0x0A, 0x00, 0x03};

// TLinear 0x8E
// Bytes 0-1: 0x0040
// Bytes 2-3: TLin Enable Status
static constexpr char TLIN_COMMANDS_ENABLE[5] = {(char)0x8E, 0x00, 0x40, 0x00, 0x01}; // cmd enable TLinear
static constexpr char TLIN_COMMANDS_DISABLE[5] = {(char)0x8E, 0x00, 0x40, 0x00, 0x00}; // cmd disable TLinear

static constexpr char TRIGGER_COMMANDS_DISABLED[3] = {(char)0x21,0x00,0x00};
static constexpr char TRIGGER_COMMANDS_SLAVE[3] = {(char)0x21,0x00,0x01};
static constexpr char TRIGGER_COMMANDS_MASTER[3] = {(char)0x21,0x00,0x02};

// TLinear Resolution 0x8E
// Bytes 0-1: 0x0010
// Bytes 2-3: TLin Output Mode
// 0x0000 = Low resolution mode
// 0x0001 = High resolution mode
static constexpr char TLIN_COMMANDS_HIGH_RESOLUTION[5] = {(char)0x8E, 0x00, 0x10, 0x00, 0x01}; // cmd enable TLinear high resolution
static constexpr char TLIN_COMMANDS_LOW_RESOLUTION[5] = {(char)0x8E, 0x00, 0x10, 0x00, 0x00}; // cmd enable TLinear low resolution

// Common disable (affects both the LVDS modes.
// and XP channels)
// 0x0000 = enabled
// Note: In Tau 1.X, it was not possible to
// 0x0002 = disabled
static constexpr char DIGITAL_OUTPUT_MODE_ENABLE[3] = {0x12, 0x00, 0x00};
static constexpr char DIGITAL_OUTPUT_MODE_DISABLE[3] = {0x12, 0x00, 0x02};

// Digital Output Mode
// Byte 0: 0x03
// Byte 1:
// 0x00 = disabled
// 0x01 = BT656
// 0x02 = CMOS 14-bit w/ 1 discrete
// 0x03 = CMOS 8-bit w/ 8 discretes
// 0x04 = CMOS 16-bit
static constexpr char DIGITAL_OUTPUT_MODE_XP_MODE_DISABLED[3] = {0x12, 0x03, 0x00};
static constexpr char DIGITAL_OUTPUT_MODE_XP_MODE_CMOS_14_BIT[3] = {0x12, 0x03, 0x02};
static constexpr char DIGITAL_OUTPUT_MODE_CMOS_BIT_DEPTH_14[3] = {0x12, 0x06, 0x00};

//
//--- GET function codes ------------------------------------------------------
//

// get digital output enabled
static constexpr char GET_DIGITAL_OUTPUT_MODE[1] = {0x12};

//get digital output xp mode
static constexpr char GET_DIGITAL_OUTPUT_MODE_XP_MODE[3] = {0x12, 0x02, 0x00};

// get digitial output cmos mode
static constexpr char GET_DIGITAL_OUTPUT_MODE_CMOS_MODE[3] = {0x12, 0x08, 0x00};

// get tlinear enabled state
static constexpr char GET_TLIN_ENABLED[3] = {(char)0x8E, 0x00, 0x40};


bool flirComOk = false;
bool tcConnected = false;
unsigned long frameCnt = 0;

void TauInterface::reenableFTDIConnection()
{
    flirComOk = false;
    tcConnected = false;
    reenableFTDI();
}

TauInterface::TauInterface(callbackTauRawBitmapUpdate cb, void* caller) : mVideoProcessingEnabled(false)
{
    mTauRawBitmap = NULL;
    mCallback = (callbackTauRawBitmapUpdate)cb;
    mCallbackInstance = caller;

    runWatchDog = true;
    std::thread threadWD(watchDog, this);
    threadWatchdog = std::move(threadWD);
}

TauInterface::~TauInterface()
{
    runWatchDog = false;
    if (threadWatchdog.joinable())
    {
        threadWatchdog.join();
    }


    stopGrabber();
    tcConnected = false;
    if (threadTauConnection.joinable())
    {
        threadTauConnection.join();
    }
}

static void threadWrapper(TauInterface* ti, const char* iSerialUSB)
{
//    std::cout << "thread wrapper" << std::endl;
    ti->runGrabber(iSerialUSB);
    tcConnected = false;
}

bool TauInterface::isConnected()
{
    return tcConnected;
}

bool TauInterface::isConfigUpdated()
{
    if (mTauCoreResWidth != 0 && mTauCoreResHeight != 0)
        return true;
    return false;
}

bool TauInterface::connect()
{
    // Startup time
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    //-------------------------------------------------------------------------
    // Connect to first found ThermalCapture GrabberUSB
    std::thread thread(threadWrapper, this, (char*)"\0");//(char*)0);
    threadTauConnection = std::move(thread);
    tcConnected = true;

    return checkSettings();
}

bool TauInterface::connect(const char* iSerialUSB)
{
    // safe the iSerialUSB for possible reconnects
    strncpy(&mISerialUSB[0], iSerialUSB, sizeof(mISerialUSB));

    // Startup time
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    //-------------------------------------------------------------------------
    // Connect ThermalCapture GrabberUSB with defined USB serial (iSerial)
    std::thread thread(threadWrapper, this, iSerialUSB);
    threadTauConnection = std::move(thread);
    tcConnected = true;

    return checkSettings();
}

bool TauInterface::checkSettings()
{
    //-------------------------------------------------------------------------
    // Reset the flirComOk to false.
    // Check if the flir device responds within time out
    flirComOk = false; // reset

    for (int i=0; i<10; i++)
    {
        if (!flirComOk)   // tcComOk becomes true if no_op response from flir device is received
        {
            if (grabberRuns)    // be sure that the usb part is ready
                sendCommand(NO_OP[0], 0, 0);

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        else    // communication with flir device seems to be working... ...go on
            break;
    }

    if (!flirComOk)
    {
        std::cerr << "Timeout: Communication problem with ThermalCapture GrabberUSB" << std::endl;
        return false;
    }

    //-------------------------------------------------------------------------
    // Infos about resolution are needed for processing incoming data.
    // Try to identify the flir device until timeout occurs.

    // wait additional 500ms before giving up getting config of camera
    for (int i=0; i<5; i++)
    {
        identifyTauCore();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        if (isConfigUpdated()) // config done? -> return
            break;
    }

    if (!isConfigUpdated())
    {
        std::cerr << "Timeout: Configuration could not be detected" << std::endl;
        return false;
    }

    //-------------------------------------------------------------------------
    // Check settings of Flir device
    bool safeSettings = false;
    bool configIsRead = false;

    for (int i=0; i<12; i++)
    {
        if (!mDigitalOutputEnabledChecked)
        {
            mPresentRequestDigitalOutput = REQ_DIGITAL_OUTPUT_ENABLED;

            if (grabberRuns)
                sendCommand(GET_DIGITAL_OUTPUT_MODE[0],
                        const_cast<char*>(&GET_DIGITAL_OUTPUT_MODE[1]),
                        sizeof(GET_DIGITAL_OUTPUT_MODE)-1);
        }
        else if (!mDigitalOutputXPMode14bitChecked)
        {
            mPresentRequestDigitalOutput = REQ_DIGITAL_OUTPUT_XP_MODE;

            if (grabberRuns)
                sendCommand(GET_DIGITAL_OUTPUT_MODE_XP_MODE[0],
                        const_cast<char*>(&GET_DIGITAL_OUTPUT_MODE_XP_MODE[1]),
                        sizeof(GET_DIGITAL_OUTPUT_MODE_XP_MODE)-1);
        }
        else if (!mDigitalOutputCMOSBitDepth14bitChecked)
        {
            mPresentRequestDigitalOutput = REQ_DIGITAL_OUTPUT_CMOS_MODE;

            if (grabberRuns)
                sendCommand(GET_DIGITAL_OUTPUT_MODE_CMOS_MODE[0],
                        const_cast<char*>(&GET_DIGITAL_OUTPUT_MODE_CMOS_MODE[1]),
                        sizeof(GET_DIGITAL_OUTPUT_MODE_CMOS_MODE)-1);
        }
        /*else if (!mTLinearEnabledChecked)
        {
            mPresentRequestTLin = REQ_GET_TLIN_ENABLED;

            if (grabberRuns)
                sendCommand(GET_TLIN_ENABLED[0],
                        const_cast<char*>(&GET_TLIN_ENABLED[1]),
                        sizeof(GET_TLIN_ENABLED)-1);
        }*/
        else
        {
            configIsRead = true;
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }

    if (!configIsRead)
        return false;

//    std::cout << "Results of config check:" << std::endl;
//
//    if (mDigitalOutputEnabledStatus)
//        std::cout << "- Digital Output is enabled" << std::endl;
//    else
//        std::cout << "- Digital Output is disabled" << std::endl;
//
//    if (mDigitalOutputCMOSBitDepth14bitStatus)
//        std::cout << "- CMOS 14 bit enabled" << std::endl;
//    else
//        std::cout << "- CMOS 14 bit disabled" << std::endl;
//
//    if (mDigitalOutputXPMode14bitStatus)
//        std::cout << "- XP Mode 14bit enabled" << std::endl;
//    else
//        std::cout << "- XP Mode 14bit disabled" << std::endl;

//    if (mTLinearEnabledStatus)
//        std::cout << "- TLinear enabled" << std::endl;
//    else
//        std::cout << "- TLinear disabled" << std::endl;


    //
    //---- Enable missing settings of Flir device -----------------------------
    //

    if (!mDigitalOutputCMOSBitDepth14bitStatus)
    {
        safeSettings = true;
        std::cout << "Enabling CMOS 14 bit" << std::endl;

        // enable digital output cmos 14 bit
        if (grabberRuns)
            sendCommand(DIGITAL_OUTPUT_MODE_CMOS_BIT_DEPTH_14[0],
                    const_cast<char*>(&DIGITAL_OUTPUT_MODE_CMOS_BIT_DEPTH_14[1]),
                    sizeof(DIGITAL_OUTPUT_MODE_CMOS_BIT_DEPTH_14)-1);

        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    if (!mDigitalOutputXPMode14bitStatus)
    {
        safeSettings = true;
        std::cout << "Enabling XP Mode 14 bit" << std::endl;

        // enable digital output xp mode 14 bit
        if (grabberRuns)
            sendCommand(DIGITAL_OUTPUT_MODE_XP_MODE_CMOS_14_BIT[0],
                    const_cast<char*>(&DIGITAL_OUTPUT_MODE_XP_MODE_CMOS_14_BIT[1]),
                    sizeof(DIGITAL_OUTPUT_MODE_XP_MODE_CMOS_14_BIT)-1);

        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    if (!mDigitalOutputEnabledStatus)
    {
        safeSettings = true;
        std::cout << "Enabling Digital Output" << std::endl;

        // enable digital output
        if (grabberRuns)
            sendCommand(DIGITAL_OUTPUT_MODE_ENABLE[0],
                    const_cast<char*>(&DIGITAL_OUTPUT_MODE_ENABLE[1]),
                    sizeof(DIGITAL_OUTPUT_MODE_ENABLE)-1);

        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

//    if (!mTLinearEnabledStatus)
//    {
//        safeSettings = true;
//        std::cout << "Enabling TLinear" << std::endl;

//        // enable tlinear
//        mPresentRequestTLin = REQ_SET_TLIN_ENABLED;
//        if (grabberRuns)
//            sendCommand(TLIN_COMMANDS_ENABLE[0],
//                    const_cast<char*>(&TLIN_COMMANDS_ENABLE[1]),
//                    sizeof(TLIN_COMMANDS_ENABLE)-1);

//        std::this_thread::sleep_for(std::chrono::milliseconds(250));
//    }


    // safe settings should only be done if neccessary (write cycles)
    if (safeSettings)
    {
        std::cout << "Save settings" << std::endl;
        if (grabberRuns)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            sendCommand(0x01, 0, 0);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }

    return true;
}

void TauInterface::frameDecoded(TauRawBitmap* tauRawBitmap)
{
    if (mTauRawBitmap != NULL)
        delete mTauRawBitmap;
    mTauRawBitmap = tauRawBitmap;

    mCallback(*mTauRawBitmap,  mCallbackInstance);
}

void TauInterface::sendCommand(char cmd, char *data, unsigned int data_len)
{
    if (isConnected())
    {
        //uint8_t buffer[32];
        uint8_t buffer[64];
        // char cmd, len of data, pointer to data, transfer buffer
        int size=genTauFrame(cmd, data_len, (uint8_t*)data, buffer);
        sendUartData(buffer, size);

        // Small pause to be sure tau core / vue has processed the command
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }
    else
    {
        std::cerr << "TauInterface: Send command failed - not connected" << std::endl;
    }
}

void TauInterface::processUartData(uint8_t* buffer,uint32_t size)
{
    parseInFrame(buffer,size);
}


void TauInterface::processSerialPacket(uint8_t* buffer, uint32_t size)
{
    /*
    * 1 Process Code Set to 0x6E on all valid incoming and reply messages
    * 2 Status
    * 3 Reserved
    * 4 Function
    * 5 Byte Count (MSB)
    * 6 Byte Count (LSB)
    * 7 CRC1 (MSB)
    * 8 CRC1 (LSB)
    * N Argument
    * N+1 CRC2 (MSB)
    * N+2 CRC2 (LSB)
    */

    if(size<4)
    {
        return;
    }

//    std::cout << "TEST: " << std::hex;
//    for (int i=0; i<size; i++)
//        std::cout << (int)(char)buffer[i];
//    std::cout << std::dec << std::endl;


    switch (buffer[3])  // parse function code
    {

    case 0x00:
        std::cout << "No op" << std::endl;
        if (!flirComOk)
            flirComOk = true;
        break;

    case 0x01:
        std::cout << "Set defaults" << std::endl;
        break;

    case 0x02:
        std::cout << "Reset" << std::endl;
        break;

    case 0x03:
        std::cout << "Factory defaults" << std::endl;
        break;

    case 0x04:
        std::cout << "Serial numbers" << std::endl;
        //Bytes 0-3: Camera serial number
        //Bytes 4-7: Sensor serial number

        mCameraSerial = buffer[8] << 24;
        mCameraSerial |= buffer[9] << 16;
        mCameraSerial |= buffer[10] << 8;
        mCameraSerial |= buffer[11];

        mSensorSerial = buffer[12] << 24;
        mSensorSerial |= buffer[13] << 16;
        mSensorSerial |= buffer[14] << 8;
        mSensorSerial |= buffer[15];

        std::cout << "Camera: " << mCameraSerial << std::endl;
        std::cout << "Sensor: " << mSensorSerial << std::endl;
        break;

    case 0x05:
        std::cout << "Revisions" << std::endl;
        //Bytes 0-1: SW major version
        //Bytes 2-3: SW minor version
        //Bytes 4-5: FW major version
        //Bytes 6-7: FW minor version

        mSWMajorVersion = buffer[8] << 8;
        mSWMajorVersion |= buffer[9];

        mSWMinorVersion = buffer[10];
        mSWMinorVersion |= buffer[11];

        mFWMajorVersion = buffer[12] << 8;
        mFWMajorVersion |= buffer[13];

        mFWMinorVersion = buffer[14];
        mFWMinorVersion |= buffer[15];

        std::cout << "Software: " << mSWMajorVersion << "." << mSWMinorVersion << std::endl;
        std::cout << "Firmware: " << mFWMajorVersion << "." << mFWMinorVersion << std::endl;
        break;

    case 0x0A:
        std::cout << "Gain Mode" << std::endl;
        if (buffer[9] == 0x00)
            std::cout << "Automatic" << std::endl;
        else if (buffer[9] == 0x01)
            std::cout << "Low gain" << std::endl;
        else if (buffer[9] == 0x02)
            std::cout << "High gain" << std::endl;
        else if (buffer[9] == 0x03)
            std::cout << "Manual" << std::endl;
        break;

    case 0x0C:
        std::cout << "FFC" << std::endl;
        break;

    case 0x12:
        std::cout << "Digital Output" << std::endl;

        if (buffer[8] == 0x00)
        {
            // For 0x00 you can only do response interpretation if it's known
            // what the initial request was!
            if (mPresentRequestDigitalOutput == REQ_DIGITAL_OUTPUT_ENABLED)
            {
                if (!mDigitalOutputEnabledChecked)
                {
                    mDigitalOutputEnabledChecked = true;
                    std::cout << "Status is ";
                    if (buffer[9] == 0x02)
                        std::cout << "disabled" << std::endl;
                    else if (buffer[9] == 0x00)
                    {
                        mDigitalOutputEnabledStatus = true;
                        std::cout << "enabled" << std::endl;
                    }
                    else
                        std::cout << "unknown" << std::endl;
                }
                else
                {
                    std::cout << "Set to ";
                    if (buffer[9] == 0x02)
                    {
                        std::cout << "disabled" << std::endl;
                    }
                    else if (buffer[9] == 0x00)
                    {
                        mDigitalOutputEnabledStatus = true;
                        std::cout << "enabled" << std::endl;
                    }
                    else
                    {
                        std::cout << "unknown" << std::endl;
                    }
                }
            }
            else if (mPresentRequestDigitalOutput == REQ_DIGITAL_OUTPUT_XP_MODE)
            {
                mDigitalOutputXPMode14bitChecked = true;
                std::cout << "Status is ";
                if (buffer[9]==0x00)
                    std::cout << "XP Mode disabled" << std::endl;
                else if (buffer[9] == 0x01)
                    std::cout << "XP Mode BT656" << std::endl;
                else if (buffer[9] == 0x02)
                {
                    mDigitalOutputXPMode14bitStatus = true;
                    std::cout << "XP Mode CMOS 14-bit w/ 1 discrete" << std::endl;
                }
                else if (buffer[9] == 0x03)
                    std::cout << "XP Mode CMOS 8-bit w/ 8 discrete" << std::endl;
                else if (buffer[9] == 0x04)
                    std::cout << "XP Mode CMOS 16-bit" << std::endl;
                else
                    std::cout << "XP Mode unknown" << std::endl;
            }
            else if (mPresentRequestDigitalOutput == REQ_DIGITAL_OUTPUT_CMOS_MODE)
            {
                mDigitalOutputCMOSBitDepth14bitChecked = true;
                std::cout << "Status is ";
                if (buffer[9] == 0x00)
                {
                    mDigitalOutputCMOSBitDepth14bitStatus = true;
                    std::cout << "CMOS mode 14bit" << std::endl;
                }
                else if (buffer[9] == 0x01)
                    std::cout << "8bit post-AGC/pre-colorize" << std::endl;
                else if (buffer[9] == 0x02)
                    std::cout << "8bit Bayer encoded" << std::endl;
                else if (buffer[9] == 0x03)
                    std::cout << "16bit YCbCr" << std::endl;
                else if (buffer[9] == 0x04)
                    std::cout << "8bit 2x Clock YCbCr" << std::endl;
                else
                    std::cout << "CMOS mode Bit Depth unknown" << std::endl;
            }
        }
        else if (buffer[8] == 0x03)
        {
            std::cout << "Set to ";
            if (buffer[9]==0x00)
                std::cout << "XP Mode disabled" << std::endl;
            else if (buffer[9] == 0x02)
                std::cout << "XP Mode CMOS 14-bit w/ 1 discrete" << std::endl;
            else
                std::cout << "XP Mode unknown" << std::endl;
        }
        else if (buffer[8] == 0x06)
        {
            std::cout << "Set to ";
            if (buffer[9] == 0x00)
                std::cout << "CMOS mode Bit Depth 14bit pre-AGC" << std::endl;
            else
                std::cout << "CMOS mode Bit Depth unknown" << std::endl;

        }

        break;

    case 0x66:
        std::cout << "Camera part number" << std::endl << std::hex;
        //Bytes 0-31: Part number (ASCII)

        for (int i=0; i<32; i++)
        {
            mCameraPartNumber[i] = buffer[i+8];
            if (buffer[i+8] == 0) // end of ascii string
                break;
            std::cout << (char)buffer[i+8];
        }
        std::cout << std::dec << std::endl;

        if (!mVideoProcessingEnabled)
        {
            int idx=-1;
            for (int i=0; i<32; i++)
            {
                if (mCameraPartNumber[i] == '^')
                {
                    idx = i;
                    mCameraPartNumber[31] = 0;
                    break;
                }
            }
            if (idx != -1)
            {
                // use camera part no after character '^'
                // vue 436-0008-00S^46640009NC
                // tau2 46640013H-SPNLX^46640013H
                // tau1 41320009H-SPNLX^41320009H
                char resolution_info[4]={0};
                resolution_info[0]= mCameraPartNumber[idx+3];//mCameraPartNumber[2];
                resolution_info[1]= mCameraPartNumber[idx+4];//mCameraPartNumber[3];
                resolution_info[2]= mCameraPartNumber[idx+5];//mCameraPartNumber[4];
                int result = atoi(resolution_info);

                if (result == 640)
                {
                    mTauCoreResWidth=640;
                    mTauCoreResHeight=512;
                    mVideoProcessingEnabled=true;
                }
                else if (result == 336)
                {
                    mTauCoreResWidth=336;
                    mTauCoreResHeight=256;
                    mVideoProcessingEnabled=true;
                }
                else if (result == 324 || result == 320)
                {
                    mTauCoreResWidth=324;
                    mTauCoreResHeight=256;
                    mVideoProcessingEnabled=true;
                }
                else if (result == 160)
                {
                    mTauCoreResWidth=160;
                    mTauCoreResHeight=128;
                    mVideoProcessingEnabled=true;
                }
            }
        }
        break;

    case 0x8E:
        // std::cout << "TLin Enable Status" << std::endl;
        // For 0x8E you can only do response interpretation if it's known
        // what the initial request was!

        if (mPresentRequestTLin == REQ_GET_TLIN_ENABLED)
        {
            std::cout << "TLinear enable status" << std::endl;

            //Bytes 0-1: TLin Enable Status
            mTLinearEnableStatus = buffer[8] << 8;
            mTLinearEnableStatus |= buffer[9];
            mTLinearEnabledStatus = mTLinearEnableStatus;

            std::cout << "TLinear status is " << mTLinearEnableStatus << std::endl;

            mTLinearEnabledChecked = true;

            if (mTLinearEnableStatus == 0)
                mTLinearEnabledStatus = false;

            else
                mTLinearEnabledStatus = true;
        }
        else if (mPresentRequestTLin == REQ_GET_TLIN_MODE)
        {
            std::cout << "TLinear mode status" << std::endl;

            if (buffer[9] == 0x00)
                std::cout << "TLinear is in low resolution mode" << std::endl;

            else if (buffer[9] == 0x01)
                std::cout << "TLinear is in high resolution mode" << std::endl;
        }
        else if (mPresentRequestTLin == REQ_SET_TLIN_ENABLED)
        {
            std::cout << "TLinear set" << std::endl;
        }
        else if (mPresentRequestTLin == REQ_SET_TLIN_MODE)
        {
            std::cout << "TLinear mode set" << std::endl;
        }

        break;

    default:
        std::cout << "unknown/not implemeted yet: " << (int)(char)buffer[3] << std::endl;

    }
    std::cout << std::endl;
}


void TauInterface::processVideoData(uint16_t* buffer, uint32_t size)
{
    vector<uint16_t> v;
    v.resize(size);
    memcpy(v.data(), buffer, size*2);

//    std::cout << "processing video data" << std::endl;

    // Video processing uses the tau core properties for performance reasons.
    // Therfor it's not done till the parameters of the tau core/vue are known(w/h).
    if (mVideoProcessingEnabled)
    {
        // Infos about tau core especially resolution are present -> process data
        decodeData(v);

        watchDogCnt = 10;
    }
}



void TauInterface::identifyTauCore()
{
    // get the camera part number with informations about tau core (resolution etc.)
    if (mTauCoreResHeight==0 || mTauCoreResWidth==0)
        sendCommand(CAMERA_PART[0], 0, 0);

    // get serial numbers of tau core and sensor
    if (mCameraSerial==0 || mSensorSerial==0)
        sendCommand(SERIAL_NUMBER[0], 0, 0);

    // get revision
    if ((mSWMajorVersion==0 && mSWMinorVersion==0) || (mFWMajorVersion==0 && mFWMinorVersion==0))
        sendCommand(GET_REVISION[0], 0, 0);
}

//
//--- tau core getters --------------------------------------------------------
//

unsigned int TauInterface::getTLinearEnableStatus()
{
    return mTLinearEnableStatus;
}

unsigned int TauInterface::getCameraSerialNumber()
{
    return mCameraSerial;
}

unsigned int TauInterface::getSensorSerialNumber()
{
    return mSensorSerial;
}

const char* TauInterface::getCameraPartNumber() const
{
    return mCameraPartNumber;
}

unsigned int TauInterface::getWidth()
{
    return mTauCoreResWidth;
}

unsigned int TauInterface::getHeight()
{
    return mTauCoreResHeight;
}

//
//--- tau core settings -------------------------------------------------------
//

void TauInterface::doFFC()
{
    sendCommand(DO_FFC[0], 0, 0);
}

void TauInterface::enableGainModeAutomatic()
{
    sendCommand(GAIN_MODE_Automatic[0],
            const_cast<char*>(&GAIN_MODE_Automatic[1]),
            sizeof(GAIN_MODE_Automatic)-1);
}

void TauInterface::enableGainModeHigh()
{
    sendCommand(GAIN_MODE_High[0],
            const_cast<char*>(&GAIN_MODE_High[1]),
            sizeof(GAIN_MODE_High)-1);
}

void TauInterface::enableGainModeLow()
{
    sendCommand(GAIN_MODE_Low[0],
            const_cast<char*>(&GAIN_MODE_Low[1]),
            sizeof(GAIN_MODE_Low)-1);
}

void TauInterface::enableGainModeManual()
{
    sendCommand(GAIN_MODE_Manual[0],
            const_cast<char*>(&GAIN_MODE_Manual[1]),
            sizeof(GAIN_MODE_Manual)-1);
}

void TauInterface::enableTLinear()
{
    mPresentRequestTLin = REQ_SET_TLIN_ENABLED;
    sendCommand(TLIN_COMMANDS_ENABLE[0],
            const_cast<char*>(&TLIN_COMMANDS_ENABLE[1]),
            sizeof(TLIN_COMMANDS_ENABLE)-1);
}

void TauInterface::disableTLinear()
{
    mPresentRequestTLin = REQ_SET_TLIN_ENABLED;
    sendCommand(TLIN_COMMANDS_DISABLE[0],
            const_cast<char*>(&TLIN_COMMANDS_DISABLE[1]),
            sizeof(TLIN_COMMANDS_DISABLE)-1);
}

void TauInterface::setTLinearHighResolution()
{
    mPresentRequestTLin = REQ_SET_TLIN_MODE;
    sendCommand(TLIN_COMMANDS_HIGH_RESOLUTION[0],
            const_cast<char*>(&TLIN_COMMANDS_HIGH_RESOLUTION[1]),
            sizeof(TLIN_COMMANDS_HIGH_RESOLUTION)-1);
}

void TauInterface::setTlinearLowResolution()
{
    mPresentRequestTLin = REQ_SET_TLIN_MODE;
    sendCommand(TLIN_COMMANDS_LOW_RESOLUTION[0],
            const_cast<char*>(&TLIN_COMMANDS_LOW_RESOLUTION[1]),
            sizeof(TLIN_COMMANDS_LOW_RESOLUTION)-1);
}

void TauInterface::enableDigitalOutputMode()
{
    mPresentRequestDigitalOutput = REQ_DIGITAL_OUTPUT_ENABLED;
    sendCommand(DIGITAL_OUTPUT_MODE_ENABLE[0],
            const_cast<char*>(&DIGITAL_OUTPUT_MODE_ENABLE[1]),
            sizeof(DIGITAL_OUTPUT_MODE_ENABLE)-1);
}

void TauInterface::disableDigitalOutputMode()
{
    sendCommand(DIGITAL_OUTPUT_MODE_DISABLE[0],
            const_cast<char*>(&DIGITAL_OUTPUT_MODE_DISABLE[1]),
            sizeof(DIGITAL_OUTPUT_MODE_DISABLE)-1);
}

void TauInterface::enableDigitalOutputMode_XPModeCMOS14Bit()
{
    sendCommand(DIGITAL_OUTPUT_MODE_XP_MODE_CMOS_14_BIT[0],
            const_cast<char*>(&DIGITAL_OUTPUT_MODE_XP_MODE_CMOS_14_BIT[1]),
            sizeof(DIGITAL_OUTPUT_MODE_XP_MODE_CMOS_14_BIT)-1);
}

void TauInterface::disableDigitalOutputMode_XPMode()
{
    sendCommand(DIGITAL_OUTPUT_MODE_XP_MODE_DISABLED[0],
            const_cast<char*>(&DIGITAL_OUTPUT_MODE_XP_MODE_DISABLED[1]),
            sizeof(DIGITAL_OUTPUT_MODE_XP_MODE_DISABLED)-1);
}

void TauInterface::setDigitalOutputMode_XPModeCMOSBitDepth14()
{
    sendCommand(DIGITAL_OUTPUT_MODE_CMOS_BIT_DEPTH_14[0],
            const_cast<char*>(&DIGITAL_OUTPUT_MODE_CMOS_BIT_DEPTH_14[1]),
            sizeof(DIGITAL_OUTPUT_MODE_CMOS_BIT_DEPTH_14)-1);
}

void TauInterface::disableTrigger()
{
    sendCommand(TRIGGER_COMMANDS_DISABLED[0],
            const_cast<char*>(&TRIGGER_COMMANDS_DISABLED[1]),
            sizeof(TRIGGER_COMMANDS_DISABLED)-1);
}

void TauInterface::enableTriggerSlave()
{
    sendCommand(TRIGGER_COMMANDS_SLAVE[0],
            const_cast<char*>(&TRIGGER_COMMANDS_SLAVE[1]),
            sizeof(TRIGGER_COMMANDS_SLAVE)-1);
}

void TauInterface::enableTriggerMaster()
{
    sendCommand(TRIGGER_COMMANDS_MASTER[0],
            const_cast<char*>(&TRIGGER_COMMANDS_MASTER[1]),
            sizeof(TRIGGER_COMMANDS_MASTER)-1);
}

