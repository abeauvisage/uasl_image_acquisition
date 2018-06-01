#include <inttypes.h>

class ThermoGrabberPrivate;

class ThermoGrabber
{

public:

    ThermoGrabber();
    ~ThermoGrabber();

    //Run this function to initiate the connection to ThermoGrabber USB hardware
    //int runGrabber(void);
    int runGrabber(char *iSerialUSB);

    //Closes the connection to the USB hardware, runGrabber will return after calling this
    void stopGrabber(void);

    //Sends data to the TAU via the UART data channel
    void sendUartData(uint8_t* buffer,uint32_t size);

    // Becomes true if ThermoGrabber begins
    bool grabberRuns;

    void reenableFTDI();

protected:

    //This function is called when UART data was received
    virtual void processUartData(uint8_t* buffer,uint32_t size)=0;

    //This function is called when a video frame was received
    //Data is encoded in 16 bit words:
    //Bit 15 HSYNC //0 between lines, 1 if pixel data is valid
    //Bit 14 VSYNC //0 between frames, 1 if lines are valid
    //BIT 13-0 Pixel Data
    virtual void processVideoData(uint16_t* buffer, uint32_t size)=0;

    unsigned int getPPSTimestamp();

private:

    ThermoGrabberPrivate* tgP;
    static int static_readCallback(uint8_t *buffer, int length, void *progress, void *userdata);
    int readCallback(uint8_t *buffer, int length, void *progress);
    void writeUartHeader(uint8_t* buffer,uint8_t dataSize);

    unsigned int mPPSTimestamp;

};
