#include "trigger.hpp"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef __unix__
#include <fcntl.h>
#include <unistd.h>
#endif

#include <chrono>
#include <thread>

namespace cam {

//Good tuto on serial : http://www.cmrr.umn.edu/~strupp/serial.html#1

#ifdef __unix__
Trigger_vcp::Trigger_vcp(const std::string& port_name_, const speed_t& baudrate_) :
						opened(false),
						port_name(port_name_),
                        baudrate(baudrate_)
{}
#else
Trigger_vcp::Trigger_vcp()
			: opened(false)
{}
#endif

Trigger_vcp::~Trigger_vcp()
{
	#ifdef __unix__
	if(opened) close(fd);
	#endif
}

void Trigger_vcp::open_vcp()
{
	if(opened) return;
	
	#ifdef __unix__
	fd = open(port_name.c_str(), O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        printf("Error opening %s: %s\n", port_name.c_str(), std::strerror(errno));
        return;
    }
    /*baudrate 115200, 8 bits, no parity, 1 stop bit */
    if(set_interface_attribs(fd, baudrate) < 0) return;

//    std::this_thread::sleep_for(std::chrono::milliseconds(delay_start_ms));

    opened = true;
    #else
    printf("Error opening the trigger : TRIGGER NOT DEFINED IN WINDOWS");
    #endif	
}

int Trigger_vcp::set_interface_attribs(int fd, int speed)
{
	#ifdef __unix__
    struct termios tty;

//    if (tcgetattr(fd, &tty) < 0)//Fetch the options from the device
//    {
//        printf("Error from tcgetattr: %s\n", strerror(errno));
//        return -1;
//    }

    cfsetospeed(&tty, static_cast<speed_t>(speed));
    cfsetispeed(&tty, static_cast<speed_t>(speed));

//    tty.c_cflag |= (CLOCAL | CREAD);    /* ignore modem controls */
//    tty.c_cflag &= ~CSIZE;
//    tty.c_cflag |= CS8;         /* 8-bit characters */
//    tty.c_cflag &= ~PARENB;     /* no parity bit */
////    tty.c_cflag &= ~CSTOPB;     /* only need 1 stop bit */
//    tty.c_cflag &= ~CRTSCTS;    /* no hardware flowcontrol */
//
//    /* setup for non-canonical mode */
////    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
////    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
////    tty.c_oflag &= ~OPOST;
//
//    /* read timeouts : wait 1/10 seconds for each byte  */
//    tty.c_cc[VMIN] = 0;
//    tty.c_cc[VTIME] = 1;

    tty.c_cflag = speed | CS8 | CREAD | CLOCAL;
    tty.c_iflag = IGNPAR;
    tty.c_oflag = 0;
    tty.c_lflag = 0;
    tty.c_cc[VTIME] = 1; // inter-character timer used, in 1/10 seconds
    tty.c_cc[VMIN] = 0;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) //Apply the options to the device
    {
        printf("Error from tcsetattr: %s\n", strerror(errno));
        return -1;
    }
    return 0;
    #else
    return -1;
    #endif
}

bool Trigger_vcp::send_trigger()
{
	if(!opened) return false;
	
	#ifdef __unix__
    int bytes_written;
	//Send the trigger signal, returns true if success, else false.

	constexpr char byte_to_send = trigger_byte;

    bytes_written = write(fd, &byte_to_send, 1);
    if (bytes_written != sizeof(char))
    {
        return false;
    }
//    if(tcdrain(fd) != 0) return false;    //Wait for data transmission

    //Wait for the end of the triggering signal

//    char received_byte = 0x00;
//    ssize_t bytes_read = read(fd,&received_byte,1);//When the trigger is finished, it sents back a byte 0xFF
//    if(bytes_read != 1 || received_byte != static_cast<char>(0xFF))
//    {
//    	return false;
//    }

    return true;
    #else
    return false;
    #endif
}

} //end of cam namespace
