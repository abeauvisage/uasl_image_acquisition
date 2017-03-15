#ifndef IMAGE_ACQUISITION_MULTIPLE_CAMERA_SYNC_HPP
#define IMAGE_ACQUISITION_MULTIPLE_CAMERA_SYNC_HPP

#include "camera.hpp"

namespace cam
{
static constexpr int timeout_ms = 500;//Timeout value in milliseconds for acquiring the set of images

class CamSync
{
    public:
    CamSync();
    virtual ~CamSync();

    int start_acq();
    int stop_acq();

    int add_camera(Camera& cam);
    int remove_camera(size_t idx);

    int take_pictures(std::vector<cv::Mat>& images, int * images_nb_= nullptr, std::chrono::time_point<clock_type> * time_pt = nullptr);

    void set_write_on_disk(bool value, const std::string& path_to_folder_, const std::string&
    image_prefix_);//Activate/Deactivate the writing of the files on the disk, the directory is designated by the path to folder, the image prefix is the name of the image before the number
    
    void set_image_display(bool value, const std::string& window_name_);//Activate/Deactivate the display of the image
    void set_img_keep_rate(unsigned int value);

    private:
    std::vector<std::reference_wrapper<Camera>> cam_vec;

    Barrier init_bar;//This barrier is used once, it goes to the beginning of the axquiring thread, before the main loop
    Barrier acquiring_bar;//This barrier is used at each iteration, it goes just BEFORE sending the order of acquiring
    //an image
    
    MatSync mat_sync;//Deal with the synchronisation of the transfer of the images
    
    unsigned int img_keep_rate;
};

}//namespace cam

#endif
