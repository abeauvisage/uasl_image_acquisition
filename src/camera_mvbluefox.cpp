#include "camera_mvbluefox.hpp"

#if CV_MAJOR_VERSION == 2
#include <opencv2/highgui/highgui.hpp>
#elif CV_MAJOR_VERSION == 3
#include <opencv2/highgui.hpp>
#endif


namespace cam
{
CamBlueFox::CamBlueFox():
    isOpened(false),
    started(false),
    image_nb(0),
    img_keep_rate(1),
    image_has_been_returned(false),
    write_on_disk(false),
    image_prefix("image_"),
    show_on_screen(false)
{    
    init();
}

CamBlueFox::CamBlueFox(int camId):
    isOpened(false),
    started(false),
    image_nb(0),
    img_keep_rate(1),
    image_has_been_returned(false),
    write_on_disk(false),
    image_prefix("image_"),
    show_on_screen(false)
{
    init(camId);
}

CamBlueFox::~CamBlueFox()
{
    stop_acq();
    std::cout << "Camera (Serial " << p_dev->serial.read() << ") has been destructed successfully." << std::endl;
}

int CamBlueFox::start_acq()
//Start the acquisition 
//Return -1 if failure, 0 otherwise
{
    if(!isOpened)
    {
       std::cerr << "Camera is not opened, cannot start acquisition" << std::endl;
       return -1;
    }

    if(started.load()) return 0;//Already started

    started = true;

    //Fill the request queue
    TDMR_ERROR result = DMR_NO_ERROR;
    while( ( result = static_cast<TDMR_ERROR>(p_fi->imageRequestSingle()) ) == DMR_NO_ERROR) {};
    if( result != DEV_NO_FREE_REQUEST_AVAILABLE )
    {
         std::cerr << "'FunctionInterface.imageRequestSingle' returned with an unexpected result: " << result  << "(" << ImpactAcquireException::getErrorCodeAsString( result ) << ")" << std::endl;
    }


    thd = std::thread(&CamBlueFox::thread_func, this);
}

int CamBlueFox::stop_acq()
//Stop the acquisition and wait for the thread to stop
{
    started = false;

    if(thd.joinable()) thd.join();

    return 0;
}

int CamBlueFox::take_picture(cv::Mat& image, int * image_nb_, std::chrono::time_point<clock_type> * time_pt_)
{
    //Copy the latest picture taken by the camera into image. If the latest picture has not changed, wait until a new one is taken
    //Return 0 if success, -1 if the image is empty
    std::unique_lock<std::mutex> mlock(mtx_image_current);
    image_has_changed.wait(mlock,[this]{return !image_has_been_returned;});
    //End of waiting. the image has not been returned
    if(image_current.empty()) return -1;
    image_current.copyTo(image);
    if(image_nb_ != nullptr) *image_nb_ = image_nb;
    if(time_pt_ != nullptr) *time_pt_ = image_current_time;
    image_has_been_returned = true;
    return 0;
}

void CamBlueFox::set_write_on_disk(bool value, const std::string& path_to_folder_ , const std::string& image_prefix_)
{
    std::lock_guard<std::mutex> mlock(mtx_writing_var);
    if(!path_to_folder_.empty()) path_to_folder = path_to_folder_;
    if(!image_prefix_.empty()) image_prefix = image_prefix_;
    write_on_disk = value;
}

void CamBlueFox::set_image_display(bool value, const std::string& window_name_)
{
    std::lock_guard<std::mutex> mlock(mtx_image_display);
    show_on_screen = value;
    if(value)
    {
        if(!window_name_.empty()) 
        {
            cv::destroyWindow(window_name);//If the name change, destroy former window
            window_name = window_name_;
        }

        cv::namedWindow(window_name);
    }
    else
    {
        cv::destroyWindow(window_name);
    }
}

void CamBlueFox::set_img_keep_rate(unsigned int value)
{
    if(value > 0)
    {
        img_keep_rate = value;
    }
    else
    {
        std::cerr << "The slowdown in image speed cannot be null." << std::endl;
    }
}

void CamBlueFox::set_image_size(int width, int height)
{
    //Set the image size, if the value is negative, keep former value
    if(!isOpened)
    {
        std::cerr << "Error : Camera not opened." << std::endl;
        return;
    }
    stop_acq();
    mvIMPACT::acquire::CameraSettingsBlueFOX cam_settings(p_dev);
    if(width > 0) cam_settings.aoiWidth.write(width);
    if(height > 0) cam_settings.aoiHeight.write(height);
}

void CamBlueFox::set_agc(bool value)
{
    if(!isOpened)
    {
        std::cerr << "Error : Camera not opened." << std::endl;
        return;
    }
    stop_acq();
    mvIMPACT::acquire::CameraSettingsBlueFOX cam_settings(p_dev);
    mvIMPACT::acquire::TAutoGainControl agc_val = value ? agcOn : agcOff;
    
    if(cam_settings.autoGainControl.isValid()) cam_settings.autoGainControl.write(agc_val);
}

void CamBlueFox::set_aec(bool value)
{
    if(!isOpened)
    {
        std::cerr << "Error : Camera not opened." << std::endl;
        return;
    }
    stop_acq();
    mvIMPACT::acquire::CameraSettingsBlueFOX cam_settings(p_dev);
    mvIMPACT::acquire::TAutoExposureControl aec_val = value ? aecOn : aecOff;
    
    if(cam_settings.autoExposeControl.isValid()) cam_settings.autoExposeControl.write(aec_val);
}

void CamBlueFox::set_image_type(int ocv_color_code)
{
    //Set both the acquisition type and the transfer type to the same value
    //Supported : CV_8U, CV_16U, CV_8UC3 (see OpenCV documentation for details)
    if(!isOpened)
    {
        std::cerr << "Error : Camera not opened." << std::endl;
        return;
    }
    
    stop_acq();//If the thread is not stopped (i.e. if this line is removed), the following is non thread safe and WILL
    //make the program crash.

    mvIMPACT::acquire::TImageDestinationPixelFormat pixel_format_destination;
    //From what I understand, the ImageBuffer pixelFormat is related to the format for sending the image, however, the
    //output format is the ImageDestination

    switch(ocv_color_code)
    {
        case CV_8U:
            pixel_format_destination = idpfMono8;
            pixel_format = ocv_color_code;
            break;
        case CV_16U:
            pixel_format_destination = idpfMono16;
            pixel_format = ocv_color_code;
            break;
        case CV_8UC3:
            pixel_format_destination = idpfRGB888Packed;
            pixel_format = ocv_color_code;
            break;
        default:
            std::cerr << "Error : Unknown pixel format." << std::endl;
            return;
    }

    mvIMPACT::acquire::ImageDestination image_destination_settings(p_dev);

    image_destination_settings.pixelFormat.write(pixel_format_destination);
}

unsigned int CamBlueFox::get_image_nb() const 
{
    std::lock_guard<std::mutex> mlock(mtx_image_current);
    return image_nb;
}

void CamBlueFox::init(int camId)
{
    //Update the device list
    dev_mgr.changedCount();

    if(!dev_mgr.deviceCount())//Check that there is at least one device connected
    {
        std::cerr << "Bluefox : No device detected." <<std::endl;
        return;
    }
    if(camId == -1)
    {
        p_dev = dev_mgr[0];
    }
    else
    {
        p_dev = dev_mgr.getDeviceBySerial(std::to_string(camId));
    }

    try
    {
        p_dev->open();
        isOpened = true;
    }
    catch(mvIMPACT::acquire::ImpactAcquireException& e)
    {
        std::cerr << "An error occured while opening the device (error code: " << e.getErrorString() << ")" << std::endl;
    }

    if(!isOpened || !p_dev) return;
    
    try
    {
        p_fi = std::unique_ptr<mvIMPACT::acquire::FunctionInterface>(new
        mvIMPACT::acquire::FunctionInterface(p_dev));//Replace by make_unique in C++14
    }
    catch(std::bad_alloc& e)
    {
        std::cerr << "Fail to allocate FunctionInterface, closing." <<std::endl;
        p_dev->close();
        isOpened = false;
    }

    if(isOpened) 
    {
        //Initialize the settings
        //Settings for the acquisition
        mvIMPACT::acquire::CameraSettingsBlueFOX cam_settings(p_dev);
        cam_settings.aoiWidth.write(width_d);
        cam_settings.aoiHeight.write(height_d);
        if(cam_settings.autoGainControl.isValid()) cam_settings.autoGainControl.write(agc_d);
        if(cam_settings.autoExposeControl.isValid()) cam_settings.autoExposeControl.write(aec_d);
        //Settings for the output
        mvIMPACT::acquire::ImageDestination image_destination_settings(p_dev);
        image_destination_settings.pixelFormat.write(pixel_format_output_d);//Set the pixel format of the output image
        pixel_format = pixel_fromat_d;//Register the pixel format
        std::cout << "Camera (Serial " << p_dev->serial.read() << ") has been opened successfully." << std::endl;
        if(!clock_type::is_steady)
        {
            std::cerr << "Warning : non steady clock type, timer might go back in time." << std::endl;
        }
    }
}

void CamBlueFox::thread_func()
{
    manuallyStartAcquisitionIfNeeded(p_dev, *p_fi);
    mvIMPACT::acquire::Request * pRequest = nullptr;
    mvIMPACT::acquire::Request * pPreviousRequest = nullptr;
    const unsigned timeoutMs = 500;
    unsigned real_img_count = 0;
    
    while(started.load())
    {
        //Wait for the results from the default capture queue
        int requestNr = p_fi->imageRequestWaitFor(timeoutMs);

        pRequest = p_fi->isRequestNrValid(requestNr) ? p_fi->getRequest(requestNr) : nullptr;

        if(pRequest != nullptr)
        {
            if(pRequest->isOK())
            {
                //Do image processing
                if(real_img_count%img_keep_rate.load() == 0)//Keep one image out of img_keep_rate
                {
                    mvIMPACT::acquire::ImageBuffer * p_ib = pRequest->getImageBufferDesc().getBuffer();
                    {//Mutex scope
                        std::lock_guard<std::mutex> mlock(mtx_image_current);//Mutex to lock the image
                        import_image(p_ib->iWidth, p_ib->iHeight, p_ib->vpData);
                        //We have copied the image data a this point
                        ++image_nb;
                        image_current_time = clock_type::now();
                        image_has_been_returned = false;
                    }// /Mutex scope
                    image_has_changed.notify_one();//Notify a waiting thread (usually the main)
                    write_image_disk();
                    show_image();
                }
                ++real_img_count;
            }
            else
            {
                std::cerr << "Error during image request : " << pRequest->requestResult.readS() << std::endl;
            }

            //This image has been used thus the buffer is no longer needed
            if(pPreviousRequest != nullptr)
            {
                //This image has been displayed thus the buffer is no longer needed
                pPreviousRequest->unlock();
            }
            pPreviousRequest = pRequest;
            //Send a new image request to the capture queue
            p_fi->imageRequestSingle();
        }
        else
        {
            std::cerr << "imageRequestWaitFor failed : " << requestNr << std::endl;
        }
    }
    manuallyStopAcquisitionIfNeeded(p_dev, *p_fi);

    if(pRequest)//Unlock the last potentially locked request
    {
        pRequest->unlock();
    }
    if(p_fi->imageRequestReset(0,0) != mvIMPACT::acquire::DMR_NO_ERROR)//Clear reset queue
    {
        std::cerr << "Error while resetting FunctionInterface." <<std::endl;
    }
    //Extract and unlock all requests
    int requestNr = mvIMPACT::acquire::INVALID_ID;
    while((requestNr = p_fi->imageRequestWaitFor(0)) >= 0)
    {
        pRequest = p_fi->getRequest(requestNr);
        pRequest->unlock();
    }
}


void CamBlueFox::write_image_disk()
{
    if(write_on_disk.load())
    {
        std::unique_lock<std::mutex> mlock(mtx_writing_var);

        std::stringstream nb;
        nb << std::setfill('0') << std::setw(5) << image_nb;
        std::string img_name = path_to_folder + '/' + image_prefix + nb.str() + ".png"; 
        mlock.unlock();

        std::lock_guard<std::mutex> mlock_img(mtx_image_current);
        cv::imwrite(img_name, image_current); 
    }
}

void CamBlueFox::show_image()
{
    if(show_on_screen.load())
    {
        std::lock_guard<std::mutex> mlock(mtx_image_display);
        std::lock_guard<std::mutex> mlock_img(mtx_image_current);
        cv::imshow(window_name, image_current);
        cv::waitKey(1);
    }
}


} //namespace cam
