#include "acquisition.hpp"

#include "util_signal.hpp"

#include "camera_mvbluefox.hpp"
#include "camera_tau2.hpp"

#include <opencv2/core/version.hpp>
#if CV_MAJOR_VERSION == 2
#include <opencv2/highgui/highgui.hpp>
#elif CV_MAJOR_VERSION == 3
#include <opencv2/highgui.hpp>
#endif

#include <chrono>
#include <vector>
#include <string>

int main()
{
	cam::SigHandler sig_handle;//Instantiate this class first since the constructor blocks the signal of all future child threads

	//Class for managing cameras
	cam::Acquisition acq;

    acq.add_camera<cam::tau2>(25000812);
    acq.add_camera<cam::bluefox>(30400333);

    std::vector<cv::Mat> img_vec;//Vector to store the images

    //dynamic_cast<cam::BlueFoxParameters&>(acq.get_cam_params(0)).set_image_size(640,480);
    //dynamic_cast<cam::BlueFoxParameters&>(acq.get_cam_params(1)).set_image_size(640,480);

    //dynamic_cast<cam::BlueFoxParameters&>(acq.get_cam_params(0)).set_aec(true);
    //dynamic_cast<cam::BlueFoxParameters&>(acq.get_cam_params(1)).set_aec(true);

    //Loop variables
    bool looping = true;
    unsigned int count = 0;

    std::chrono::time_point<std::chrono::steady_clock> start, end;//Variable to measure framerate
    start = std::chrono::steady_clock::now();

    acq.start_acq();//Start the acquisition

//    for(;;)
    for(int signal_received, ret_sig = sig_handle.get_signal(signal_received);looping && ret_sig != 2 ;ret_sig = sig_handle.get_signal(signal_received))
    {
		if(ret_sig == 0 && (signal_received == SIGINT || signal_received == SIGTERM || signal_received == SIGABRT))
		{
			looping = false;//A signal is catched, and it is SIGINT
			std::cout << "SIGINT caught." << std::endl;
			break;
		}

		int ret_acq = acq.get_images(img_vec);
		if(!ret_acq)
		{
			++count;
			for(size_t n = 0; n<img_vec.size(); ++n)
			{
				std::string image_name_show = "Image_" + std::to_string(n+1);
				cv::imshow(image_name_show.c_str(), img_vec[n]);

				/*std::string image_name = "test_pic/" + std::to_string(n+1) + "_" + std::to_string(count) + ".png";
				cv::imwrite(image_name.c_str(), img_vec[n]);*/
			}
			cv::waitKey(1);
		}



		if(count && count%100 == 0)//Print framerate every 100 frames
		{
			end = std::chrono::steady_clock::now();
			std::cout << "Framerate : " << 100 * 1000.0 /  std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " fps" << std::endl;
			start = std::chrono::steady_clock::now();
		}
	}

    return 0;
}
