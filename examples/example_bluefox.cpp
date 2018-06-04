#include <iostream>

#include <opencv2/core/version.hpp>
#if CV_MAJOR_VERSION == 2
#include <opencv2/highgui/highgui.hpp>
#elif CV_MAJOR_VERSION == 3
#include <opencv2/highgui.hpp>
#endif 

#include "acquisition.hpp" //The main class
#include "camera_mvbluefox.hpp" //You need the header corresponding to the specific camera you are using
#include "util_signal.hpp" //For the signal handling




int main()
{
    //This example illustrate the creation of one or several cameras to run.
    //Note that if you use several cameras, you need the trigger. The onboard code for the trigger can be found in the trigger folder.

    //Initialise the signal handling (always initialize this class first)
    cam::SigHandler sig_handle;
    
    //The acquisition class manages all the cameras
	cam::Acquisition acq;

    //Initialise one camera
    acq.add_camera<cam::bluefox>(29900221);//You can then add a second, third, etc, by calling the function add_camera again
    
    std::vector<cv::Mat> img_vec;//Vector to store the images
    
    //You can modify options such as the size of the image, the type, etc ...
    dynamic_cast<cam::BlueFoxParameters&>(acq.get_cam_params(0)).set_image_type(CV_8UC3);//Tell the first camera to record in RGB
	dynamic_cast<cam::BlueFoxParameters&>(acq.get_cam_params(0)).set_image_size(720,480);//Modify the image size in first camera
    
    
    //Start the acquisition
    acq.start_acq();
		
  	//Loop variables to manage the interruption of the program
	bool looping = true;
	int signal_received;

	//ret_sig is the signal detected by the OS (if any). This has no direct relevance to the camera itself, 
	//but is necessary to allow the program to stop : at each iteration, we check if a signal (here, SIGINT)
	//was received.
	for(int ret_sig = -1;looping && ret_sig != 2;ret_sig = sig_handle.get_signal(signal_received))
	{
		if(ret_sig == 0 && signal_received == SIGINT) 
		{
			looping = false;//A signal is catched, and it is SIGINT
			std::cout << "SIGINT caught, exiting." << std::endl;
		}			
		//Get the pictures
		int ret_acq = acq.get_images(img_vec);
		if(ret_acq == 0)
		{
			//If success, img_vec is now filled with the returned images.
			for(unsigned i = 0;i<img_vec.size();++i)
			{
				//Do what we want with img_vec[i], here for instance, show it
				cv::imshow("Image " + std::to_string(i), img_vec[i]);//Show the image
			}
			cv::waitKey(1);//Update the windows so that the images appear 	
		}
		else
		{
			std::cout << "Error during the acquisition" << std::endl;
	   	}
	}
    return 0;
}
