#include "acquisition.hpp"

#include "util_signal.hpp"

#include "camera_mvbluefox.hpp"

#include <opencv2/core/version.hpp>
#if CV_MAJOR_VERSION == 2
#include <opencv2/highgui/highgui.hpp>
#elif CV_MAJOR_VERSION == 3
#include <opencv2/highgui.hpp>
#endif


int main()
{
	cam::SigHandler sig_handle;//Instantiate this class first since the constructor blocks the signal of all future child threads

	//Class for managing cameras
	cam::Acquisition acq;

	//Add a camera
    acq.add_camera<cam::bluefox>(25000812);

    std::vector<cv::Mat> img_vec;//Vector to store the images

    int signal_received;
    const int max_iter = 100;
    int i = 0;
    std::chrono::time_point<std::chrono::steady_clock> start, end;

    bool looping = true;
    std::chrono::time_point<cam::clock_type> image_time;

    //Warm the cache and init the cam to have real running time
    acq.start_acq();

    for(int ret_sig = sig_handle.get_signal(signal_received);looping && ret_sig != 2 && i<max_iter;ret_sig = sig_handle.get_signal(signal_received))
    {
			if(ret_sig == 0 && signal_received == SIGINT) looping = false;//A signal is catched, and it is SIGINT

			int ret_acq = acq.get_images(img_vec);

			if(!ret_acq)
			{
				++i;
			}
		}


	//TEST 16BIT MONO
    i = 0;
    dynamic_cast<cam::BlueFoxParameters&>(acq.get_cam_params(0)).set_image_type(CV_16U);
    acq.start_acq();
    start = std::chrono::steady_clock::now();
    for(int ret_sig = sig_handle.get_signal(signal_received);looping && ret_sig != 2 && i<max_iter;ret_sig = sig_handle.get_signal(signal_received))
    {
			if(ret_sig == 0 && signal_received == SIGINT) looping = false;//A signal is catched, and it is SIGINT

			int ret_acq = acq.get_images(img_vec);

			if(!ret_acq)
			{
				++i;
				cv::imwrite("temp/image16_" + std::to_string(i) + ".png", img_vec[0]);
			}
		}
		end = std::chrono::steady_clock::now();
		std::cout << "TEST 16BIT MONO ended in " <<  std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms (" <<(double) max_iter * 1000.0 /  std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " fps)" << std::endl;

	//TEST 8BIT MONO
    i = 0;
    dynamic_cast<cam::BlueFoxParameters&>(acq.get_cam_params(0)).set_image_type(CV_8U);
    acq.start_acq();
    start = std::chrono::steady_clock::now();
    for(int ret_sig = sig_handle.get_signal(signal_received);looping && ret_sig != 2 && i<max_iter;ret_sig = sig_handle.get_signal(signal_received))
    {
			if(ret_sig == 0 && signal_received == SIGINT) looping = false;//A signal is catched, and it is SIGINT

			int ret_acq = acq.get_images(img_vec);

			if(!ret_acq)
			{
				++i;
				cv::imwrite("temp/image8_" + std::to_string(i) + ".png", img_vec[0]);
			}
		}
		end = std::chrono::steady_clock::now();
		std::cout << "TEST 8BIT MONO ended in " <<  std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms (" <<(double) max_iter * 1000.0 /  std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " fps)" << std::endl;

	//TEST 8BIT RGB
    i = 0;
    dynamic_cast<cam::BlueFoxParameters&>(acq.get_cam_params(0)).set_image_type(CV_8UC3);
    acq.start_acq();
    start = std::chrono::steady_clock::now();
    for(int ret_sig = sig_handle.get_signal(signal_received);looping && ret_sig != 2 && i<max_iter;ret_sig = sig_handle.get_signal(signal_received))
    {
			if(ret_sig == 0 && signal_received == SIGINT) looping = false;//A signal is catched, and it is SIGINT

			int ret_acq = acq.get_images(img_vec);

			if(!ret_acq)
			{
				++i;
				cv::imwrite("temp/image8UC3_" + std::to_string(i) + ".png", img_vec[0]);
			}
		}
		end = std::chrono::steady_clock::now();
		std::cout << "TEST 8BIT RGB ended in " <<  std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms (" <<(double) max_iter * 1000.0 /  std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " fps)" << std::endl;

    return 0;
}
