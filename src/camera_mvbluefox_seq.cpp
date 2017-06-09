#include "camera_mvbluefox_seq.hpp"

#include "acquisition.hpp"

#include <iostream>

namespace cam
{
//BlueFoxParameters : Public functions
void BlueFoxParameters::set_image_size(int width, int height)
{
	Acquisition_lock lock(package);//If the function modifies the parameters, always call the lock at the very beginning
	//Set the image size, if the value is negative, keep former value
	if(!check_cam() || !lock.is_valid()) return;
	
    mvIMPACT::acquire::CameraSettingsBlueFOX cam_settings(p_dev);
    if(width > 0) cam_settings.aoiWidth.write(width);
    if(height > 0) cam_settings.aoiHeight.write(height);	
}

void BlueFoxParameters::set_agc(bool value)
{
	Acquisition_lock lock(package);//If the function modifies the parameters, always call the lock at the very beginning
	if(!check_cam() || !lock.is_valid()) return;
	
    mvIMPACT::acquire::CameraSettingsBlueFOX cam_settings(p_dev);
    mvIMPACT::acquire::TAutoGainControl agc_val = value ? agcOn : agcOff;
    
    if(cam_settings.autoGainControl.isValid()) cam_settings.autoGainControl.write(agc_val);
    else
    {
    	std::cerr << "Warning : attempt to modify Automatic Gain Control, but the feature is not available." << std::endl;
    }
}

void BlueFoxParameters::set_aec(bool value)
{
	Acquisition_lock lock(package);//If the function modifies the parameters, always call the lock at the very beginning
	if(!check_cam() || !lock.is_valid()) return;
	
    mvIMPACT::acquire::CameraSettingsBlueFOX cam_settings(p_dev);
    mvIMPACT::acquire::TAutoExposureControl aec_val = value ? aecOn : aecOff;
    
    if(cam_settings.autoExposeControl.isValid()) cam_settings.autoExposeControl.write(aec_val);
    else
    {
    	std::cerr << "Warning : attempt to modify Automatic Exposure Control, but the feature is not available." << std::endl;
    }
}

void BlueFoxParameters::set_image_type(int ocv_color_code)
{
	Acquisition_lock lock(package);//If the function modifies the parameters, always call the lock at the very beginning
	//Set both the acquisition type and the transfer type to the same value
    //Supported : CV_8U, CV_16U, CV_8UC3 (see OpenCV documentation for details)
	if(!check_cam() || !lock.is_valid()) return;

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

void BlueFoxParameters::set_trigger_mode(mvIMPACT::acquire::TCameraTriggerMode trigger_mode)
{
	Acquisition_lock lock(package);//If the function modifies the parameters, always call the lock at the very beginning
	if(!check_cam() || !lock.is_valid()) return;
    
	mvIMPACT::acquire::CameraSettingsBlueFOX cam_settings(p_dev);
	cam_settings.triggerMode.write(trigger_mode);
}

//CamBlueFox : public functions
CamBlueFoxSeq::CamBlueFoxSeq(Cond_var_package& package_, int camId)
	: params(package_)
	, opened(false)
{
	init(camId);
}

CamBlueFoxSeq::~CamBlueFoxSeq()
{
	stop_acq();
}

int CamBlueFoxSeq::start_acq(bool only_one_camera)
{
	//Start the acquisition 
	//Return 0 if success, else an error code != 0
	if(!opened)
    {
       return -10;
    }
	//It is the responsability of the caller to check that the camera is not already started

    //Fill the request queue
    TDMR_ERROR result = DMR_NO_ERROR;
    while( ( result = static_cast<TDMR_ERROR>(p_fi->imageRequestSingle()) ) == DMR_NO_ERROR) {};
    if( result != DEV_NO_FREE_REQUEST_AVAILABLE )
    {
         std::cerr << "'FunctionInterface.imageRequestSingle' returned with an unexpected result: " << result  << "(" << ImpactAcquireException::getErrorCodeAsString( result ) << ")" << std::endl;
    }
    
   	mvIMPACT::acquire::CameraSettingsBlueFOX cam_settings(p_dev);
   	//See https://www.matrix-vision.com/manuals/SDK_CPP/group__DeviceSpecificInterface.html#ga7d880247a3af52241ce96ba703c526a1
   	//It seems that for 1 camera, the continuous mode provides higher framerate. In the case of multiple cameras, a trigger based  acquisition is needed for synchronization purpose
	if(!only_one_camera)
	{
		//Check if the OnHighLevel is supported by the camera
		bool mode_available_acq = check_property<mvIMPACT::acquire::TCameraTriggerMode>(ctmOnHighLevel, cam_settings.triggerMode);
		
		if(mode_available_acq) cam_settings.triggerMode.write(ctmOnHighLevel);
		else
		{
				cam_settings.triggerMode.write(ctmContinuous);
				std::cout << "Warning : camera (Serial " << p_dev->serial.read() << ") does not support external trigger. Defaulting on continuous. Synchronization might be compromised." << std::endl; 
		}
		
		if(mode_available_acq)//If we use a trigger, set the input 0
		{
			bool check_input_0 = check_property<mvIMPACT::acquire::TCameraTriggerSource>(ctsDigIn0 , cam_settings.triggerSource);
			
			if(check_input_0) cam_settings.triggerSource.write(ctsDigIn0);
		}
	}
	else //If there is only one camera, continuous is the fastest
	{
		cam_settings.triggerMode.write(ctmContinuous);
	}
	
	manually_start_acquisition_if_needed();
	
	//Empty the request queue to start on the most recent images (necessary with trigger ?)
    //We only do this when there is multiple cameras because it can slow the acquisiton
    if (!only_one_camera) empty_request_queue();
    
    return 0;
}

int CamBlueFoxSeq::stop_acq()
{
	if(!opened) return 0;
	manually_stop_acquisition_if_needed();

    if(p_fi->imageRequestReset(0,0) != mvIMPACT::acquire::DMR_NO_ERROR)//Clear reset queue
    {
        std::cerr << "Error while resetting FunctionInterface." <<std::endl;
    }

    //Extract and unlock all requests
    empty_request_queue();
    
    return 0;
}

int CamBlueFoxSeq::send_image_request()
{
	//Send a new image request to the capture queue
	int ret = -10;
    if(opened) 
    {
    	p_fi->imageRequestSingle();
    	ret = 0;
    }
    return ret;
}

int CamBlueFoxSeq::retrieve_image_request(cv::Mat& image)
{
	//Wait for the results from the default capture queue
	if(!opened) return -10;
    int requestNr = p_fi->imageRequestWaitFor(timemout_waitfor_ms);
    
    mvIMPACT::acquire::Request * pRequest = nullptr;
    pRequest = p_fi->isRequestNrValid(requestNr) ? p_fi->getRequest(requestNr) : nullptr;

    if(pRequest != nullptr)
    {
        if(pRequest->isOK())
        {
            //Do image processing        
            mvIMPACT::acquire::ImageBuffer * p_ib = pRequest->getImageBufferDesc().getBuffer();               
            export_image(p_ib->iWidth, p_ib->iHeight, p_ib->vpData, params.get_pixel_format(), image);
            //We have copied the image data a this point
        }
        else
        {
            std::cerr << "Error during image request : " << pRequest->requestResult.readS() << std::endl;
            return -1;
        }
        //This image has been used thus the buffer is no longer needed
        pRequest->unlock();
    }
    else
    {
        std::cerr << "imageRequestWaitFor failed : " << requestNr << std::endl;
        return -2;
    }
    
    return 0;
}

//Private functions:
void CamBlueFoxSeq::init(int camId)
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

    if(p_dev == nullptr) 
    {
        std::cerr << "Could not open device." << std::endl;
        return;
    }
    try
    {
        p_dev->open();
    }
    catch(mvIMPACT::acquire::ImpactAcquireException& e)
    {
        std::cerr << "An error occured while opening the device (error code: " << e.getErrorString() << ")" << std::endl;
        return;
    }
    
    try
    {
        p_fi = std::unique_ptr<mvIMPACT::acquire::FunctionInterface>(new mvIMPACT::acquire::FunctionInterface(p_dev));//Replace by make_unique in C++14
    }
    catch(std::bad_alloc& e)
    {
        std::cerr << "Fail to allocate FunctionInterface, closing." <<std::endl;
        p_dev->close();
        return;
    }
    
    //At this stage, the device has been opened successfully
    opened = true;
    //Initialize the settings
    params.set_p_dev(p_dev);
    //Settings for the acquisition
    params.set_image_size(width_d, height_d);
    params.set_agc(agc_d);
    params.set_aec(aec_d);
    params.set_trigger_mode(trigger_d); 
    //Settings for the output      
    params.set_image_type(pixel_format_output_d);
    
    std::cout << "Camera (Serial " << p_dev->serial.read() << ") has been opened successfully." << std::endl;
    if(!clock_type::is_steady)
    {
        std::cerr << "Warning : non steady clock type, timer might go back in time." << std::endl;
    }
    
}

void CamBlueFoxSeq::empty_request_queue()
{
	//Empty the request queue of the camera
	if(!opened) return;
    int requestNrEmpty = mvIMPACT::acquire::INVALID_ID;
    mvIMPACT::acquire::Request * pRequest = nullptr;
    while((requestNrEmpty = p_fi->imageRequestWaitFor(timemout_waitfor_ms)) >= 0)
    {
        pRequest = p_fi->getRequest(requestNrEmpty);
        pRequest->unlock();
    }
}

void CamBlueFoxSeq::manually_start_acquisition_if_needed()
{
	// Start the acquisition manually if this was requested(this is to prepare the driver for data capture and tell the device to start streaming data)
    if(p_dev->acquisitionStartStopBehaviour.read() == mvIMPACT::acquire::assbUser )
    {
        const mvIMPACT::acquire::TDMR_ERROR result = static_cast<mvIMPACT::acquire::TDMR_ERROR>(p_fi->acquisitionStart());
        if(result != mvIMPACT::acquire::DMR_NO_ERROR )
        {
            std::cerr << "'FunctionInterface.acquisitionStart' returned with an unexpected result: " << result << "(" << mvIMPACT::acquire::ImpactAcquireException::getErrorCodeAsString(result) << ")" << std::endl;
        }
    }
}

void CamBlueFoxSeq::manually_stop_acquisition_if_needed()
{
	// Stop the acquisition manually if this was requested
    if(p_dev->acquisitionStartStopBehaviour.read() == mvIMPACT::acquire::assbUser )
    {
        const mvIMPACT::acquire::TDMR_ERROR result = static_cast<mvIMPACT::acquire::TDMR_ERROR>(p_fi->acquisitionStop());
        if( result != mvIMPACT::acquire::DMR_NO_ERROR)
        {
            std::cerr << "'FunctionInterface.acquisitionStop' returned with an unexpected result: " << result << "(" << mvIMPACT::acquire::ImpactAcquireException::getErrorCodeAsString(result) << ")" << std::endl;
        }
    }
}


}//namespace cam
