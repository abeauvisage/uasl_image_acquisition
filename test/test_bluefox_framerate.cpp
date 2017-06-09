#include "camera_mvbluefox.hpp"

#include "util_signal.hpp"

#include <opencv2/core/version.hpp>
#if CV_MAJOR_VERSION == 2                                                                                                       
#include <opencv2/highgui/highgui.hpp>                                                            
#elif CV_MAJOR_VERSION == 3                                                                          
#include <opencv2/highgui.hpp>                                                                                                
#endif 

int main(int argc, char * argv[])
{
	cam::SigHandler sig_handle;//Instantiate this class first since the constructor blocks the signal of all future child threads
    cam::CamBlueFox cam1;
    int signal_received;
    
    std::chrono::time_point<std::chrono::steady_clock> start, end;
    
    cv::Mat img;
    
    bool looping = true;
    std::chrono::time_point<cam::clock_type> now = cam::clock_type::now();
    int image_nb;
		
    unsigned int i = 0;
    cam1.set_image_type(CV_8U);
    cam1.start_acq();
    start = std::chrono::steady_clock::now();
    for(int ret_sig = sig_handle.get_signal(signal_received);looping && ret_sig != 2 ;ret_sig = sig_handle.get_signal(signal_received))
    {
			if(ret_sig == 0 && signal_received == SIGINT) looping = false;//A signal is catched, and it is SIGINT
			
			if(!cam1.take_picture(img))
			{
				++i;
				cv::imshow("Image", img);
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
