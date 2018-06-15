#ifndef UASL_IMAGE_ACQUISITION_ACQUISITION_HPP
#define UASL_IMAGE_ACQUISITION_ACQUISITION_HPP

#include "camera_sequential.hpp"
#include "cond_var_package.hpp"
#include "util_clock.hpp"
#include "trigger.hpp"

#include <vector>
#include <memory>
#include <iostream>

#include "opencv2/core/version.hpp"
#if CV_MAJOR_VERSION == 2
#include <opencv2/core/core.hpp>
#elif CV_MAJOR_VERSION == 3
#include <opencv2/core.hpp>
#endif

#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <string>


namespace cam {

static constexpr int timeout_ms = 1000;//Timeout value in milliseconds for retrieving the set of images
static constexpr int timeout_delay_ms = 5000;//Timeout value in milliseconds for getting the acquisition lock

static constexpr char default_cam_id[] = "";//Default id value for the camera

class Acquisition
{
	public:
	Acquisition(const clock_type::time_point& time_origin = clock_type::time_point());
	virtual ~Acquisition();

	int start_acq();//Start the acquisition for all cameras
	int stop_acq();//Stop the acquisition for all cameras

	bool is_running();//Returns true if an acquisition is currently running

	template <CameraType T>
	int add_camera(const std::string& cam_id = std::string(default_cam_id))//Add a camera (stops the acquisition)
	{
		//This function has to be in the header due to the template
		stop_acq();//Start by stopping any acquisition

		//Lock both the camera and image vectors at the same time
		std::unique_lock<std::mutex> lock_cam(camera_vec_mtx, std::defer_lock);
		std::unique_lock<std::mutex> lock_img(images_vec_mtx, std::defer_lock);
		std::lock(lock_cam, lock_img);

		std::unique_ptr<Camera_seq> cam_ptr = nullptr;

		int ret_value = 0;

		try
		{
			cam_ptr = Camera_seq::get_instance<T>(acq_start_package,cam_id);
		}
		catch(const std::exception& e)
		{
			std::cerr << "Exception during a camera addition : " << e.what() << std::endl;
			ret_value = -1;
		}

		if(cam_ptr)
		{
			camera_vec.push_back(std::move(cam_ptr));
			images_vec.resize(camera_vec.size());
		}
		else
		{
			std::cerr << "Error : camera type non recognized. Camera cannot be added." << std::endl;
			std::cerr << "Please check that the corresponding camera library is correctly link to the executable." << std::endl;
		}

		return ret_value;
	}


	Camera_params& get_cam_params(size_t idx);//Get the parameters of a specific camera to modify them (stops the acquisition)

	int64_t get_images(std::vector<cv::Mat>& img_vec);//Get an image from each camera

	speed_t get_trigger_baurate();
	std::string get_trigger_port_name();
	void set_trigger_baurate(const speed_t& baurate);
	void set_trigger_port_name(const std::string& portname);

	private:

    std::vector<std::unique_ptr<Camera_seq>> camera_vec;//Vector holding the cameras
	std::mutex camera_vec_mtx;//Mutex to protect the camera vector

	std::thread acq_thd;//Acquisition thread
	std::atomic<bool> should_run;//True if the acquisition thread should continue
	//Please note there is an interaction between should_run (if the acquisition should continue or not) and param_package (if we can start the acquisition or modify the parameters) :
	//The modification of param_package (via Acquisition lock) calls stop_acq(), which implies switching should_run to false. To avoid deadlock, it is then necessary to prevent stop_acq from being called before the switch of should_run to true. Thus, it is mandatory to lock package.mtx before switching should_run to false from outside the acquisition thread. This is done directly in stop_acq(). This means that you should never call stop_acq() with a lock on package.mtx, or there will be a deadlock.

	Cond_var_package acq_start_package;//Structure which manages the locking/unlocking of the possibility to start an acquisition. value is true if the acquisition can start, false otherwise (each operation preventing the acquisition uses this variable to start, so set this to false by using cv if you want to prevent the acquisition from starting). Please note that note that several operations can use this structure (e.g. each modification of a parameter)

	std::vector<cv::Mat> images_vec;//Vector holding the images
	std::mutex images_vec_mtx;//Mutex protecting the vector of images

	std::condition_variable images_have_changed;//Notification when a new set of images is registered
    bool images_have_been_returned;//If the current set of images have been used
    std::mutex images_ready_mtx;//Mutex to protect images_have_been_returned

    Trigger_vcp trigger;//External trigger
    std::string trigger_port_name;
    speed_t trigger_baudrate;
    std::mutex trigger_mtx;

    clock_type::time_point origin_tp; // used as origin for image timestamps. Initialized with the clock epoch, can be specified in the constructor.
    std::atomic<clock_type::time_point> current_tp; // time point of the latest trigger, will be converted into an image timestamp by get_images only if all images have been acquired.

	void thread_func();//Acquisition function launched by the acquisition thread
	void close_cameras();//Close each camera

}; //class Acquisition

class Acquisition_lock
{
	//The goal of this class is to manage a lock described by Cond_var_package
	//The constructor waits to acquire the lock, and set the value boolean to false.
	//The destructor acquires the lock and set the boolean to true
	//Ultimately, this allow us to prevent starting an acquisition:
	//Declare an instance of this class with the Cond_var_package of the acquisition class, and the Acquisition start is locked,
	//as well as other modifications preventing an acquisition to start (such as parameter modification)
	public:
	Acquisition_lock(Cond_var_package& package) : valid(true)
												, cv(package.cv)
												, value(package.value)
												, mtx(package.mtx)
	{
		//First prevent the acquisition from starting, then stop it
		{
			std::unique_lock<std::mutex> mlock(mtx);//Acquire the lock on acquisition start
			bool success = value ? true : cv.wait_for(mlock, std::chrono::milliseconds(timeout_delay_ms), [this]{return value;});
			if(!success)
			{
				std::cerr << "Timeout to lock acquisition." << std::endl;
				valid = false;
			}
			value = false;
		}
		package.acq_ref.stop_acq();//Stop the acquisition (this is done after switching value to false to avoid a call to start_acq in between)
	}

	~Acquisition_lock()
	{
		{//Mutex scope
			std::lock_guard<std::mutex> mlock(mtx);
			if(value)
			{
				std::cerr << "Warning : unlocking acquisition without locking first. Ignoring this call." << std::endl;
			}
			value = true;
		}
		cv.notify_all();
	}

	bool is_valid() const
	{
		//In case of timeout, this function returns false
		return valid;
	}

	private:
	bool valid;

	std::condition_variable& cv;
	bool& value;
	std::mutex& mtx;
}; //class Acquisition_lock
} //namespace cam

#endif
