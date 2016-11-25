#include "camera_mvbluefox.hpp"

#include "util_signal.hpp" //For the signal handling

#if CV_MAJOR_VERSION == 2                                                                                                       
#include <opencv2/highgui/highgui.hpp>                                                            
#elif CV_MAJOR_VERSION == 3                                                                          
#include <opencv2/highgui.hpp>                                                                                                
#endif 

#include <iostream>

int main(int argc, char * argv[])
{
		cam::SigHandler sig_handle;//Instantiate this class first since the constructor blocks the signal of all future child threads
    cam::CamBlueFox cam1(26802713);//If constructor is empty, open the first camera found    

    
    std::chrono::time_point<cam::clock_type> now = cam::clock_type::now();
    
    /*********************************************/
    //Example 1 : an algorithm uses the images
    
    cam1.set_image_type(CV_8UC3);//Get RGB images (optional, default : CV_8U)
    cam1.set_image_size(720,480);//Set image width and height (optional, default : 640x480)
    //Other options : set AGC and AEC
    
    cam1.start_acq();//Start acquisition
    
    cv::Mat img;
    int image_nb;
    std::chrono::time_point<cam::clock_type> image_time;
    int signal_received;
    
    bool looping = true;//Loop while sigint is not sent
    for(int ret_sig = -1;looping && ret_sig != 2;ret_sig = sig_handle.get_signal(signal_received))
    {
			if(ret_sig == 0 && signal_received == SIGINT) 
			{
				looping = false;//A signal is catched, and it is SIGINT
				std::cout << "SIGINT caught, exiting first example" << std::endl;
			}
			
			if(!cam1.take_picture(img, &image_nb, &image_time)) //The pointers are optional, call cam1.take_picture(img) to have just the image
			{
				//Do something with the image
				//img contains the image
				//image_nb is the internal number corresponding to this image
				//image_time contains the time the image was taken
				
				//Please note that if your processing is too long, you can skip some images. You can detect that with image_nb :
				//all the images successfully acquired increment image_nb by 1, thus if the gap is more than 1, images have been skipped
				//(you always get the latest image taken with this method, and if there is no new image, the method waits until there is one available)
				
				//Example of use :
				std::cout <<  "Image " << image_nb << " acquired " << std::chrono::duration_cast<std::chrono::milliseconds>(image_time - now).count() << " ms after beginning of the program." << std::endl;
			}		
		}
		
		/*********************************************/
    
   
    
    /*********************************************/
    //Example 2 : simply record and/or display the images from the acquiring thread (you are then sure to get all acquired images)
    cam1.set_image_type(CV_8U);//Changing the type, size, or agc and aec parameters stops the acquisition, we thus have to restart it
    cam1.start_acq();
    looping = true;
    
    cam1.set_image_display(true, "Image");//Activate the display of the images (this can be changed during the acquisition, pass false boolean to stop it)
    cam1.set_write_on_disk(true, "path_to_image_folder", "image_prefix_");//Activate the writing of the images on the disk (this can be changed during the acquisition, pass false boolean to stop it). The folder has to exist at the moment, TODO:create the folder if it does not exist
    
    cam1.set_img_keep_rate(3);//Keep 1 image out of 3, useful if we want to lower the framerate (default value is 1, i.e. we keep all the images). This can be changed during the acquisition
    
    for(int ret_sig = -1;looping && ret_sig != 2;ret_sig = sig_handle.get_signal(signal_received))
    {
			if(ret_sig == 0 && signal_received == SIGINT) 
			{
				looping = false;//A signal is catched, and it is SIGINT
				std::cout << "SIGINT caught, exiting second example" << std::endl;
			}			
		}
    
   
    
    //The acquisition is stop automatically when the camera object is destroyed, it can be stopped manually by calling the stop_acq() method
    return 0;
}
