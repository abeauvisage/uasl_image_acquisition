#ifndef CONFIGWINDOW_H
#define CONFIGWINDOW_H

#ifdef GTK3_FOUND
#include <gtk/gtk.h>
#endif

#include "cameraTau2.h"
#include <bitset>

const static int MAX_CONTRAST=255;
const static int MAX_BRIGTHNESS=5000;
const static int MIN_BRIGTHNESS=2000;


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
            }

        }
        ~ConfigWindow(){closeCamera();}

        void setPort(const std::string portname){
            if(m_cam.open(portname)){
                std::cout << portname << " has been opened successfully." << std::endl;
                m_cam.send_request(DO_FFC,NOARG);
            }
            else
                std::cerr << portname << " couldn't be opened." << std::endl;
        }

        #ifdef GTK3_FOUND
        static void createGUI();
        #endif // GUI

        static void doFFC(){
            m_cam.send_request(DO_FFC,NOARG);
        }
        static void saveSettings(){
            m_cam.send_request(SET_DEFAULTS,NOARG);
        }
        static void factoryReset(){
            m_cam.send_request(CAMERA_RESET,NOARG);
        }

        void closeCamera(){m_cam.close();}
        bool isOpened(){return m_cam.isOpened();}
        float setBrightness(unsigned short b){return m_cam.send_request(BRIGHTNESS,b,true);}
        static unsigned short getBrightness(){return m_cam.send_request(BRIGHTNESS,NOARG,false);}
        float setContrast(unsigned short c){return m_cam.send_request(CONTRAST,c,true);}
        static unsigned short getContrast(){return m_cam.send_request(CONTRAST,NOARG,false);}
        static signed char getDDE(){return m_cam.send_request(AUTO_DDE,NOARG,false) & 0xFF;}
        static signed short getACE(){return m_cam.send_request(ACE_CORRECT,NOARG,false);}
//        static unsigned short getSSO(){return m_cam.send_request(CONTRAST,NOARG,false);}
        static unsigned short getPlateau(){return m_cam.send_request(PLATEAU_LEVEL,NOARG,false);}
        static unsigned short getMaxGain(){return m_cam.send_request(MAX_AGC_GAIN,NOARG,false);}
        static unsigned short getITT(){return m_cam.send_request(AGC_MIDPOINT,NOARG,false);}
        static unsigned short getAGCFilter(){return m_cam.send_request(AGC_FILTER,NOARG,false);}
        static unsigned short getTailRejection(){return m_cam.send_request(TAIL_SIZE,NOARG,false);}
        static int getAGCMode(){return m_cam.send_request(AGC_TYPE,NOARG,false);}
        static void setAGCMode(int mode){m_cam.send_request(AGC_TYPE,mode,true);}

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

    #ifdef GTK3_FOUND
    static void gcb_AGCMode(GtkComboBox* box){
            int mode = gtk_combo_box_get_active(box);
            switch(mode){
            case 4:
                m_cam.send_request(AGC_TYPE,AGC_TYPE_LINEAR,true);break;
            case 5:
                m_cam.send_request(AGC_TYPE,AGC_TYPE_INFOBASED,true);break;
            case 6:
                m_cam.send_request(AGC_TYPE,AGC_TYPE_INFOBASEDEQ,true);break;
            default:
                m_cam.send_request(AGC_TYPE,mode,true);//break;
            }
        }

    static void gcb_contrast(GtkAdjustment* widget){
        m_contrast = (unsigned short) gtk_adjustment_get_value(widget);
        m_cam.send_request(CONTRAST,m_contrast,true);
    }

    static void gcb_brightness(GtkAdjustment* widget){
        m_brightness = (unsigned short) gtk_adjustment_get_value(widget);
        m_cam.send_request(BRIGHTNESS,m_brightness,true);
    }

    static void gcb_DDE(GtkAdjustment* widget){
        unsigned char value = (unsigned char) gtk_adjustment_get_value(widget);
        unsigned short svalue = 0x0100 | value;
        m_cam.send_request(AUTO_DDE,svalue,true);
    }

    static void gcb_ACE(GtkAdjustment* widget){
        signed short value =  gtk_adjustment_get_value(widget);
//        std::bitset<16>  bs(value);
//        std::cout << "ace: " << bs << std::endl;
        m_cam.send_request(ACE_CORRECT,value,true);
    }

//    static void gcb_SSO(GtkAdjustment* widget){
//        unsigned short value = (unsigned short) gtk_adjustment_get_value(widget);
//        m_cam.send_request(ACE_CORRECT,value,true);
//    }

    static void gcb_Plateau(GtkAdjustment* widget){
        unsigned short value = (unsigned short) gtk_adjustment_get_value(widget);
        m_cam.send_request(PLATEAU_LEVEL,value,true);
    }

    static void gcb_MaxGain(GtkAdjustment* widget){
        unsigned short value = (unsigned short) gtk_adjustment_get_value(widget);
        m_cam.send_request(MAX_AGC_GAIN,value,true);
    }

    static void gcb_ITT(GtkAdjustment* widget){
        unsigned short value = (unsigned short) gtk_adjustment_get_value(widget);
        m_cam.send_request(AGC_MIDPOINT,value,true);
    }

    static void gcb_AGCFilter(GtkAdjustment* widget){
        unsigned short value = (unsigned short) gtk_adjustment_get_value(widget);
        m_cam.send_request(AGC_FILTER,value,true);
    }

    static void gcb_TailRejection(GtkAdjustment* widget){
        unsigned short value = (unsigned short) gtk_adjustment_get_value(widget)*10;
        m_cam.send_request(TAIL_SIZE,value,true);
    }

    #endif // GTK3_FOUND

    static CameraTau2 m_cam;
};

#endif // CONFIGWINDOW_H
