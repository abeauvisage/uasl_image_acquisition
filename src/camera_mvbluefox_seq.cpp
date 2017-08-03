#include "camera_mvbluefox_seq.hpp"

#include "acquisition.hpp"

#include <iostream>

namespace cam
{
//BlueFoxParameters : Public functions
void BlueFoxParameters::set_image_size(int width, int height)
{
	Acquisition_lock lock(package);//If the function modifies the parameters, always call the lock at the very beginning
	//Set the image size, if the value is negative, keep former value, if value is 0, set it to the maximum value if available
	if(!check_cam() || !lock.is_valid()) return;
	
    mvIMPACT::acquire::CameraSettingsBlueFOX cam_settings(p_dev);
    if(width > 0) cam_settings.aoiWidth.write(width);
    else if(width == 0)
    {
    	if(cam_settings.aoiWidth.hasMaxValue()) cam_settings.aoiWidth.write(cam_settings.aoiWidth.getMaxValue());
    	else
    	{
    		std::cerr << "Warning : attempt to set width to max value, but max value is not available. Width has not been changed." << std::endl;
    	}
    }
    if(height > 0) cam_settings.aoiHeight.write(height);
    else if(height == 0)
    {
    	if(cam_settings.aoiHeight.hasMaxValue()) cam_settings.aoiHeight.write(cam_settings.aoiHeight.getMaxValue());
    	else
    	{
    		std::cerr << "Warning : attempt to set height to max value, but max value is not available. Height has not been changed." << std::endl;
    	}
    }	
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

void BlueFoxParameters::set_exposure_time(int exposure_time_us)
{
	//The exposure time is in microseconds
	Acquisition_lock lock(package);//If the function modifies the parameters, always call the lock at the very beginning
	if(!check_cam() || !lock.is_valid()) return;
	
	mvIMPACT::acquire::CameraSettingsBlueFOX cam_settings(p_dev);
	cam_settings.expose_us.write(exposure_time_us);
}

void BlueFoxParameters::set_pixelclock(mvIMPACT::acquire::TCameraPixelClock pixelclock)
{
	Acquisition_lock lock(package);//If the function modifies the parameters, always call the lock at the very beginning
	if(!check_cam() || !lock.is_valid()) return;
	
	mvIMPACT::acquire::CameraSettingsBlueFOX cam_settings(p_dev);
	if(cam_settings.pixelClock_KHz.isValid())
	{
		bool check_pixelclock_values = check_property<mvIMPACT::acquire::TCameraPixelClock>(pixelclock, cam_settings.pixelClock_KHz);
		if(check_pixelclock_values)
		{
			cam_settings.pixelClock_KHz.write(pixelclock);
		}
		else
		{
			std::cerr << "Warning : trying to set pixelclock to an invalid value." << std::endl;
		}
	}
	else
	{
		std::cerr << "Warning : attempt to set pixelclock, but setting is unavailable." << std::endl;
	}
}

void BlueFoxParameters::set_request_timeout_ms(int timeout_ms)
{
	//Timeout for an image request in milliseconds
	Acquisition_lock lock(package);//If the function modifies the parameters, always call the lock at the very beginning
	if(!check_cam() || !lock.is_valid()) return;
	
	mvIMPACT::acquire::CameraSettingsBlueFOX cam_settings(p_dev);
	
	cam_settings.imageRequestTimeout_ms.write(timeout_ms);
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
	if(opened) p_dev->close();
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
	
	//Setting the trigger mode
   	mvIMPACT::acquire::CameraSettingsBlueFOX cam_settings(p_dev);
   	
   	
   	//See https://www.matrix-vision.com/manuals/SDK_CPP/group__DeviceSpecificInterface.html#ga7d880247a3af52241ce96ba703c526a1
   	//It seems that for 1 camera, the continuous mode provides higher framerate. In the case of multiple cameras, a trigger based  acquisition is needed for synchronization purpose
   	//Keep in mind that when the trigger is set to OnHighLevel, the triggering will happen as long as the voltage is on high, so this time has to be smaller than exposure + framedelay to prevent multiple acquisition
   	//See https://www.matrix-vision.com/manuals/mvBlueFOX/Appendix_CMOS_page_0.html#CMOS1280d_section_1_1 for explaination about frame rate
	if(!only_one_camera)
	{
		//Check if the OnHighLevel is supported by the camera
		bool mode_available_acq = check_property<mvIMPACT::acquire::TCameraTriggerMode>(ctmOnHighLevel, cam_settings.triggerMode);
		
		if(mode_available_acq)
		{
			cam_settings.triggerMode.write(ctmOnHighLevel);
			
			//Print the maximal framerate
			if(cam_settings.expose_us.isValid() && cam_settings.aoiHeight.isValid()) std::cout << "Maximal frame rate for camera (Serial " << p_dev->serial.read() << ") in FPS : " << 1.0/(static_cast<double>(cam_settings.expose_us.read())/1e6 + (static_cast<double>(cam_settings.aoiHeight.read()) * (1650.0 / 40e6)) + (25.0 * (1650.0 / 40e6))) << std::endl;
		}
		else
		{
				cam_settings.triggerMode.write(ctmContinuous);
				std::cout << "Warning : camera (Serial " << p_dev->serial.read() << ") does not support external trigger. Defaulting on continuous. Synchronization might be compromised." << std::endl; 
		}
		
		if(mode_available_acq)//If we use a trigger, set the input 0
		{
			bool check_input_0 = check_property<mvIMPACT::acquire::TCameraTriggerSource>(ctsDigIn1 , cam_settings.triggerSource);
			
			if(check_input_0) cam_settings.triggerSource.write(ctsDigIn1);
			else
			{
				std::cout << "Warning : the digitial input used is not available." << std::endl;
			}			
		}
	}
	else //If there is only one camera, continuous is the fastest
	{
		cam_settings.triggerMode.write(ctmContinuous);
	}
	
	
	int request_number;
	//print_all_request_state();
    //Fill the request queue
    TDMR_ERROR result = DMR_NO_ERROR;
    while( ( result = static_cast<TDMR_ERROR>(p_fi->imageRequestSingle(nullptr, &request_number)) ) == DMR_NO_ERROR) 
    {}
    if( result != DEV_NO_FREE_REQUEST_AVAILABLE )
    {
         std::cerr << "Unable to fill request queue : " << result  << "(" << ImpactAcquireException::getErrorCodeAsString( result ) << ")" << std::endl;
    }    
	manually_start_acquisition_if_needed();
    
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

int CamBlueFoxSeq::retrieve_image(cv::Mat& image)
{
	//The model used here is to keep the request queue full, and to use an external trigger. This means that some of the requests can time out.
	//For this reason, we always check the number of results, and discard them all except the last one.
	//Please keep in mind that the request queue is supposed to be full, except if there are timeouts, which we check first.

	if(!opened) return -10;
	
	//The trigger is sent before each acquisition, so we discard all the invalid acquisition first (there should be none in usual case, if the trigger does not fire for longer than the timeout, then some incorrect acquisitions can arrive in the result queue : this loop eliminates them)
    int ret = -1;
	
	//Two cases : either the correct request is in the result queue, or not yet. We check for the size of the result queue + 1 for this reason.
	//The loop is stopped when:
	//-we have a correct image (ret is changed to 0)
	//-there has been no correct image (which implies a timeout on the imageRequestWaitFor), ret is changed to -2
	const int result_queue_size = p_fi->imageRequestResultQueueElementCount();
	for(int i = 0; (i < result_queue_size + 1) && (ret == -1); ++i)
	{    	
		int requestNr = p_fi->imageRequestWaitFor(timemout_waitfor_ms);//If there is already something in the queue, the function should return immediately.
    
    	mvIMPACT::acquire::Request * pRequest = nullptr;
    	pRequest = p_fi->isRequestNrValid(requestNr) ? p_fi->getRequest(requestNr) : nullptr;
    	
		if(pRequest != nullptr)
		{			
			if(pRequest->isOK())
			{
				//Export the image into the buffer       
				mvIMPACT::acquire::ImageBuffer * p_ib = pRequest->getImageBufferDesc().getBuffer();               
				export_image(p_ib->iWidth, p_ib->iHeight, p_ib->vpData, params.get_pixel_format(), image);
				//We have copied the image data a this point
				ret = 0;
			}
			//If the request is not ok, it is probably a timeout of the request, we silently discard it.
			pRequest->unlock();//In any case, we unlock the request
		}
		else //Note that this cases should only happen on the last index (i = index result_queue_size), by definition, the timeout cannot occur before.
		{
			TDMR_ERROR result = DMR_NO_ERROR;
		    while( ( result = static_cast<TDMR_ERROR>(p_fi->imageRequestSingle()) ) == DMR_NO_ERROR) 
			{}
			p_fi->imageRequestReset(0,0);
			
			ret = -2;
		}
		int result = p_fi->imageRequestSingle();//Send another image request to keep the queue full
	}
    
    return ret;
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
    
    //Check if manual start/stop is possible.
    /*
    if(check_property<mvIMPACT::acquire::TAcquisitionStartStopBehaviour>(assbUser, p_dev->acquisitionStartStopBehaviour))
    {
    	p_dev->acquisitionStartStopBehaviour.write( assbUser );	
    }
    else
    {
    	std::cout << "Warning : manual start/stop is not available." << std::endl;
    }*/
    
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
    params.set_image_size(0, 0);//Max width and height
    params.set_agc(agc_d);//Automatic gain control
    params.set_aec(aec_d);//Automatic exposure control
    params.set_trigger_mode(trigger_d);//Trigger mode
    params.set_exposure_time(exposure_us_d);//Exposure time
    params.set_pixelclock(pixelclock_d);//Pixel clock
    params.set_request_timeout_ms(image_request_timeout_ms_d);//image request timeout
    //Settings for the output      
    params.set_image_type(pixel_format_d);
    
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
	//VERSION 1
	/*
    int requestNrEmpty = mvIMPACT::acquire::INVALID_ID;
    mvIMPACT::acquire::Request * pRequest = nullptr;
    while((requestNrEmpty = p_fi->imageRequestWaitFor(timemout_waitfor_ms)) >= 0)
    {
        pRequest = p_fi->getRequest(requestNrEmpty);
        pRequest->unlock();
    }*/
    
    //VERSION 2
	p_fi->imageRequestReset(0,0);
    
}

void CamBlueFoxSeq::manually_start_acquisition_if_needed()
{
	// Start the acquisition manually if this was requested(this is to prepare the driver for data capture and tell the device to start streaming data)
    if(p_dev->acquisitionStartStopBehaviour.read() == mvIMPACT::acquire::assbUser )
    {
    	std::cout << "Manual startup." << std::endl;
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

void CamBlueFoxSeq::print_all_request_state()
{
	//For debugging purpose
	if(!opened) return;
	
	const int request_counter = static_cast<int>(p_fi->requestCount());
	

	for( int i=0; i<request_counter; i++ )
	{
 
		mvIMPACT::acquire::Request * request = p_fi->getRequest(i);
		
		mvIMPACT::acquire::PropertyIRequestState state = request->requestState;
		
		std::cout << "Request " << i << " is in state : " << state.readS() << std::endl;
		
	}
	std::cout << std::endl;
}


}//namespace cam
