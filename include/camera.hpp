#ifndef IMAGE_ACQUISITION_CAMERA_HPP
#define IMAGE_ACQUISITION_CAMERA_HPP

#include "opencv2/core/version.hpp"
#if CV_MAJOR_VERSION == 2
#include <opencv2/core/core.hpp>
#elif CV_MAJOR_VERSION == 3
#include <opencv2/core.hpp>
#endif

namespace cam
{

class Camera
{
	public:
    virtual ~Camera() {}
	virtual int take_picture(cv::Mat& image) = 0;
};

} //namespace cam

#endif
