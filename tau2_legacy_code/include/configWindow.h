#ifndef UASL_IMAGE_ACQUISITION_CONFIGWINDOW_H
#define UASL_IMAGE_ACQUISITION_CONFIGWINDOW_H

#include "cameraTau2.h"

class ConfigWindow
{
    public:
        ConfigWindow(const std::string portname=""){
            if(!portname.empty()){
                if(m_cam.open(portname))
                        std::cout << portname << " has been opened successfully." << std::endl;
                    else
                        std::cerr << portname << " couldn't be opened." << std::endl;
                m_cam.send_request(DO_FFC,NOARG);
                m_cam.send_request(AGC_TYPE,AGC_TYPE_AUTO_BRIGHT,true);
            }
        }

        void setPort(const std::string portname){
            if(m_cam.open(portname))
                        std::cout << portname << " has been opened successfully." << std::endl;
                    else
                        std::cerr << portname << " couldn't be opened." << std::endl;
                m_cam.send_request(DO_FFC,NOARG);
                m_cam.send_request(AGC_TYPE,AGC_TYPE_AUTO_BRIGHT,true);
        }

        void show(){
            if(!m_cam.isOpened())
                return;
            cv::namedWindow("Tau2 config",cv::WINDOW_AUTOSIZE);
            m_contrast = m_cam.send_request(CONTRAST,NOARG,false);m_slider_contrast=m_contrast;
            std::cout << "CONTRAST " << m_contrast << std::endl;
            m_brightness = m_cam.send_request(BRIGHTNESS,NOARG,false);m_slider_brightness = m_brightness;
            m_brightness = 3271;
            std::cout << "BRIGHTNESS " << m_brightness << std::endl;
            cv::createTrackbar("Constrast","Tau2 config",&m_slider_contrast,MAX_CONTRAST,trackbar_contrast);
            cv::createTrackbar("Brightness","Tau2 config",&m_slider_brightness,MAX_BRIGTHNESS,trackbar_brightness);
            trackbar_contrast(m_contrast,0);
            trackbar_brightness(m_brightness,0);
        }

        void doFFC(){
            m_cam.send_request(DO_FFC,NOARG);
        }

        void closeCamera(){m_cam.close();}
        void setBrightness(unsigned short b){m_cam.send_request(BRIGHTNESS,b,true);}
        unsigned short getBrightness(){return m_cam.send_request(BRIGHTNESS,NOARG,false);}
        void setConstrast(unsigned short c){m_cam.send_request(CONTRAST,c,true);}
        void setAGCMode(int mode){
            m_cam.send_request(AGC_TYPE,mode,true);
        }

    private:

    static unsigned short m_contrast,m_brightness;
    static int m_slider_contrast, m_slider_brightness;

    static void trackbar_contrast(int, void*){
        m_contrast = (unsigned short) m_slider_contrast;
        m_cam.send_request(CONTRAST,m_contrast,true);
    }

    static void trackbar_brightness(int, void*){
        m_brightness = (unsigned short) m_slider_brightness;
        m_cam.send_request(BRIGHTNESS,m_brightness,true);
    }

    static CameraTau2 m_cam;

    const static int MAX_CONTRAST=255;
    const static int MAX_BRIGTHNESS=5000;
    const static int MIN_BRIGTHNESS=2000;
};

#endif // CONFIGWINDOW_H
