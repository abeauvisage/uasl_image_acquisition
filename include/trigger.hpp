#ifndef IMAGE_ACQUISITION_TRIGGER_HPP
#define IMAGE_ACQUISITION_TRIGGER_HPP

#include <termios.h>
#include <string>

namespace cam {

static constexpr char trigger_byte = 0x01;//Byte to send for activating the trigger
static const std::string port_name_d = "/dev/ttyUSB0";//Default name of the VCP port
static constexpr speed_t baudrate = B115200;//Baudrate
static constexpr unsigned int delay_start_ms = 5000;//Time to wait for bootloader startup in milliseconds.

class Trigger_vcp
{
	public:
	Trigger_vcp();
	virtual ~Trigger_vcp();
	
	void open_vcp();
	
	bool send_trigger();
	
	bool is_opened() const
	{
		return opened;
	}
	
	private:
	bool opened;//True if the device was opened correctly
	int fd;//File descriptor
	
	std::string port_name;
	
	int set_interface_attribs(int fd, int speed);//Set the speed and flags of the VCP port
	
};
} //end of cam namespace

#endif
