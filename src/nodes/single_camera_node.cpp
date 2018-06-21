#include "acquisition.hpp"

#include "util_signal.hpp"

#include "camera_mvbluefox.hpp"

#include <opencv2/core/version.hpp>
#if CV_MAJOR_VERSION == 2                                                                                                       
#include <opencv2/core/core.hpp> 
#include <opencv2/highgui/highgui.hpp>                                                            
#elif CV_MAJOR_VERSION == 3                                                                          
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>                                                                                                    
#endif

#include <ros/ros.h>
#include <cv_bridge/cv_bridge.h>
#include <image_transport/image_transport.h>
#include <sensor_msgs/image_encodings.h>

std::string get_encoding(int ocv_type)//Helper function to get the encoding type from opencv type
{
  if (ocv_type == CV_8UC1)
    return sensor_msgs::image_encodings::MONO8;
  else if (ocv_type == CV_16UC1)
    return sensor_msgs::image_encodings::MONO16;
  else if (ocv_type == CV_8UC3)
    return sensor_msgs::image_encodings::BGR8;
  else if (ocv_type == CV_8UC4)
    return sensor_msgs::image_encodings::BGRA8;
  else if (ocv_type == CV_16UC3)
    return sensor_msgs::image_encodings::BGR8;
  else if (ocv_type == CV_16UC4)
    return sensor_msgs::image_encodings::BGRA8;
  else return std::string();
}

int main(int argc, char * argv[])
{
	cam::SigHandler sig_handle;
	ros::init(argc, argv, "single_camera_node", ros::init_options::NoSigintHandler);
	
	ros::NodeHandle nh;
	image_transport::ImageTransport it(nh);
	
	ros::NodeHandle private_nh("~");
	std::string cam_topic;
    std::string cam_serial;
	
	if(private_nh.hasParam("cam_topic"))
	{

		private_nh.getParam("cam_topic", cam_topic);
	}
	else 
	{
		ROS_ERROR_STREAM("Error : Cannot find the \"cam_topic\" parameter for the node. Quitting.");
		return -1;
	}	

	if(private_nh.hasParam("cam_serial"))
	{

		private_nh.getParam("cam_serial", cam_serial);
	}
	else 
	{
		ROS_ERROR_STREAM("Error : Cannot find the \"cam_serial\" parameter for the node. Quitting.");
		return -1;
	}	
	
	image_transport::Publisher pub = it.advertise(cam_topic, 3);
	
	cam::Acquisition acq;
    acq.add_camera<cam::bluefox>(cam_serial);
    
    std::vector<cv::Mat> img_vec;//Vector to store the images
	
	sensor_msgs::ImagePtr msg;
	
	std::string img_encoding = get_encoding(dynamic_cast<cam::BlueFoxParameters&>(acq.get_cam_params(0)).get_pixel_format());
	
	acq.start_acq();
	
	for(;sig_handle.check_term_sig();)
	{			
		int ret_acq = acq.get_images(img_vec);
		if(ret_acq > 0)
		{
			msg = cv_bridge::CvImage(std_msgs::Header(), img_encoding, img_vec[0]).toImageMsg();
			pub.publish(msg);
		}
		
		ros::spinOnce();		
	}
	
	return 0;
}

