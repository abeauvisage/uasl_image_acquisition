#ifndef IMAGE_ACQUISITION_CAMERA_HPP
#define IMAGE_ACQUISITION_CAMERA_HPP

#include "opencv2/core/version.hpp"
#if CV_MAJOR_VERSION == 2
#include <opencv2/core/core.hpp>
#elif CV_MAJOR_VERSION == 3
#include <opencv2/core.hpp>
#endif

#include <thread>
#include <condition_variable>
#include <atomic>

#include "util_clock.hpp"

namespace cam
{

class MatSync
{
    //Class used to synchronize the use of a vector of cv::Mat
    public:
    MatSync();

    void set_size(size_t size);

    void set_mat(size_t idx, const cv::Mat& img);
    
    int get_last_images(std::vector<cv::Mat>& images, int * images_nb_, std::chrono::time_point<clock_type> * time_pt);

    private:
    std::vector<cv::Mat> mat_vec;
    std::mutex mtx_global;//Global mutex for all the matrices
    std::unique_lock<std::mutex> lock_global;
    std::atomic<unsigned int> entry_cnt;
    std::atomic<unsigned int> exit_cnt;
    bool finished_locking;

    std::condition_variable cond;
    std::mutex mtx_wait_lock;
    
    std::chrono::time_point<clock_type> time_point;//Time when the transfer from the camera buffer to the computer STARTED (i.e. not the actual time of capture)
    int image_nb;//Index of the current set of images
    
    bool images_updated;
    std::condition_variable images_updated_cond;
};

class Barrier 
{
public:
    explicit Barrier(size_t initial_count) :
      threshold(initial_count), 
      count(initial_count), 
      generation(0),
      open(false)
      {}

    void wait() 
    {
    	std::unique_lock<std::mutex> local_lock(mtx);
    	if(open) return;
      size_t local_gen = generation;
      if (!--count) 
      {
              ++generation;
              count = threshold;
              local_lock.unlock();
              cond.notify_all();
              return;
      }
      cond.wait(local_lock, [this, local_gen] { return local_gen != generation; });
    }
    
    void reset(size_t new_count)//Reset the counter and modify the threshold. DO NOT call this method while the barrier is being used by a thread
    {
    	std::unique_lock<std::mutex> local_lock(mtx);
    	open = false;
    	++generation;
    	threshold = new_count > 0 ? new_count : 1;//New count should always be > 0
    	count = threshold;
    	local_lock.unlock();
    	cond.notify_all();
    }
    
    void open_barrier()//Open the barrier, which disable it and free all threads waiting for it
    {
    	std::unique_lock<std::mutex> local_lock(mtx);
    	open = true;
    	++generation;
    	local_lock.unlock();
    	cond.notify_all();
    }

private:
    std::mutex mtx;
    std::condition_variable cond;
    size_t threshold;//Keep the initial_count in memory
    size_t count;//Count the number of threads waiting
    size_t generation;//Become different from local_gen when the number of waiting threads reach threshold
    
    bool open;//If the barrier is open, the call to wait does nothing
};


class Camera
{
	public:
    virtual ~Camera() {}
	virtual int take_picture(cv::Mat& image) = 0;
	virtual int start_acq(Barrier* init_bar, Barrier* acquiring_bar, MatSync* mat_sync, unsigned int cam_idx) = 0;
    virtual int stop_acq() = 0;

    virtual void set_write_on_disk(bool value, const std::string& path_to_folder_ , const std::string&
    image_prefix_) = 0;//Activate/Deactivate the writing of the files on the disk, the directory is designated by the path to folder, the image prefix is the name of the image before the number

    virtual void set_image_display(bool value, const std::string& window_name_) = 0;//Activate/Deactivate the display of the image

    virtual void set_img_keep_rate(unsigned int value) = 0;
};
} //namespace cam

#endif
