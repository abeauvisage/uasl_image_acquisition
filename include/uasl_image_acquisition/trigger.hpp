#ifndef UASL_IMAGE_ACQUISITION_TRIGGER_HPP
#define UASL_IMAGE_ACQUISITION_TRIGGER_HPP

#ifdef __unix__
#include <termios.h>
#endif

#include <string>

namespace cam {

static constexpr char trigger_byte = 0x00;//Byte to send for activating the trigger

class Trigger_vcp
{
	public:
	Trigger_vcp();
	
	virtual ~Trigger_vcp();

	#ifdef __unix__
	void open_vcp(const std::string& port_name, const speed_t& baudrate);
	#else
	void open_vcp();
	#endif

	bool send_trigger();

	bool is_opened() const
	{
		return opened;
	}

	private:
	bool opened;//True if the device was opened correctly
	int fd;//File descriptor

	int set_interface_attribs(int fd, int speed);//Set the speed and flags of the VCP port

};
} //end of cam namespace

#endif
