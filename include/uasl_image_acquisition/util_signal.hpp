#ifndef UASL_IMAGE_ACQUISITION_UTIL_SIGNAL_HPP
#define UASL_IMAGE_ACQUISITION_UTIL_SIGNAL_HPP

#include <signal.h>
#include <sys/signalfd.h>
#include <poll.h>
#include <unistd.h>

//NOTE : it is the responsability of the caller to define signal handling. The recommanded design is to block all
//signals in main, so that each spawned thread ignore them also, and to periodically check them with a sigwait or
//signalfd
//See http://pubs.opengroup.org/onlinepubs/9699919799/functions/V2_chap02.html#tag_15_04
//Note that initialising this class will block all signals in this thread and the future spawned threads

namespace cam
{
class SigHandler
{
    public:
    SigHandler() :
        valid(true)
    {
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
    }

    bool is_valid() const { return valid; }

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

    private:
    sigset_t mask;
    struct signalfd_siginfo fdsi;
    int sfd;
    struct pollfd pfd;
    bool valid;
}; //class SigHandler

} //namespace cam
#endif
