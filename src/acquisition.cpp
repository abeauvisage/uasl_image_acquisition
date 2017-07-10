#include "acquisition.hpp"

//Camera specific headers
#if BLUEFOX_FOUND
#include "camera_mvbluefox_seq.hpp"
#endif

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
	//Start the acquisition for all cameras. Returns 0 if success, else an error code.
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
	if(acq_thd.joinable()) acq_thd.join();
		
	return 0;
}

int Acquisition::add_camera(CameraType type, int id)
{
	stop_acq();//Start by stopping any acquisition
	
	//Lock both the camera and image vectors at the same time
	std::unique_lock<std::mutex> lock_cam(camera_vec_mtx, std::defer_lock);
    std::unique_lock<std::mutex> lock_img(images_vec_mtx, std::defer_lock);
    std::lock(lock_cam, lock_img);
    
	int ret_value = 0;
	switch(type)
	{
		#if BLUEFOX_FOUND
		case bluefox:
			try
			{
				if(id != default_cam_id) camera_vec.push_back(std::unique_ptr<CamBlueFoxSeq>(new CamBlueFoxSeq(acq_start_package, id)));
				else camera_vec.push_back(std::unique_ptr<CamBlueFoxSeq>(new CamBlueFoxSeq(acq_start_package)));
				images_vec.resize(camera_vec.size());				
			}
			catch(const std::exception& e)
			{
				std::cerr << "Exception during a camera addition : " << e.what() << std::endl;
				ret_value = -1;
			}
			break;
		#endif			
		default:
			std::cerr << "Error : camera type non recognized." << std::endl;
			ret_value = -2;
			break;
	}
	
	return ret_value;
}

Camera_params& Acquisition::get_cam_params(size_t idx)
{
	//Get the parameters of a given camera. Throw exceptions if the index is invalid (the return type is preferred to an error code for usability reasons
	stop_acq();
	
	//Lock the camera vector
	std::lock_guard<std::mutex> lock_cam(camera_vec_mtx);
	
	if(idx >= camera_vec.size())
	{
		throw std::out_of_range("Index out of range : should be between 0 and " + std::to_string(camera_vec.size() - 1) + " (inclusive).");
	}
	
	return camera_vec[idx]->get_params();
}

int Acquisition::get_images(std::vector<cv::Mat>& img_vec_out)
{
	std::unique_lock<std::mutex> mlock(images_vec_mtx);//Lock the images vector
	
	bool success = !images_have_been_returned? true : images_have_changed.wait_for(mlock, std::chrono::milliseconds(timeout_ms), [this]{return !images_have_been_returned;});
	
	if(!success) return -1;
	
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
		}
	
		const bool only_one_camera = (cam_number == 1);
	
		for(size_t i = 0;i<cam_number; ++i)
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
		std::lock_guard<std::mutex> lock_cam(camera_vec_mtx);//Lock the camera vector mutex for all the duration of the processing
		
		const size_t cam_number = camera_vec.size();
		
		bool acquisition_ok = true; //If the acquisition is valid
		
		//Please note that even if there is an error, we still try to retrieve the images to preserve synchronization (e.g. for the bluefox,
		//a request is always treated).
		//First, send the acquisition request
		for(size_t i = 0;i<cam_number; ++i)
		{
			if(camera_vec[i]->send_image_request() != 0) acquisition_ok = false;
		}
		
		//Second, get the acquired pictures
		{//Mutex scope
			std::lock_guard<std::mutex> lock_img(images_vec_mtx);//Lock the vector of images
			for(size_t i = 0;i<cam_number; ++i)
			{
				if(camera_vec[i]->retrieve_image_request(images_vec[i]) != 0) acquisition_ok = false;//By design, the size of camera_vec and image_vec should be the same
			}
			
			if(acquisition_ok) images_have_been_returned = false;
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

