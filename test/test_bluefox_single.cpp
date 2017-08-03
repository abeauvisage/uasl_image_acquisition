#include "camera_mvbluefox.hpp"

#include "util_signal.hpp"

#if CV_MAJOR_VERSION == 2                                                                                                       
#include <opencv2/highgui/highgui.hpp>                                                            
#elif CV_MAJOR_VERSION == 3                                                                          
#include <opencv2/highgui.hpp>                                                                                                
#endif 

int main(int argc, char * argv[])
{
	cam::SigHandler sig_handle;//Instantiate this class first since the constructor blocks the signal of all future child threads
    cam::CamBlueFox cam1(26802713);
    int signal_received;
    const int max_iter = 100;
    int i = 0;
    std::chrono::time_point<std::chrono::steady_clock> start, end;
    
    cv::Mat img;
    
    bool looping = true;
    std::chrono::time_point<cam::clock_type> now = cam::clock_type::now();
    std::chrono::time_point<cam::clock_type> image_time;
    int image_nb;
    
    //Warm the cache and init the cam to have real running time
    cam1.start_acq();
    for(int ret_sig = sig_handle.get_signal(signal_received);looping && ret_sig != 2 && i<max_iter;ret_sig = sig_handle.get_signal(signal_received))
    {
			if(ret_sig == 0 && signal_received == SIGINT) looping = false;//A signal is catched, and it is SIGINT
			
			if(cam1.take_picture(img, &image_nb, &image_time)) true;//std::cout <<"Error during the acquisition" << std::endl;
			else 
			{
				++i;
			}		
		}
    
    //TEST ON MAIN THREAD
    std::cout << "Test on main thread started" << std::endl;
		
		//TEST 16BIT
    i = 0;
    cam1.set_image_type(CV_16U);
    cam1.start_acq();
    start = std::chrono::steady_clock::now();
    for(int ret_sig = sig_handle.get_signal(signal_received);looping && ret_sig != 2 && i<max_iter;ret_sig = sig_handle.get_signal(signal_received))
    {
			if(ret_sig == 0 && signal_received == SIGINT) looping = false;//A signal is catched, and it is SIGINT
			
			if(!cam1.take_picture(img, &image_nb, &image_time))
			{
				++i;
				cv::imwrite("temp/image_" + std::to_string(image_nb) + ".png", img);
			}		
		}
		end = std::chrono::steady_clock::now();
		std::cout << "TEST 16BIT MONO ended in " <<  std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms (" <<(double) max_iter * 1000.0 /  std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " fps)" << std::endl;
		
		//TEST 8BIT MONO
    i = 0;
    cam1.set_image_type(CV_8U);
    cam1.start_acq();
    start = std::chrono::steady_clock::now();
    for(int ret_sig = sig_handle.get_signal(signal_received);looping && ret_sig != 2 && i<max_iter;ret_sig = sig_handle.get_signal(signal_received))
    {
			if(ret_sig == 0 && signal_received == SIGINT) 
			{
				looping = false;//A signal is catched, and it is SIGINT
				std::cout << "SIGINT catched" << std::endl;
			}
			if(!cam1.take_picture(img, &image_nb, &image_time))
			{
				++i;
				cv::imwrite("temp/image_" + std::to_string(image_nb) + ".png", img);
			}		
		}
		end = std::chrono::steady_clock::now();
		std::cout << "TEST 8BIT MONO ended in " <<  std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms (" <<(double) max_iter * 1000.0 /  std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " fps)" << std::endl;
		
		//TEST 8BIT RGB
    i = 0;
    cam1.set_image_type(CV_8UC3);
    cam1.start_acq();
    start = std::chrono::steady_clock::now();
    for(int ret_sig = sig_handle.get_signal(signal_received);looping && ret_sig != 2 && i<max_iter;ret_sig = sig_handle.get_signal(signal_received))
    {
			if(ret_sig == 0 && signal_received == SIGINT) looping = false;//A signal is catched, and it is SIGINT
			
			if(!cam1.take_picture(img, &image_nb, &image_time))
			{
				++i;
				cv::imwrite("temp/image_" + std::to_string(image_nb) + ".png", img);
			}		
		}
		end = std::chrono::steady_clock::now();
		std::cout << "TEST 8BIT RGB ended in " <<  std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms (" <<(double) max_iter * 1000.0 /  std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " fps)" << std::endl;
		
		
		//END OF MAIN THREAD TEST
    std::cout << "Test on main thread ended" << std::endl;
    
    //TEST OF THE WORKER THREAD (WRITING)
    std::cout << "Test on worker thread started" << std::endl;
    int nb_begin, nb_end;
    
    //TEST 8 BIT MONO
    cam1.set_image_type(CV_8U);
    cam1.start_acq();
    std::this_thread::sleep_for(std::chrono::seconds(3));
    start = std::chrono::steady_clock::now();
    nb_begin = cam1.get_image_nb(); 
    cam1.set_write_on_disk(true, "temp", "image_wt_8bits_");
    std::this_thread::sleep_for(std::chrono::seconds(5));
    cam1.set_write_on_disk(false);
    nb_end = cam1.get_image_nb();
    end = std::chrono::steady_clock::now();
    std::cout << "TEST 8BIT mono (worker) ended in " <<  std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms for " << nb_end - nb_begin << "images (" <<(double) (nb_end - nb_begin) * 1000.0 /  std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " fps)" << std::endl;
    
     //TEST 16 BIT MONO
    cam1.set_image_type(CV_16U);
    cam1.start_acq();
    std::this_thread::sleep_for(std::chrono::seconds(3));
    start = std::chrono::steady_clock::now();
    nb_begin = cam1.get_image_nb(); 
    cam1.set_write_on_disk(true, "temp", "image_wt_16bits_");
    std::this_thread::sleep_for(std::chrono::seconds(5));
    cam1.set_write_on_disk(false);
    nb_end = cam1.get_image_nb();
    end = std::chrono::steady_clock::now();
    std::cout << "TEST 16BIT mono (worker) ended in " <<  std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms for " << nb_end - nb_begin << "images (" <<(double) (nb_end - nb_begin) * 1000.0 /  std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " fps)" << std::endl;
    
    //TEST 8 BIT RGB
    cam1.set_image_type(CV_8UC3);
    cam1.start_acq();
    std::this_thread::sleep_for(std::chrono::seconds(3));
    start = std::chrono::steady_clock::now();
    nb_begin = cam1.get_image_nb(); 
    cam1.set_write_on_disk(true, "temp", "image_wt_8bitsRGB_");
    std::this_thread::sleep_for(std::chrono::seconds(5));
    cam1.set_write_on_disk(false);
    nb_end = cam1.get_image_nb();
    end = std::chrono::steady_clock::now();
    std::cout << "TEST BIT RGB (worker) ended in " <<  std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms for " << nb_end - nb_begin << "images (" <<(double) (nb_end - nb_begin) * 1000.0 /  std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " fps)" << std::endl;
    
    //TEST OF THE WORKER THREAD (DISPLAY)
    
     //TEST 8 BIT MONO
    cam1.set_image_type(CV_8U);
    cam1.start_acq();
    std::this_thread::sleep_for(std::chrono::seconds(3));
    start = std::chrono::steady_clock::now();
    nb_begin = cam1.get_image_nb(); 
    cam1.set_image_display(true, "Image");
    std::this_thread::sleep_for(std::chrono::seconds(5));
    cam1.set_image_display(false);
    nb_end = cam1.get_image_nb();
    end = std::chrono::steady_clock::now();
    std::cout << "TEST 8BIT mono (worker display) ended in " <<  std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms for " << nb_end - nb_begin << "images (" <<(double) (nb_end - nb_begin) * 1000.0 /  std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " fps)" << std::endl;
    
     //TEST 16 BIT MONO
    cam1.set_image_type(CV_16U);
    cam1.start_acq();
    std::this_thread::sleep_for(std::chrono::seconds(3));
    start = std::chrono::steady_clock::now();
    nb_begin = cam1.get_image_nb(); 
    cam1.set_image_display(true, "Image");
    std::this_thread::sleep_for(std::chrono::seconds(5));
    cam1.set_image_display(false);
    nb_end = cam1.get_image_nb();
    end = std::chrono::steady_clock::now();
    std::cout << "TEST 16BIT mono (worker display) ended in " <<  std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms for " << nb_end - nb_begin << "images (" <<(double) (nb_end - nb_begin) * 1000.0 /  std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " fps)" << std::endl;
    
    //TEST 8 BIT RGB
    cam1.set_image_type(CV_8UC3);
    cam1.start_acq();
    std::this_thread::sleep_for(std::chrono::seconds(3));
    start = std::chrono::steady_clock::now();
    nb_begin = cam1.get_image_nb(); 
    cam1.set_image_display(true, "Image");
    std::this_thread::sleep_for(std::chrono::seconds(5));
    cam1.set_image_display(false);
    nb_end = cam1.get_image_nb();
    end = std::chrono::steady_clock::now();
    std::cout << "TEST BIT RGB (worker display) ended in " <<  std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms for " << nb_end - nb_begin << "images (" <<(double) (nb_end - nb_begin) * 1000.0 /  std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " fps)" << std::endl;
    
    
    return 0;
}
