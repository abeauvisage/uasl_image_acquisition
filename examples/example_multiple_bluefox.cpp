#include <iostream>

#include <opencv2/core/version.hpp>
#if CV_MAJOR_VERSION == 2
#include <opencv2/highgui/highgui.hpp>
#elif CV_MAJOR_VERSION == 3
#include <opencv2/highgui.hpp>
#endif 

#include "util_signal.hpp" //For the signal handling
#include "multiple_camera_sync.hpp"
#include "camera_mvbluefox.hpp"


int main(int argc, char * argv[])
{
    //This example illustrate the creation of several cameras to run simultaneously

    //Initialise the signal handling (always initialize this class first)
    cam::SigHandler sig_handle;

    //Initialise two cameras
    cam::CamBlueFox cam1(26802713);
    cam::CamBlueFox cam2(32902184);
    

    cam2.set_image_size(1024,640);
    
    
    //Initialise the synchronization class
    cam::CamSync cam_sync;

    //Add the cameras to the sync system
    cam_sync.add_camera(cam1);
    cam_sync.add_camera(cam2);
    
    //Option 1 : perform the writing/display on the worker threads
    //cam_sync.set_write_on_disk(true, "temp", "image_");
    //cam_sync.set_image_display(true, "image");
    
    //cam_sync.set_image_display(true, "Image");

    //Start the acquisition
    cam_sync.start_acq();
		
		
		
  //Loop variables
	bool looping = true;
	int signal_received;
  
  std::vector<cv::Mat> mat_vec = {cv::Mat(), cv::Mat()};
  std::chrono::time_point<std::chrono::steady_clock> start, end;
  int i = 0;

  start = std::chrono::steady_clock::now();

	for(int ret_sig = -1;looping && ret_sig != 2;ret_sig = sig_handle.get_signal(signal_received))
	{
        //Check if a signal is received
		if(ret_sig == 0 && signal_received == SIGINT) 
		{
			looping = false;//A signal is catched, and it is SIGINT
			std::cout << "SIGINT caught, exiting." << std::endl;
		}			
    //Get the pictures
    if(cam_sync.take_pictures(mat_vec) == 0)
    {
      //Option 2 : get the images manually, then do what we want with them
      ++i;
			if(i == 100)
			{
				end = std::chrono::steady_clock::now();
				std::cout << "Framerate : " <<(double) 100 * 1000.0 /  std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " fps" << std::endl;
				start = std::chrono::steady_clock::now();
				i = 0;
			}
        //cv::imshow("Image 1", mat_vec[0]);
        //cv::imshow("Image 2", mat_vec[1]);
        //cv::waitKey(1);
    }
    else
    {
    	std::cout << "Error during the acquisition" << std::endl;
   	}
	}
    return 0;
}
