#include <iostream>
#include <iomanip>
#include <fstream>

#include <opencv2/core/version.hpp>
#if CV_MAJOR_VERSION == 2
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#elif CV_MAJOR_VERSION == 3
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#endif

#include "acquisition.hpp" //The main class
#include "camera_tau2.hpp" //You need the header corresponding to the specific camera you are using
#include "camera_mvbluefox.hpp"
#include "util_signal.hpp" //For the signal handling


struct{

std::string dir;
bool save=false;

}params;

int main(int argc, char** argv)
{
    char c;
    while ((c = getopt(argc, argv, "sd:")) != -1)
	{
		switch (c)
		{
        case 'd':
            params.dir = optarg;break;
        case 's':
            params.save=true;break;
		default:
			std::cout << "Invalid option: -" << c << std::endl;
			return 1;
		}
	}

	if(params.save && params.dir.empty()){
		std::cerr << "please provide directory (using option -d \"pathToDir\")" << std::endl;
		exit(-1);
	}

	//saving image timestamps
	std::ofstream fimage;
	if(params.save)
        fimage.open(params.dir+"/image_data.csv");

    cam::SigHandler sig_handle;

	cam::Acquisition acq;
    acq.add_camera<cam::bluefox>("25000812");
    acq.add_camera<cam::tau2>("FT2HKAW5");

    std::vector<cv::Mat> img_vec;

    //parameters
    dynamic_cast<cam::Tau2Parameters&>(acq.get_cam_params(1)).set_pixel_format(CV_16U);
    dynamic_cast<cam::Tau2Parameters&>(acq.get_cam_params(1)).set_image_roi(0,16,640,480);
    dynamic_cast<cam::BlueFoxParameters&>(acq.get_cam_params(0)).set_image_roi(56,0,640,480);
    dynamic_cast<cam::BlueFoxParameters&>(acq.get_cam_params(0)).set_exposure_time(150);

    //start acquisition
	acq.set_trigger_port_name("/dev/ttyTRIGGER");
    acq.start_acq();

	unsigned int img_nb = 0;
	while(sig_handle.check_term_sig() && acq.is_running())
	{
		int64_t ret_acq = acq.get_images(img_vec);
		if(ret_acq > 0)
		{
			std::string img_nb_s = std::to_string(img_nb);
			for(unsigned i = 0;i<img_vec.size();++i)
			{
				cv::Mat img = img_vec[i].clone();
				if(i==1){ // equalisation for infrared image
                    double m = 20.0/64.0;
                    double mean_img = cv::mean(img)[0];
                    img = img * m + (127-mean_img* m);
                    img.convertTo(img, CV_8U);
				}
				if(params.save)
                    cv::imwrite(params.dir+"/cam"+std::to_string(i)+"_image"+std::string(5-img_nb_s.length(),'0')+img_nb_s+".png", img);
                else
                    cv::imshow("cam "+std::to_string(i),img);
			}
            if(fimage.is_open())
                    fimage << img_nb_s <<  "," << ret_acq << std::endl;

			cv::waitKey(1);
			img_nb++;
		}
		else
            std::cerr << "[Error] could not retreive images" << std::endl;
	}
    return 0;
}
