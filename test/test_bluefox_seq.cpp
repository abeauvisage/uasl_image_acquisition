#include "acquisition.hpp"

#include "util_signal.hpp"

#include "camera_mvbluefox_seq.hpp"

#include <opencv2/core/version.hpp>
#if CV_MAJOR_VERSION == 2                                                                                                       
#include <opencv2/highgui/highgui.hpp>                                                            
#elif CV_MAJOR_VERSION == 3                                                                          
#include <opencv2/highgui.hpp>                                                                                                
#endif 

#include <chrono>
#include <vector>
#include <string>

int main(int argc, char * argv[])
{
	cam::SigHandler sig_handle;//Instantiate this class first since the constructor blocks the signal of all future child threads
    
    
    cam::Acquisition acq;
    acq.add_camera(cam::bluefox, 30400333);
    acq.add_camera(cam::bluefox, 30400337);
    int signal_received;
    
    dynamic_cast<BlueFoxParameters&>(acq.get_params(0)).set_aec(false);
    dynamic_cast<BlueFoxParameters&>(acq.get_params(1)).set_aec(false);
    
    std::chrono::time_point<std::chrono::steady_clock> start, end;
   
    
    std::vector<cv::Mat> img_vec;
    
    bool looping = true;
    std::chrono::time_point<std::chrono::steady_clock> now = std::chrono::steady_clock::now();
    int image_nb;
		
    unsigned int i = 0;
    unsigned int count = 0;
	
	acq.start_acq();
    start = std::chrono::steady_clock::now();
    for(int ret_sig = sig_handle.get_signal(signal_received);looping && ret_sig != 2 ;ret_sig = sig_handle.get_signal(signal_received))
    {
		if(ret_sig == 0 && signal_received == SIGINT) looping = false;//A signal is catched, and it is SIGINT
		
		if(!acq.get_images(img_vec))
		{
			++i;
			++count;
			for(int n = 0; n<img_vec.size(); ++n)
			{
				std::string image_name = "Image" + std::to_string(n+1);
				cv::imshow(image_name.c_str(), img_vec[n]);
				
				/*std::string image_name = "test_pic/" + std::to_string(n+1) + "_" + std::to_string(count) + ".png";
				cv::imwrite(image_name.c_str(), img_vec[n]);*/
			}
			cv::waitKey(1);
		}
		
		if(i == 100)
		{
			end = std::chrono::steady_clock::now();
			std::cout << "Framerate : " <<(double) 100 * 1000.0 /  std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " fps" << std::endl;
			start = std::chrono::steady_clock::now();
			i = 0;
		}
	}    
    
    return 0;
}
