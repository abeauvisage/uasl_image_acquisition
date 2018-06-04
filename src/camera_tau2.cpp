#include "camera_tau2.hpp"
#include "acquisition.hpp"

#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

namespace cam
{

template<>
std::unique_ptr<Camera_seq> Camera_seq::get_instance<tau2>(Cond_var_package& package, int id)
{
	return std::unique_ptr<CamTau2>(new CamTau2(package, id));				
} 

void CamTau2::callbackTauImage(TauRawBitmap& tauRawBitmap, void* caller)
{
    CamTau2* ptr = static_cast<CamTau2*>(caller);
    if(!ptr)
        return;

    cv::Mat img(tauRawBitmap.height,tauRawBitmap.width,CV_16U,tauRawBitmap.data);

    double m = 10.0/64.0;
    double mean_img = cv::mean(img)[0];
    img = img * m + (127-mean_img* m);
    img.convertTo(img, CV_8U);
    cv::equalizeHist(img,img);
    {
        std::lock_guard<std::mutex> lock(ptr->image_available_mutex);
        img.copyTo(ptr->image_acquired);
        ptr->new_image_available = true;
        ptr->cv.notify_all();
    }
}

int CamTau2::retrieve_image(cv::Mat& image)
{

    if(!opened)
        return -10;

    std::unique_lock<std::mutex> mlock(image_available_mutex);
    bool success = new_image_available ? true : cv.wait_for(mlock, std::chrono::milliseconds(timeout_retrieve_ms), [this] {return new_image_available;});
    if(!success)
        return -1;

    image_acquired.copyTo(image);
    new_image_available = false;

    return 0;
}

void Tau2Parameters::set_image_size(int width_,int height_)
{

    if(p_grab->is_opened())
    {
        width_=640;
        height_=480;
//        p_grab->setResolutionWidth(width_);
//        p_grab->setResolutionHeight(height_);
    }
    else
    {
        std::cerr << "[Tau2] could not set resolution. Camera is not connected!" << std::endl;
    }
}

void Tau2Parameters::set_trigger_mode(int trigger_mode_)
{
    //TODO
    trigger_mode_=0;
}

CamTau2::CamTau2(Cond_var_package& package_, int camID_) : params(package_), new_image_available(false), opened(false)
{
    init(camID_);
}

CamTau2::~CamTau2()
{
    stop_acq();
}

int CamTau2::start_acq(bool only_one_camera)
{

    if(!opened)
    {
        return -10;
    }

//    std::cout << "one cam: " << std::endl;
    if(!only_one_camera)
    {
        //set cam to trigger mode (slave)
        p_grab->doFFC();
    }
    else
    {
        //set cam to continous
    }

    return 0;
}

int CamTau2::stop_acq()
{

    return 0;
}



void CamTau2::init(int camID)
{

    p_grab = std::unique_ptr<ThermalGrabber>(new ThermalGrabber(callbackTauImage, this));

    //TO DO check nb cam
    if(camID == -1)
    {
        //get first camera found
    }
    else
    {
        // get correponding cam
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    opened = p_grab->is_opened();
    if(!opened)
    {
        std::cerr << "[Tau2] could not open camera, check connection." << std::endl;
        p_grab.reset();
    }

    //open camera return if failed
//    params.set_image_size(width_dt,height_dt);
//    params.set_trigger_mode(trigger_dt);
//    std::cout << "before" << std::endl;
    std::cout << "[Tau2] Camera " << p_grab->getCameraSerialNumber() << " has been opened successfully" << std::endl;

    if(!clock_type::is_steady)
    {
        std::cerr << "Warning : non steady clock type, timer might go back in time." << std::endl;
    }

}

}
