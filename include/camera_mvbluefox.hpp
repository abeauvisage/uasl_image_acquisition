#ifndef IMAGE_ACQUISITION_CAMERA_MVBLUEFOX_HPP
#define IMAGE_ACQUISITION_CAMERA_MVBLUEFOX_HPP

#include "camera.hpp"
#include "util_clock.hpp"

#include "opencv2/core/version.hpp"
#if CV_MAJOR_VERSION == 2
#include <opencv2/core/core.hpp>
#elif CV_MAJOR_VERSION == 3
#include <opencv2/core.hpp>
#endif

#include <mvIMPACT_acquire.h>

#include <memory> //For unique_ptr
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <iostream>

namespace cam
{
//The type of the property is the type in the template parameter of the enum in the doc
static const mvIMPACT::acquire::TImageDestinationPixelFormat pixel_format_output_d(idpfMono8);
static constexpr int pixel_fromat_d(CV_8U);//Default pixel format //(has to be coherent with pixel_fromat_output_d)
static constexpr int width_d(640);//Default width
static constexpr int height_d(480);//Default height
static const mvIMPACT::acquire::TAutoGainControl agc_d(agcOff);//Default for the agc. To activate, set to
//agcOn
static const mvIMPACT::acquire::TAutoExposureControl aec_d(aecOff);//Default for the aec. To activate, set to 
//aecOn
static const mvIMPACT::acquire::TCameraTriggerMode trigger_d(ctmContinuous);//Default setting for the trigger mode. See
//https://www.matrix-vision.com/manuals/SDK_CPP/group__DeviceSpecificInterface.html#ga7d880247a3af52241ce96ba703c526a1
//for the details.

static constexpr int timeout_ms_get_image = 500;//Timeout value for getting images from the outside thread
static constexpr int timemout_waitfor_ms = 500;//Timeout for a waitfor request for the camera

class CamBlueFox : public Camera
{
    public:
    CamBlueFox();//Open the first camera available
    CamBlueFox(int camId);//Open a camera by serial number
    ~CamBlueFox();

    int start_acq(Barrier* init_bar, Barrier* acquiring_bar, MatSync * mat_sync, unsigned int cam_idx);
    int start_acq()
	{
		return start_acq(nullptr, nullptr, nullptr, 0);
	}	
    int stop_acq();
    int take_picture(cv::Mat& image, int * image_nb_, std::chrono::time_point<clock_type> * time_pt);
    int take_picture(cv::Mat& image)
    {
        return take_picture(image, nullptr, nullptr);
    }
		
    void set_write_on_disk(bool value, const std::string& path_to_folder_, const std::string& image_prefix_);//Activate/Deactivate the writing of the files on the disk, the directory is designated by the path to folder, the image prefix is the name of the image before the number
    void set_write_on_disk(bool value)
    {
    		set_write_on_disk(value, std::string(), std::string());
    }
    
    void set_image_display(bool value, const std::string& window_name_);//Activate/Deactivate the display of the image
    void set_image_display(bool value)
    {
    		set_image_display(value, std::string());
    }
    void set_img_keep_rate(unsigned int value);

		unsigned int get_image_nb() const;//Get the internal counter
		int get_image_format() const;//Get the image format
		
    //Please note that the following set functions stop the acquisition
    void set_image_size(int width, int height);
    void set_agc(bool value);
    void set_aec(bool value);
    void set_image_type(int ocv_color_code);


    private:
    //Device manager, maybe one in common for all mv cameras
    mvIMPACT::acquire::DeviceManager dev_mgr;
    mvIMPACT::acquire::Device * p_dev;//Interface to device
    std::unique_ptr<mvIMPACT::acquire::FunctionInterface> p_fi;
    
    bool isOpened;//True of the camera has been successfully opened (different from mvIMPACT::acquire::Device::isOpen)

    std::atomic<bool> started;//True if the acquisition has started
    std::thread thd;

    std::chrono::time_point<clock_type> image_current_time;//Time of last image

    mutable std::mutex mtx_image_current;//Protect current image and the associated variables
    cv::Mat image_current;//Last image acquired
    int pixel_format;//Pixel format for the output image
    unsigned int image_nb;//Image number
    std::atomic<unsigned int> img_keep_rate;//During the acquisition, keep 1 image out of img_keep_rate
    std::condition_variable image_has_changed;//Notification when a new image is added
    bool image_has_been_returned;//If the current image has been used

    std::atomic<bool> write_on_disk;
    std::string path_to_folder;//Path to the folder for image writing
    std::string image_prefix;
    std::mutex mtx_writing_var;//Protect the writing variables path_to_folder and image_prefix
    std::atomic<bool> show_on_screen;
    std::string window_name;
	bool window_name_changed;//If the window name has changed
    std::mutex mtx_image_display;//Protect the window_name and window_name_changed variables
    
    void init(int camId = -1);//Initialisation function for the camera
    void thread_func(Barrier* init_bar, Barrier* acquiring_bar, MatSync * mat_sync, unsigned int cam_idx);//Thread function to acquire images

    void write_image_disk();//Write the current image on the disk
    void show_image();//Show the current image on the screen

    void empty_request_queue();//Empty the request queue of the camera

    inline void import_image(int width, int height, void * vpData)
    {
        //Copy the image from the internal buffer of the camera to currentImage.
        cv::Mat temp_img(height, width, pixel_format, vpData);
        temp_img.copyTo(image_current);//We construct a temporary header to avoid memory alignment issue (useful compared to a memcpy ?)
    }


}; //class CamBlueFox

// Start the acquisition manually if this was requested(this is to prepare the driver for data capture and tell the device to start streaming data)
inline void manuallyStartAcquisitionIfNeeded( mvIMPACT::acquire::Device* p_dev, const mvIMPACT::acquire::FunctionInterface& fi )
{
    if( p_dev->acquisitionStartStopBehaviour.read() == mvIMPACT::acquire::assbUser )
    {
        const mvIMPACT::acquire::TDMR_ERROR result = static_cast<mvIMPACT::acquire::TDMR_ERROR>(fi.acquisitionStart());
        if(result != mvIMPACT::acquire::DMR_NO_ERROR )
        {
            std::cerr << "'FunctionInterface.acquisitionStart' returned with an unexpected result: " << result << "(" << mvIMPACT::acquire::ImpactAcquireException::getErrorCodeAsString(result) << ")" << std::endl;
        }
    }
}
// Stop the acquisition manually if this was requested
inline void manuallyStopAcquisitionIfNeeded( mvIMPACT::acquire::Device* p_dev, const mvIMPACT::acquire::FunctionInterface& fi )
{
    if( p_dev->acquisitionStartStopBehaviour.read() == mvIMPACT::acquire::assbUser )
    {
        const mvIMPACT::acquire::TDMR_ERROR result = static_cast<mvIMPACT::acquire::TDMR_ERROR>(fi.acquisitionStop());
        if( result != mvIMPACT::acquire::DMR_NO_ERROR)
        {
            std::cerr << "'FunctionInterface.acquisitionStop' returned with an unexpected result: " << result << "(" << mvIMPACT::acquire::ImpactAcquireException::getErrorCodeAsString(result) << ")" << std::endl;
        }
    }
}

} //namespace cam
#endif
