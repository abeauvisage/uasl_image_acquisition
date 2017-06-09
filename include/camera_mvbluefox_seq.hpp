#ifndef IMAGE_ACQUISITION_CAMERA_MVBLUEFOX_SEQ_HPP
#define IMAGE_ACQUISITION_CAMERA_MVBLUEFOX_SEQ_HPP

#include "camera_sequential.hpp"

#include "cond_var_package.hpp"

#include "util_clock.hpp"

#include "opencv2/core/version.hpp"
#if CV_MAJOR_VERSION == 2
#include <opencv2/core/core.hpp>
#elif CV_MAJOR_VERSION == 3
#include <opencv2/core.hpp>
#endif

#include <mvIMPACT_acquire.h>

#include <atomic>
#include <memory> //For unique_ptr
#include <condition_variable>
#include <mutex>

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

static constexpr int timemout_waitfor_ms = 500;//Timeout for a waitfor request for the camera

class BlueFoxParameters : public Camera_params
{
	public:
	BlueFoxParameters(Cond_var_package& package_) : Camera_params(package_),
													p_dev(nullptr),
													pixel_format(pixel_fromat_d)
													{}
	void set_p_dev(mvIMPACT::acquire::Device * p_dev_)
	{
		p_dev = p_dev_;
	}
	
	//Please note that the following set functions stop the acquisition
    void set_image_size(int width, int height);
    void set_agc(bool value);
    void set_aec(bool value);
    void set_image_type(int ocv_color_code);
    void set_trigger_mode(mvIMPACT::acquire::TCameraTriggerMode trigger_mode);
    
    int get_pixel_format() const
    {
    	return pixel_format;
    }
    
    private:
    mvIMPACT::acquire::Device * p_dev;
    int pixel_format;//Pixel format for the output image
    
    bool check_cam() const
    {
    	if(!p_dev)
    	{
    		std::cerr << "Error : Camera not opened." << std::endl;
        	return false;
    	}
    	return true;
    }
}; //class BlueFoxParameters

class CamBlueFoxSeq : public Camera_seq
{
	public:
    CamBlueFoxSeq(Cond_var_package& package_, int camId = -1);//Open a camera by serial number, or the first one found if camId is negative
    virtual ~CamBlueFoxSeq();
    
    int start_acq(bool only_one_camera) override;
    int stop_acq() override;
    int send_image_request() override;
    int retrieve_image_request(cv::Mat& image) override;
    
    virtual BlueFoxParameters& get_params() override
    {    	
    	//Note that the acquisition has to be stopped by the caller
    	return params;
    }
    
    private:
    //Device manager, maybe one in common for all mv cameras
    mvIMPACT::acquire::DeviceManager dev_mgr;
    mvIMPACT::acquire::Device * p_dev;//Interface to device
    std::unique_ptr<mvIMPACT::acquire::FunctionInterface> p_fi;
    
    BlueFoxParameters params;//Interface to modify the parameters of the camera
    
    bool opened;//True if the camera has been successfully opened (different from mvIMPACT::acquire::Device::isOpen)
    
    void init(int camId = -1);//Initialisation function for the camera
    
    void empty_request_queue();//Empty the request queue of the camera
    
    void manually_start_acquisition_if_needed();// Start the acquisition manually if this was requested(this is to prepare the driver for data capture and tell the device to start streaming data)
    void manually_stop_acquisition_if_needed();// Stop the acquisition manually if this was requested
    
    template<typename T> bool check_property(const T& property_value, mvIMPACT::acquire::EnumPropertyI<T>& property)
    {
    	//Check if a given integer property is present in the device
    	std::vector<T> modes_available;
		property.getTranslationDictValues(modes_available);
		for(typename std::vector<T>::iterator it = modes_available.begin(); it != modes_available.end(); ++it)
		{
				if(*it == property_value)//the property is available is available
				{
					return true;
				}
		}
		return false;
    }
    
    void export_image(int width, int height, void * vpData, int pixel_format, cv::Mat& image)
    {
        //Copy the image from the internal buffer of the camera to currentImage.
        cv::Mat temp_img(height, width, pixel_format, vpData);
        temp_img.copyTo(image);//We construct a temporary header to avoid memory alignment issue (useful compared to a memcpy ?)
    }
    
}; //class CamBlueFoxSeq
} //namespace cam

#endif
