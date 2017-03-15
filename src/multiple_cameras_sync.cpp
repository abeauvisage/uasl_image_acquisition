#include "multiple_camera_sync.hpp"
#include "camera.hpp"

#include <iostream>
namespace cam
{

CamSync::CamSync():
    init_bar(1),
    acquiring_bar(1),
    img_keep_rate(1)
{}

CamSync::~CamSync()
{
    stop_acq();
}
int CamSync::start_acq()
{
    if(cam_vec.size() < 2)
    {
        std::cerr << "Error : cannot start a synchronized acquisition with less than 2 cameras" << std::endl;
        return -1;
    }

    stop_acq();//Stop all cameras if some of them were used before

    init_bar.reset(cam_vec.size());
    acquiring_bar.reset(cam_vec.size());
    
    mat_sync.set_size(cam_vec.size());

    //Start all acquisition
    for(std::vector<std::reference_wrapper<Camera>>::size_type i = 0; i != cam_vec.size(); ++i)
    {
        cam_vec[i].get().start_acq(&init_bar, &acquiring_bar, &mat_sync, i);
    }
}

int CamSync::stop_acq()
{
		init_bar.open_barrier();
    acquiring_bar.open_barrier();
    for(std::vector<std::reference_wrapper<Camera>>::iterator it = cam_vec.begin(); it != cam_vec.end(); ++it)
    {
        it->get().stop_acq();
    }//Please note that each thread will stop at a different time, thus if you are writing images to the disk using the
    //acquiring thread, you might end up with a slightly different number of images for each camera
}

int CamSync::add_camera(Camera& camera)
{
    //Add a camera to the list (stop the acquisition)
    stop_acq();
    camera.stop_acq();

    //Add the new camera
    cam_vec.push_back(std::ref(camera));

    camera.set_img_keep_rate(img_keep_rate);
    return 0;
}

int CamSync::remove_camera(size_t idx)
{
    //Remove a camera from the list (stop the acquisition)
    stop_acq();
    if(idx < 0 || idx >= cam_vec.size())
    {
        std::cerr << "Out of range index (" << idx << " for only " << cam_vec.size() << " elements." << std::endl;
        return -1;
    }
    cam_vec.erase(cam_vec.begin() + idx);
    return 0;
}

int CamSync::take_pictures(std::vector<cv::Mat>& images, int * images_nb_, std::chrono::time_point<clock_type> * time_pt)
{
    return mat_sync.get_last_images(images, images_nb_, time_pt);
}

void CamSync::set_write_on_disk(bool value, const std::string& path_to_folder_, const std::string&
    image_prefix_)
{
		//To ensure consistency, we stop all the cameras before (whereas with a single camera, this can be changed during an acquisition)
    stop_acq();
    for(std::vector<std::reference_wrapper<Camera>>::iterator it = cam_vec.begin(); it != cam_vec.end(); ++it)
    {
    		//Add the index of the camera at the end of the prefix
        it->get().set_write_on_disk(value, path_to_folder_, image_prefix_ + std::to_string(it - cam_vec.begin()) + '_');
    }
}

void CamSync::set_image_display(bool value, const std::string& window_name_)
{
		//To ensure consistency, we stop all the cameras before (whereas with a single camera, this can be changed during an acquisition)
		stop_acq();
		
		for(std::vector<std::reference_wrapper<Camera>>::iterator it = cam_vec.begin(); it != cam_vec.end(); ++it)
    {
    		//Add the index of the camera at the end of the name
        it->get().set_image_display(value, window_name_ + '_' + std::to_string(it - cam_vec.begin()));
    }
}

void CamSync::set_img_keep_rate(unsigned int value)
{
    if(value == 0)
    {
        std::cerr << "Error : Keep rate cannot be null." << std::endl;
    }
    stop_acq();
    img_keep_rate = value;

    for(std::vector<std::reference_wrapper<Camera>>::iterator it = cam_vec.begin(); it != cam_vec.end(); ++it)
    {
        it->get().set_img_keep_rate(value);
    }
}

MatSync::MatSync() :
		lock_global(mtx_global, std::defer_lock),
		entry_cnt(0),
		exit_cnt(0),
		finished_locking(false),
		image_nb(0),
		images_updated(false)
		{}

void MatSync::set_size(size_t size)
{
		//Resize mat_vec
		std::lock_guard<std::mutex> mlock(mtx_global);
		mat_vec.resize(size);
}

void MatSync::set_mat(size_t idx, const cv::Mat& img)
{
    if(idx<mat_vec.size())
    {
        //The first to enter this function locks the global lock, then the other wait for the lock to be taken
        if(entry_cnt.fetch_add(1, std::memory_order_acq_rel) == 0)
        {
            lock_global.lock();
            {
                std::lock_guard<std::mutex> mlock(mtx_wait_lock);
                time_point = clock_type::now();
                finished_locking = true;
            }
            cond.notify_all();
        }
        else
        {
            std::unique_lock<std::mutex> mlock(mtx_wait_lock);
            cond.wait(mlock, [this]{return finished_locking;});//Note that we do not enter the wait if finished loading
            //is true
        }
        //The condition variable is there to ensure that no individual image is touched before we have the global
        //lock. As we expect the waiting time to be really short, we could think about a spin lock instead

        //Do the actual work
        if(!img.empty()) img.copyTo(mat_vec[idx]);
        else mat_vec[idx].release();

        //Check if we are the last to copy, if this is the case, we clean up for the next set of call
        if(exit_cnt.fetch_add(1, std::memory_order_acq_rel) == mat_vec.size() - 1)
        {
            entry_cnt.store(0, std::memory_order_acq_rel);
            exit_cnt.store(0, std::memory_order_acq_rel);
            finished_locking = false;
            ++image_nb;
            images_updated = true;
            lock_global.unlock();
            images_updated_cond.notify_all();
        }
    }
}

int MatSync::get_last_images(std::vector<cv::Mat>& images, int * image_nb_, std::chrono::time_point<clock_type> * time_pt)
{
    images.resize(mat_vec.size());
    bool valid = true;
    std::unique_lock<std::mutex> mlock(mtx_global);
    
    if(!images_updated_cond.wait_for(mlock, std::chrono::milliseconds(timeout_ms), [this]{return images_updated;}))
    {
        valid = false;
    }

    for(std::vector<cv::Mat>::size_type i = 0; i != mat_vec.size(); ++i)
    {
        if(mat_vec[i].empty()) valid = false;
        else
        {
            mat_vec[i].copyTo(images[i]);
        }
    }
    if(image_nb_ != nullptr) *image_nb_ = image_nb;
    if(time_pt != nullptr) *time_pt = time_point;
    images_updated = false;
    return valid? 0 : -1;
}

} //namespace cam

