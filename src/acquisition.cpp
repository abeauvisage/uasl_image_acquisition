#include "acquisition.hpp"

#include <stdexcept>
#include <typeinfo>

namespace cam {


//Public functions :

Acquisition::Acquisition()
				: should_run(false)
				, acq_start_package(*this)
				, images_have_been_returned(true)
{}

Acquisition::~Acquisition()
{
	stop_acq();
}

int Acquisition::start_acq()
{
	if(acq_thd.joinable())
	{
		std::cerr << "Acquisition thread is already running." << std::endl;
		return -1;
	}
	else
	{
		//We lock the acquisition start here, because if we request a lock on the acquisition, we want to call the stop_acq function AFTER switching should_run to true.
		//If the stop_acq function is called before, this could result in a deadlock.
		std::unique_lock<std::mutex> mlock(acq_start_package.mtx);
		bool success = acq_start_package.value ? true : acq_start_package.cv.wait_for(mlock, std::chrono::milliseconds(timeout_delay_ms), [this]{return acq_start_package.value;});
		if(!success)
		{
			std::cerr << "Timeout to start acquisition." << std::endl;
			return -2;
		}
		else
		{
			should_run.store(true);
			acq_thd = std::thread(&Acquisition::thread_func, this);
		}
	}
	return 0;
}

int Acquisition::stop_acq()
{

	std::lock_guard<std::mutex> lock(acq_start_package.mtx);
	should_run.store(false);
	if(acq_thd.joinable())
        acq_thd.join();


	return 0;
}

bool Acquisition::is_running()
{

	std::lock_guard<std::mutex> lock(acq_start_package.mtx);
	//The lock is not necessary since we check an atomic boolean
	//However, it ensures that the answer we get is accurate : if we don't lock, and stop_acq was called
	//soon after a start_acq, then should_run will only switch to false after stop_acq acquires the lock
	//Thus, if we wait for the lock ourselves, we will return the correct value : false
	//If we don't lock, we might get the result too soon and get true, even if stop_acq has already been called
	bool is_running = should_run.load();

	return is_running;
}

Camera_params& Acquisition::get_cam_params(size_t idx)
{
	//Get the parameters of a given camera. Throw exceptions if the index is invalid (the return type is preferred to an error code for usability reasons
	stop_acq();

	//Lock the camera vector
	std::lock_guard<std::mutex> lock_cam(camera_vec_mtx);

	if(idx >= camera_vec.size())
	{
		throw std::out_of_range("Index out of range : should be between 0 and " + std::to_string(camera_vec.size()) + " (exclusive).");
	}

	return camera_vec[idx]->get_params();
}

int Acquisition::get_images(std::vector<cv::Mat>& img_vec_out)
{
	std::unique_lock<std::mutex> mlock(images_ready_mtx);//Lock the images vector
	bool success = !images_have_been_returned? true : images_have_changed.wait_for(mlock, std::chrono::milliseconds(timeout_ms), [this]{return !images_have_been_returned;});
	if(!success) return -1;

	std::lock_guard<std::mutex> lock_images(images_vec_mtx);
	img_vec_out.resize(images_vec.size());
	for(size_t i = 0; i < images_vec.size(); ++i)
	{
		images_vec[i].copyTo(img_vec_out[i]);
	}

	images_have_been_returned = true;
	return 0;
}

//Private functions:
void Acquisition::thread_func()
{

	{//Mutex scope
		std::lock_guard<std::mutex> lock_cam(camera_vec_mtx);//Lock the camera vector mutex
		const size_t cam_number = camera_vec.size();

		if(cam_number == 0)
		{
			std::cerr << "Error : trying to launch an acquisition without cameras." << std::endl;
			stop_acq();

			return;
		}

		const bool only_one_camera = (cam_number == 1);//If there is a unique camera

		//Open the trigger if more than 1 camera is started
		if(!only_one_camera)
		{
			trigger.open_vcp();
			if(!trigger.is_opened())
			{
				std::cerr << "Trigger could not be opened. Aborting acquisition." << std::endl;
				stop_acq();
			}
		}

		//Start the acquisition for all cameras. Returns 0 if success, else an error code.
		for(size_t i = 0;i<cam_number && should_run.load(); ++i)
		{
			if(camera_vec[i]->start_acq(only_one_camera) != 0)
			{
				std::cerr << "Camera " << i << " could not be started. Aborting acquisition." << std::endl;
				should_run.store(false);
			}
		}
	}

	while(should_run.load())
	{
		//Send the trigger
		if(trigger.is_opened() && !trigger.send_trigger())
		{
			std::cerr << "Error during triggering." << std::endl;
			continue;
		}

		std::lock_guard<std::mutex> lock_cam(camera_vec_mtx);//Lock the camera vector mutex for all the duration of the processing

		const size_t cam_number = camera_vec.size();

		bool acquisition_ok = true; //If the acquisition is valid

		//Second, get the acquired pictures
		{//Mutex scope
			std::lock_guard<std::mutex> lock_img(images_vec_mtx);//Lock the vector of images
			for(size_t i = 0;i<cam_number; ++i)
			{
				if(camera_vec[i]->retrieve_image(images_vec[i]) != 0)
				{
					acquisition_ok = false;//By design, the size of camera_vec and image_vec should be the same
				}
			}
		}

		if(acquisition_ok)
		{
			std::lock_guard<std::mutex> lock_ready(images_ready_mtx);
			images_have_been_returned = false;
		}


		if(acquisition_ok)
		{
			//If the acquisition is successful, update the status to indicate that new images have been taken
			images_have_changed.notify_all();
		}
	}

	close_cameras();
}

void Acquisition::close_cameras()
{
	//Stop acquisition for each camera
	std::lock_guard<std::mutex> lock_cam(camera_vec_mtx);//Lock the camera vector mutex
	const size_t cam_number = camera_vec.size();

	for(size_t i = 0;i<cam_number; ++i)
	{
		if(camera_vec[i]->stop_acq() != 0)
		{
			std::cerr << "Camera " << i << " could not be stopped." << std::endl;
		}
	}

}
} //namespace cam

