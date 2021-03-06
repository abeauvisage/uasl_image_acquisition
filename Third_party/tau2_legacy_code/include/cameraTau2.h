#ifndef CAMERATAU2_H
#define CAMERATAU2_H

#include <string>
#include <iostream>
#include <mutex>

#include <SerialStream.h>
#include <SerialPort.h>

#include "opencv2/opencv.hpp"

/**** Functions Definition ****/

#define SET_DEFAULTS 0x01
#define CAMERA_RESET 0x02
#define RESTORE_FACTORY_DEFAULTS 0x03
#define SERIAL_NUMBER 0x04
#define BAUD_RATE 0x07
#define GAIN_MODE 0x0A
#define FFC_MODE_SELECT 0x0B
#define DO_FFC 0x0C
#define AGC_TYPE 0x13
#define CONTRAST 0x14
#define BRIGHTNESS 0x15
#define EXTERNAL_SYNC 0x21
#define PLATEAU_LEVEL 0x3F
#define AGC_MIDPOINT 0x55
#define AGC_FILTER 0x3E
#define MAX_AGC_GAIN 0x6A
#define TAIL_SIZE 0x1B
#define ACE_CORRECT 0x1C
#define AUTO_DDE 0xE3

/**** Arguments Definition ****/

#define NOARG 0xFFFF
#define BAUD_RATE_AUTOBAUD 0x0000
#define BAUD_RATE_19200 0x0001
#define BAUD_RATE_28800 0x0002
#define BAUD_RATE_57600 0x0003
#define BAUD_RATE_115200 0x0004
#define GAIN_MODE_AUTOMATIC OxOOOO
#define GAIN_MODE_LOW OxOOO1
#define GAIN_MODE_HIGH OxOOO2
#define GAIN_MODE_MANUAL OxOOO3
#define FFC_MODE_SELECT_MANUAL 0x0000
#define FFC_MODE_SELECT_AUTOMATIC 0x0001
#define FFC_MODE_SELECT_EXTERNAL 0x0002
#define DO_FFC_SHORT 0x0000
#define DO_FFC_LONG 0x0001
#define AGC_TYPE_PLATEAU 0x0000
#define AGC_TYPE_ONCE_BRIGHT 0x0001
#define AGC_TYPE_AUTO_BRIGHT 0x0002
#define AGC_TYPE_MANUAL 0x0003
#define AGC_TYPE_NOT_DEF 0x0004
#define AGC_TYPE_LINEAR 0x0005
#define AGC_TYPE_INFOBASED 0x0009
#define AGC_TYPE_INFOBASEDEQ 0x000A
#define EXTERNAL_SYNC_DISABLED 0x0000
#define EXTERNAL_SYNC_SLAVE 0x0001
#define EXTERNAL_SYNC_MASTER 0x0002

class CameraTau2
{
    public:
        CameraTau2(const std::string portname=""){
            if(!portname.empty()){
                if(open(portname))
                    std::cout << portname << " has been opened successfully." << std::endl;
                else
                    std::cerr << portname << " couldn't be opened." << std::endl;

                send_request(DO_FFC,NOARG);
            }
        }

        bool open(const std::string portname="/dev/ttyUSB0");
        bool isOpened()const{return m_serPort.good();}
        unsigned short send_request(const unsigned char fnumber, const unsigned short argument=NOARG, const bool set=false);

        ~CameraTau2(){close();}
        void close(){m_serPort.Close();m_serPort.~SerialStream();}

    private:

        LibSerial::SerialStream m_serPort;
        const static int BUFFER_SIZE=50;
        std::mutex m_mutex;

        unsigned char m_request[BUFFER_SIZE];
        char m_response[BUFFER_SIZE];

        int build_request(const unsigned char fnumber, const unsigned short argument=NOARG, const bool set=false);
        unsigned short computeCRC_CCITT(int nb_bytes);
};

#endif // CAMERATAU2_H
