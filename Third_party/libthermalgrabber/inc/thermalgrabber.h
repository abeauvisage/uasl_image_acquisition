#ifndef THERMALGRABBER_H
#define THERMALGRABBER_H

#define LOG

#include <inttypes.h>

//! Container for raw tau bitmap.
/*!
*   The "width" and "height" paramter inform about the size of the bitmap.
*   Every pixel value of the tau core has 14 bit resolution and is stored
*   in the data section of this class as 16 bit value.
*   In addition for further processing the min and max values of the
*   raw tau bitmap are delivered by the "min" and "max" values.
*/
class TauRawBitmap
{

public:

    //! Width of raw tau core bitmap.
    /*!
    *    For example 160, 324, 336 or 640.
    */
    unsigned int width;

    //! Height of raw tau core bitmap.
    /*!
    *   For example 128, 256 or 512.
    */
    unsigned int height;

    //! Minimum value of raw tau core bitmap.
    /*!
    *   The range of raw pixel values goes from 0 to 16383.
    */
    unsigned int min;

    //! Maximum value of raw tau core bitmap.
    /*!
    *   The range of raw pixel values goes from 0 to 16383.
    */
    unsigned int max;

    //! PPS Timestamp
    /*!
     * The present pps timestamp value
     * (Milliseconds since last rising edge of ppm signal).
     */
    unsigned int pps_timestamp;

    //! Array of raw pixel values.
    /*!
    *   The raw pixel values (14 bit) are stored in a 16 bit array.
    */
    unsigned short* data;

    //! Constructor of TauRawBitmap.
    /*!
     * Initializes an object for raw tau bitmaps.
     * \param w Width of TauRawBitmap.
     * \param h height of TauRawBitmap.
     */
    TauRawBitmap(unsigned int w, unsigned int h);

    //! Destructor of TauRawBitmap.
    /*!
    *   Cleans up the TauRawBitmap object memory.
    */
    ~TauRawBitmap();
};

//! Container for rgb tau bitmap.
/*!
*   The "width" and "height" paramter inform about the size of the bitmap.
*   Every pixel value is stored in the data section of this class as 24 bit value.
*   The 24 bits per pixel contain 3 RGB channels.
*   Each channel value is 8 bit and all channels contain the same value.
*   The original 14 bit values of the TauRawBitmap pixels are scaled down to 8 bit resolution.
*/
class TauRGBBitmap
{

public:

    //! Width of raw tau core bitmap.
    /*!
    *    For example 160, 324, 336 or 640.
    */
    unsigned int width;

    //! Height of raw tau core bitmap.
    /*!
    *    For example 128, 256 or 512.
    */
    unsigned int height;

    //! Array of rgb pixel values.
    /*!
    *   The rgb pixel values (24 bit for all 3 channels) are stored in a byte array.
    */
    unsigned char * data;

    //! Constructor of TauRGBBitmap.
    /*!
     *  Initializes an object for rgb tau bitmaps.
     *  \param w Width of TauRGBBitmap.
     *  \param h height of TauRGBBitmap.
     */
    TauRGBBitmap(unsigned int width, unsigned int height);

    //! Destructor of TauRGBBitmap.
    /*!
    *   Cleans up the TauRGBBitmap object memory.
    */
    ~TauRGBBitmap();

    //! Scanline function of TauRGBBitmap.
    /*!
    *   For easy access of line addresses.
    */
    unsigned char * scanLine(unsigned int line);
};


enum TLinearResolution { High, Low };

namespace thermal_grabber {
enum GainMode {
    Automatic,
    LowGain,
    HighGain,
    Manual
};
}

//! Class for connecting "Thermal Capture Grabber USB".
/*!
*   Connecting Thermal Capture Grabber USB needs three steps:
*   1. Include the library header file "thermalgrabber.h" into your project.
*   2. Write a callback function for receiving TauRawBitmaps.
*   3. Use the constructor of ThermalGrabber. You will have to put in 2 parameters.
*   First put in the pointer of the callback function.
*   Second add the a pointer of the calling instance in the constructor.
*/
class ThermalGrabber
{

public:

    //! Type definition of callback.
    typedef void (*callbackThermalGrabber)(TauRawBitmap& tauRawBitmap, void* caller);

    //! Constructor of ThermalGrabber.
    /*!
    *   ThermalGrabber is used to connect the "Thermal Capture Grabber USB" device.
    *   The callback is called every time a full frame is received from the device.
    *   In order to use the callback ThermalGrabber needs to know the
    *   calling instance.
    *   ThermalCapture GrabberUSB uses the digital output of the tau core.
    *   Therefor it configures the tau core on start appropriately. The settings are
    *   volatile.
    *   \param cb Callback.
    *   \param caller Calling instance.
     */
    ThermalGrabber(callbackThermalGrabber cb, void* caller);


    //! Constructor of ThermalGrabber.
    /*!
    *   ThermalGrabber is used to connect the "Thermal Capture Grabber USB" device.
    *   The callback is called every time a full frame is received from the device.
    *   In order to use the callback ThermalGrabber needs to know the
    *   calling instance.
    *   ThermalCapture GrabberUSB uses the digital output of the tau core.
    *   Therefor it configures the tau core on start appropriately. The settings are
    *   volatile.
    *   The parameter iSerialUSB makes it possible to connect a
    *   ThermalCapture GrabberUSB by its unique iSerial. This iSerial can be
    *   identified by inspecting the usb system log of the operation system the
    *   ThermalCapture GrabberUSB is connected to.
    *   \param cb Callback.
    *   \param caller Calling instance.
    *   \param iSerialUSB Char string representation of iSerial.
     */
    ThermalGrabber(callbackThermalGrabber cb, void* caller, char* iSerialUSB);

    //! Destructor of ThermalGrabber.
    /*!
    *   Cleans up the ThermalGrabber object memory and waits for
    *   connection thread to close.
    */
    ~ThermalGrabber();

    //! Convert the TauRawBitmap to TauRGBBitmap.
    /*!
    *   TauRawBitmap has 14bit values per pixel that are stored in a 16 bit array.
    *   The converted TauRGBBitmap has 24bit per pixel that are stored in a byte array.
    */
    void convertTauRawBitmapToTauRGBBitmap(TauRawBitmap &tauRawBitmap, TauRGBBitmap &tauRGBBitmap);

    //! Send a command to TauCore.
    /*!
    *   Please have a look at the "FLIR TAU2/QUARK2 SOFTWARE IDD" document for further information.
    *   In general a command consists of function byte (cmd)
    *   and an optional byte array of arguments (data) and
    *   a length info of the data (the function byte/cmd is not counted).
    *   If no additional data are send put a "NULL" for the 2. and "0" for the 3. parameter.
    *   \param cmd The function that is called
    *   \param data Optional data bytes as arguments.
    *   \param data_len The length of the data field.
    */
    void sendCommand(char cmd, char *data, unsigned int data_len);

    //! Get the resolution width.
    /*!
     * Get the resolution width of the tau core that
     * is connected to the ThermalCapture GrabberUSB.
     * \return Width of Resolution or '0' if not found
     */
    unsigned int getResolutionWidth();

    //! Get the resolution height.
    /*!
     * Get the resolution height of the tau core that
     * is connected to the ThermalCapture GrabberUSB.
     * \return Height of Resolution or '0' if not found
     */
    unsigned int getResolutionHeight();

    //! Get Camera Serial Number
    /*!
     * Get Camera Serial Number of the tau core that
     * is connected to the ThermalCapture GrabberUSB.
     * The flir camera serial number is unique.
     * \return Camera Serial Number or '0' if not found
     */
    unsigned int getCameraSerialNumber();

    //! Get Camera Part Number
    /*!
     * Get Camera Part Number of the tau core that
     * is connected to the ThermalCapture GrabberUSB.
     * The ascii based char string contains the
     * flir camera part number that informs about the
     * general type of camera (resolution etc.).
     * \return char* Pointer to the char string or NULL if not available
     */
    const char* getCameraPartNumber() const;


    //! Shutter command
    /*!
    * Triggers a shutter event of the tau core that
    * is used for a Flat Field Correction
    * of the tau core sensor.
    */
    void doFFC();

    //! Set the Gain Mode
    /*!
     * Sets the Gain Mode to Automatic, High, Low or Manual.
     * Remember that Gain Mode is ignored when TLinear is
     * enabled (if TLinear is supported by the device).
     * The enum GainMode is defined in this header.
     * \param gainMode Enum of Type GainMode.
     */
    void setGainMode(thermal_grabber::GainMode gainMode);

    //! Enables TLinear in high resolution
    /*!
     * Enables TLinear mode and
     * sets the resolution of the TLinear digital video
     * to high resolution mode (0.04Kelvin/count in 14-bit digital).
     */
    void enableTLinearHighResolution();


    //! Enable TLinear in low resolution
    /*!
     * Enables TLinear mode and
     * sets the resolution of the TLinear digital video
     * to low resolution mode (0.4Kelvin/count in 14-bit digital).
     */
    void enableTlinearLowResolution();

    //! Disable TLinear Mode
    /*!
     * Disables the TLinear Mode of the TauCore to get raw values.
     */
    void disableTLinear();

    bool is_opened(){return opened;}

private:

    // internally used callback definitions
    static void static_callbackTauData(TauRawBitmap& tauRawBitmap , void* caller);
    void callbackTauData(TauRawBitmap& tRawBitmap);

    // internally used reference to callback and calling instance
    callbackThermalGrabber mCallbackThermalGrabber;
    void* mCallingInstance;
    bool opened;

    // internally used helper function for scaling color values.
    unsigned int scale(unsigned int value, unsigned int lowBound, unsigned int upBound , unsigned int minOutput, unsigned int maxOutput);
};

#endif // THERMALGRABBER_H
