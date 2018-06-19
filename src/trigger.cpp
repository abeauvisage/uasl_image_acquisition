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

Trigger_vcp::Trigger_vcp() :
					opened(false)
{}


Trigger_vcp::~Trigger_vcp()
{
	#ifdef __unix__
	if(opened) close(fd);
	#endif
}

#ifdef __unix__
void Trigger_vcp::open_vcp(const std::string& port_name, const speed_t& baudrate)
{
	if(opened) return;
	

	fd = open(port_name.c_str(), O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        printf("Error opening %s: %s\n", port_name.c_str(), std::strerror(errno));
        return;
    }
    /*baudrate 115200, 8 bits, no parity, 1 stop bit */
    if(set_interface_attribs(fd, baudrate) < 0) return;

    opened = true;	
}
#else
void Trigger_vcp::open_vcp()
{
	printf("Error opening the trigger : TRIGGER NOT DEFINED IN WINDOWS");
}
#endif

int Trigger_vcp::set_interface_attribs(int fd, int speed)
{
	#ifdef __unix__
    struct termios tty;

    cfsetospeed(&tty, static_cast<speed_t>(speed));
    cfsetispeed(&tty, static_cast<speed_t>(speed));

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

    return true;
    #else
    return false;
    #endif
}

} //end of cam namespace
