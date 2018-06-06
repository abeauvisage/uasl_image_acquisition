#ifndef UASL_IMAGE_ACQUISITION_CAMERA_TAU2_HPP
#define UASL_IMAGE_ACQUISITION_CAMERA_TAU2_HPP

#include "camera_sequential.hpp"
#include "cond_var_package.hpp"
#include "util_clock.hpp"

#include "opencv2/core/version.hpp"
#if CV_MAJOR_VERSION == 2
#include <opencv2/core/core.hpp>
#elif CV_MAJOR_VERSION == 3
#include <opencv2/core.hpp>
#endif

#include <thermalgrabber.h>

#include <atomic>
#include <memory> //For unique_ptr
#include <condition_variable>
#include <mutex>

#include <iostream>

namespace cam
{
static constexpr int pixel_format_dt(CV_16U);//Default pixel format //(has to be coherent with pixel_fromat_output_d)
static constexpr int width_dt(640);//Default width (if max width is not available)
static constexpr int height_dt(512);//Default height (if max height is not available)
static constexpr int startx_dt(0);//Default width (if max width is not available)
static constexpr int starty_dt(0);//Default height (if max height is not available)
static constexpr int timeout_retrieve_ms=100;

class Tau2Parameters : public Camera_params
{
	public:
	Tau2Parameters(Cond_var_package& package_) :    Camera_params(package_),
                                                    image_ROI(startx_dt,starty_dt,width_dt,height_dt),
													pixel_format(pixel_format_dt)
													{}

	//Please note that the following set functions stop the acquisition
    void set_image_roi(int startx, int starty, int width, int height);
    void set_pixel_format(int pixel_format);
    void set_trigger_mode(thermal_grabber::TriggerMode trigger_mode);

    void setThermalGrabber(ThermalGrabber* p_grab_){
        p_grab = p_grab_;
    }

    int get_pixel_format() const
    {
    	return pixel_format;
    }

    cv::Rect get_image_roi() const{
        return image_ROI;
    }

    private:
    ThermalGrabber* p_grab; //thermal grabber
    cv::Rect image_ROI;
    int pixel_format;//Pixel format for the output image

}; //class Tau2Parameters

class CamTau2 : public Camera_seq
{
	public:
    CamTau2(Cond_var_package& package_, const std::string& cam_id);//Open a camera by serial number, or the first one found if camId is negative
    virtual ~CamTau2();

    int start_acq(bool only_one_camera) override;
    int stop_acq() override;
    int retrieve_image(cv::Mat& image) override;

    virtual Tau2Parameters& get_params() override
    {
    	//Note that the acquisition has to be stopped by the caller
    	return params;
    }

    private:

    std::unique_ptr<ThermalGrabber> p_grab; //thermal grabber
    Tau2Parameters params;//Interface to modify the parameters of the camera

    bool new_image_available;
    std::condition_variable cv;
    std::mutex image_available_mutex;
    cv::Mat image_acquired;
    bool opened;//True if the camera has been successfully opened (different from mvIMPACT::acquire::Device::isOpen)

    void init(const std::string& cam_id);//Initialisation function for the camera
    static void callbackTauImage(TauRawBitmap& tauRawBitmap, void* caller);

}; //class CamTau2

template<>
std::unique_ptr<Camera_seq> Camera_seq::get_instance<tau2>(Cond_var_package& package, const std::string& cam_id);

} //namespace cam

#endif
