#ifndef UASL_IMAGE_ACQUISITION_UTIL_SIGNAL_HPP
#define UASL_IMAGE_ACQUISITION_UTIL_SIGNAL_HPP

#ifdef _WIN32
#include <windows.h>
#else
#include <signal.h>
#include <sys/signalfd.h>
#include <poll.h>
#include <unistd.h>
#endif


//Linux case :
//NOTE : it is the responsability of the caller to define signal handling. The recommanded design is to block all
//signals in main, so that each spawned thread ignore them also, and to periodically check them with a sigwait or
//signalfd
//See http://pubs.opengroup.org/onlinepubs/9699919799/functions/V2_chap02.html#tag_15_04
//Note that initialising this class will block all signals in this thread and the future spawned threads

//Windows case :
//The signal handler simply catch the signals ctrl-c and ctrl-close without exiting the process, so that it can exit naturally if the user wants to do so.
//


namespace cam
{
class SigHandler
{
	#ifdef _WIN32
	private:
	BOOL ctrl_handler(DWORD fdw_ctrl_type)
	{
		switch(fdw_ctrl_type)
		{
			case CTRL_C_EVENT:
			case CTRL_CLOSE_EVENT:
				signal_catched = true;
				return TRUE;//We don't call the other handlers (i.e. including the terminating process
			case CTRL_BREAK_EVENT:
			case CTRL_LOGOFF_EVENT:
			case CTRL_SHUTDOWN_EVENT:
				signal_catched = true;
				return FALSE;
			default:
				return FALSE;
		}	
	}
	
	bool signal_catched;
	#endif
	
    public:
    SigHandler()
    	: valid(true)
    	#ifdef _WIN32
    	, signal_catched(false)
    	#endif
        
    {
    	#ifdef _WIN32
    	if (!SetConsoleCtrlHandler(this->ctrl_handler, TRUE)) 
    	{
        	valid = false;
    	}
    	#else
        // Block signals in this thread, from
        //http://stackoverflow.com/questions/13498309/am-i-over-engineering-per-thread-signal-blocking
        //Return 0 if success, else an error code (man pthread_sigmask)
        sigemptyset(&mask);
        sigaddset(&mask, SIGINT);
        sigaddset(&mask, SIGTERM);
        sigaddset(&mask, SIGABRT);
        sigaddset(&mask, SIGHUP);
        sigaddset(&mask, SIGPIPE);
        if(pthread_sigmask(SIG_BLOCK, &mask, NULL))
        {
            valid = false;
        }

        sfd = signalfd(-1, &mask, 0);//Create the signal file descriptor
        if(sfd == -1)
        {
            valid = false;
        }
        pfd.fd = sfd;
        pfd.events = POLLIN;//There is data to read
        #endif
    }

    bool is_valid() const { return valid; }

    bool check_term_sig(int * signal = nullptr)
    {
    	#ifdef _WIN32
    	if(!is_valid())
        {
			//The initialisation failed
            return true;
        }
        
        return !signal_catched;//Return true if no signal is found
    	#else
    	//Check if any signal requiring termination has been received
    	//If a non null pointer is provided, the value of the received signal will be stored here
        int sig_received=-1;
        int ret_signal = get_signal(sig_received);
        if(!ret_signal)
        {
            switch(sig_received)
            {
                case SIGINT:
                case SIGTERM:
                case SIGABRT:
                	if(signal) *signal = sig_received;
                	return false;
            }
        }
        else if(ret_signal == 2) return false;
        return true;
		#endif
    }

    

    private:
    bool valid;
    
    #ifndef _WIN32
    sigset_t mask;
    struct signalfd_siginfo fdsi;
    int sfd;
    struct pollfd pfd;
    
    int get_signal(int& signal)
    {
        //Get a signal if it has been catched.
        //If the signal was catched, return 0 and the signal is saved
        //If there was no signal, returns 1.
        //If there was an error, returns 2.
        if(!is_valid())
        {
			//The initialisation failed
            return 2;
        }
        int ret_poll = poll(&pfd, 1, 0);

        if(!(ret_poll > 0 && (pfd.revents & POLLIN)))
        {
        	  //No signal; was received
            return 1;
        }
        //The signal is ready, read it
        ssize_t s = read(sfd, &fdsi, sizeof(struct signalfd_siginfo));
        if(s != sizeof(struct signalfd_siginfo))
        {
            return 2;
        }
        signal = fdsi.ssi_signo;
        return 0;
    }
    #endif
}; //class SigHandler

} //namespace cam
#endif
