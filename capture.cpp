#define DISPLAY 0 // saving or displaying images

/*** camera selection (0: left, 1: right)***/
#define VISIBLE_0 26802713  //26802713  32902193
#define VISIBLE_1 0         //          32902184
#define INFRARED_0 0
#define INFRARED_1 1

/*** cropping for infrared ***/
#define WIDTH 640 //640   1280
#define HEIGHT 480 //480   1024
#define START 56 //56   0


#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>         /* for videodev2.h */
#include <linux/videodev2.h>
#include <sys/soundcard.h>
#include <sstream>
#include <iomanip>
#include <fstream>

#include <time.h>
#include <signal.h>

#include "opencv2/opencv.hpp"

/*** bluefox ***/
#include "apps/Common/exampleHelper.h"
#include <mvIMPACT_CPP/mvIMPACT_acquire.h>

/*** xsens IMU ***/
//#include <xsens/xsportinfoarray.h>
//#include <xsens/xsdatapacket.h>
//#include <xsens/xstime.h>
//#include <xcommunication/legacydatapacket.h>
//#include <xcommunication/int_xsdatapacket.h>
//#include "deviceclass.h"

#define CLEAR(x) memset (&(x), 0, sizeof (x))


using namespace std;
using namespace cv;
using namespace mvIMPACT::acquire;

std::string dir = "/home/abeauvisage/Insa/PhD/datasets/16_10_27/test_16_10_27-b/"; // directory where to save images
std::ofstream image_data_file;
//DeviceClass device;
//XsPortInfo mtPort;
//XsByteArray data;
//XsMessageArray msgs;

/*** global variables for each camera ***/

#if VISIBLE_0
Device* pDev_0=0;
Request *pRequest_0 = 0;
Request *pPrevRequest_0 = 0;
int request_0=INVALID_ID;
#endif // VISIBLE_0

#if VISIBLE_1
Device* pDev_1=0;
Request *pRequest_1 = 0;
Request *pPrevRequest_1 = 0;
int request_1=INVALID_ID;
#endif // VISIBLE_1

unsigned int img_nb=1;

struct buffer {
	void *		start;
	size_t		length;
};

#if INFRARED_0
static char *		dev_name_0	= NULL;
static int		fd_0		= -1;
struct buffer *		buffers_0		= NULL;
static unsigned int	n_buffers_0	= 0;
#endif // INFRARED_0

#if INFRARED_1
static char *		dev_name_1	= NULL;
static int		fd_1		= -1;
struct buffer *		buffers_1		= NULL;
static unsigned int	n_buffers_1	= 0;
#endif // INFRARED_1

static off_t total_size = 0;
static struct timeval time_start;

static void
errno_exit (const char *s)
{
	fprintf (stderr, "%s error %d, %s\n",
		s, errno, strerror (errno));

	exit (EXIT_FAILURE);
}

static int
xioctl (int fd, int request, void *arg)
{
	int r;

	do {
		r = ioctl (fd, request, arg);
	} while (-1 == r && EINTR == errno);

	return r;
}


static void
process_images (const void *p, int len, unsigned int seq, const void *p2, int len2, unsigned int seq2) //creating opencv Mat and display/saving images
{
    #if INFRARED_0
	unsigned char *pptr = (unsigned char*) p;
    cv::Mat img_0(480, 640, CV_8UC1,pptr);
    #endif // INFRARED_0
    #if INFRARED_1
    unsigned char *pptr2 = (unsigned char*) p2;
    cv::Mat img_1(480, 640, CV_8UC1,pptr2);
    #endif // INFRARED_1

    #if VISIBLE_0
	const ImageBuffer *pib =  pRequest_0->getImageBufferDesc().getBuffer();
	cv::Mat img_0(pib->iHeight,pib->iWidth,CV_8UC1,pib->vpData);
	#endif // VISIBLE_0
    #if VISIBLE_1
	const ImageBuffer *pib2 =  pRequest_1->getImageBufferDesc().getBuffer();
	cv::Mat img_1(pib2->iHeight,pib2->iWidth,CV_8UC1,pib2->vpData);
    #endif // VISIBLE_1

    // retrieving time
	struct timeval act_time;
	long ms_time, seconds, useconds;
	gettimeofday(&act_time, NULL);
	seconds = act_time.tv_sec;
	useconds = act_time.tv_usec;

	ms_time = ((seconds)*1000+useconds/1000.0+0.5);
	cout << "time: " << ms_time << endl;

    stringstream num; num << std::setfill('0') << std::setw(5) << img_nb; // saving image number
    #if DISPLAY
    #if VISIBLE_0
	cv::imshow("left",img_0);
	#endif // VISIBLE_0
	#if VISIBLE_1
	cv::imshow("right",img_1);
	#endif // VISIBLE_1
	#if INFRARED_0
	cv::imshow("left",img_0);
	#endif // INFRARED_0
	#if INFRARED_1
	cv::imshow("right",img_1);
	#endif // INFRARED_1
	cv::waitKey(5);
	#else // if images are not displayed they are saved
	#if VISIBLE_0
	imwrite(dir+"cam0_image"+num.str()+".png",img_0);
	#endif // VISIBLE_0
	#if VISIBLE_1
	imwrite(dir+"cam1_image"+num.str()+".png",img_1);
	#endif // VISIBLE_1
	#if INFRARED_0
    imwrite(dir+"cam0_image"+num.str()+".png",img_0);
    #endif // INFRARED_0
    #if INFRARED_1
    imwrite(dir+"cam1_image"+num.str()+".png",img_1);
    #endif // INFRARED_1
	image_data_file << num.str() << "," << ms_time << ";" << std::endl;
	#endif // DISPLAY
	img_nb++;
}

static int
read_frame			(FunctionInterface& fi, FunctionInterface& fi2) // retrieving buffers from the cameras and requesting new ecquisition
{
    #if INFRARED_0
	struct v4l2_buffer buf_0;
	unsigned int i;
    CLEAR (buf_0);

    buf_0.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf_0.memory = V4L2_MEMORY_MMAP;

    if (-1 == xioctl (fd_0, VIDIOC_DQBUF, &buf_0)) {
        switch (errno) {
        case EAGAIN:
            return 0;

        case EIO:
            /* Could ignore EIO, see spec. */

            /* fall through */

        default:
            fprintf(stderr,"error");
            errno_exit ("VIDIOC_DQBUF");
        }
    }
    assert (buf_0.index < n_buffers_0);
    #endif // INFRARED_0
    #if INFRARED_1
	struct v4l2_buffer buf_1;
	unsigned int i2;
    CLEAR (buf_1);

    buf_1.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf_1.memory = V4L2_MEMORY_MMAP;

    if (-1 == xioctl (fd_1, VIDIOC_DQBUF, &buf_1)) {
        switch (errno) {
        case EAGAIN:
            return 0;

        case EIO:
            /* Could ignore EIO, see spec. */

            /* fall through */

        default:
            fprintf(stderr,"error");
            errno_exit ("VIDIOC_DQBUF");
        }
    }
    assert (buf_1.index < n_buffers_1);
    #endif // INFRARED_1

    #if VISIBLE_0 && VISIBLE_1
    process_images (0,0,0,0,0,0);
    #elif INFRARED_0 && !INFRARED_1
    cout << buffers_0[0].start << endl;
    process_images (buffers_0[buf_0.index].start,
               buf_0.bytesused,
               buf_0.sequence,
               0,0,0);
    #elif INFRARED_1 && !INFRARED_0
    process_images (0,0,0,buffers_1[buf_1.index].start,
               buf_1.bytesused,
               buf_1.sequence
              );
    #else
    process_images (buffers_0[buf_0.index].start,
               buf_0.bytesused,
               buf_0.sequence,
               buffers_1[buf_1.index].start,
               buf_1.bytesused,
               buf_1.sequence
               );
    #endif // INFRARED_0
    #if INFRARED_0
    if (-1 == xioctl (fd_0, VIDIOC_QBUF, &buf_0)){
        errno_exit ("VIDIOC_QBUF");
        fprintf(stderr,"error");
        }
    #endif // INFRARED_0
    #if INFRARED_1
    if (-1 == xioctl (fd_1, VIDIOC_QBUF, &buf_1)){
        errno_exit ("VIDIOC_QBUF");
        fprintf(stderr,"error");
        }
    #endif // INFRARED_1
    #if VISIBLE_0
    if(pPrevRequest_0)
        pPrevRequest_0->unlock();
    pPrevRequest_0 = pRequest_0;
    pRequest_0 = 0;
    #endif // VISIBLE_0
    #if VISIBLE_1
     if(pPrevRequest_1)
        pPrevRequest_1->unlock();
    pPrevRequest_1 = pRequest_1;
    pRequest_1 = 0;
    #endif // VISIBLE_1
    #if VISIBLE_0
    int result = fi.imageRequestSingle();
    if( result != DMR_NO_ERROR)
        cout << "Error during the request result. " << ImpactAcquireException::getErrorCodeAsString( result ) << endl;
    #endif // VISIBLE_0
    #if VISIBLE_1
    int result2 = fi2.imageRequestSingle();
    if( result2 != DMR_NO_ERROR)
        cout << "Error during the request result. " << ImpactAcquireException::getErrorCodeAsString( result2 ) << endl;
    #endif // VISIBLE_1
	return 1;
}

static void
mainloop(FunctionInterface& fi, FunctionInterface& fi2)
{
	unsigned int count;


	const unsigned int timeout_ms = 500;

	while (1) {
		for (;;) {
            #if VISIBLE_0
			request_0 = INVALID_ID;
			if(!pRequest_0)			{
				request_0 = fi.imageRequestWaitFor(timeout_ms);
				pRequest_0 = fi.isRequestNrValid( request_0 ) ? fi.getRequest( request_0 ) : 0;
			}
			#endif // VISIBLE_0
			#if VISIBLE_1
            request_1 = INVALID_ID;
			if(!pRequest_1)			{
				request_1 = fi2.imageRequestWaitFor(timeout_ms);
				pRequest_1 = fi2.isRequestNrValid( request_1 ) ? fi2.getRequest( request_1 ) : 0;
			}
			#endif // VISIBLE_1

            struct timeval tv;
            int r_0,r_1;
            #if INFRARED_0
			fd_set fds_0;
			FD_ZERO (&fds_0);
			FD_SET (fd_0, &fds_0);
			#endif // INFRARED_0
            #if INFRARED_1
            fd_set fds_1;
			FD_ZERO (&fds_1);
			FD_SET (fd_1, &fds_1);
            #endif // INFRARED_1


			/* Timeout. */
			tv.tv_sec = 2;
			tv.tv_usec = 0;

            #if INFRARED_0
			r_0 = select (fd_0 + 1, &fds_0, NULL, NULL, &tv);
			#endif // INFRARED_0
			#if INFRARED_1
			r_1 = select (fd_1 + 1, &fds_1, NULL, NULL, &tv);
			#endif // INFRARED_1

            #if INFRARED_0
			if (-1 == r_0) {
				if (EINTR == errno)
					continue;

				errno_exit ("select");
			}

			if (0 == r_0) {
				fprintf (stderr, "select timeout 0\n");
				//ioctl (fd, S2253_VIDIOC_DEBUG, msg);
				//fprintf (stderr, "debug: %s\n", msg);
				exit (EXIT_FAILURE);
			}
			#endif // INFRARED_0
			#if INFRARED_1
			if (-1 == r_1) {
				if (EINTR == errno)
					continue;

				errno_exit ("select");
			}

			if (0 == r_1) {
				fprintf (stderr, "select timeout 1\n");
				//ioctl (fd, S2253_VIDIOC_DEBUG, msg);
				//fprintf (stderr, "debug: %s\n", msg);
				exit (EXIT_FAILURE);
			}
			#endif // INFRARED_0
			bool vis_ok = false;
            #if VISIBLE_0
            if (pRequest_0 && pRequest_0->isOK())
                vis_ok = true;
            else
                cout << "error" << endl;
            #endif // VISIBLE_0
            #if VISIBLE_1
            if(pRequest_1 && pRequest_1->isOK())
                vis_ok = true;
            #endif // VISIBLE_1
            #if VISIBLE_0 || VISIBLE_1
			if (vis_ok && read_frame(fi,fi2))
				break;
            #else
            if (read_frame(fi,fi2))
				break;
            #endif // VISIBLE_0


			/* EAGAIN - continue select loop. */
		}
	}
}


static void
stop_capturing(int fd)
{
	enum v4l2_buf_type type;

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fprintf(stderr, "stream off sent\n");
    if (-1 == xioctl (fd, VIDIOC_STREAMOFF, &type))
        perror ("VIDIOC_STREAMOFF");

}

static void
start_capturing_bluefox(FunctionInterface& fi,ImageRequestControl& irc, Device* pDev)
{

	/*** start capturing bluefox camera ***/

	TDMR_ERROR result;
	result = DMR_NO_ERROR;

	while( ( result = static_cast<TDMR_ERROR>(fi.imageRequestSingle(&irc)) ) == DMR_NO_ERROR) {};
	if( result != DEV_NO_FREE_REQUEST_AVAILABLE ){
			cout << "'FunctionInterface.imageRequestSingle' returned with an unexpected result: " << result	<< "(" << ImpactAcquireException::getErrorCodeAsString( result ) << ")" << endl;
	}

	if( pDev->acquisitionStartStopBehaviour.read() == assbUser )
		if( ( result = static_cast<TDMR_ERROR>(fi.acquisitionStart()) ) != DMR_NO_ERROR )
			cout << "'FunctionInterface.acquisitionStart' returned with an unexpected result: " << result << "(" << ImpactAcquireException::getErrorCodeAsString( result ) << ")" << endl;

}

static void
start_capturing(int fd, char* dev_name, buffer* buffers, unsigned int n_buffers)
{
	/*** start capturing v4l2 camera ***/
	unsigned int i;
	enum v4l2_buf_type type;


    for (i = 0; i < n_buffers; ++i) {
        struct v4l2_buffer buf;

        CLEAR (buf);

        buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory      = V4L2_MEMORY_MMAP;
        buf.index       = i;

        if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
            errno_exit ("VIDIOC_QBUF");
    }

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (-1 == xioctl (fd, VIDIOC_STREAMON, &type))
        errno_exit ("VIDIOC_STREAMON");
}

static void
uninit_device (buffer* buffers, unsigned int n_buffers)
{
	unsigned int i;

	for (i = 0; i < n_buffers; ++i)
		if (-1 == munmap (buffers[i].start, buffers[i].length))
			perror ("munmap");

	free (buffers);
}

static void
init_mmap (int fd, char* dev_name, buffer** buffers, unsigned int& n_buffers)
{
	struct v4l2_requestbuffers req;

	CLEAR (req);

	req.count               = 4;
	req.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory              = V4L2_MEMORY_MMAP;


	if (-1 == xioctl (fd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			fprintf (stderr, "%s does not support "
				"memory mapping\n", dev_name);
			exit (EXIT_FAILURE);
		} else {
			errno_exit ("VIDIOC_REQBUFS");
		}
	}

	if (req.count < 2) {
		fprintf (stderr, "Insufficient buffer memory on %s\n",
			dev_name);
		exit (EXIT_FAILURE);
	}

	*buffers = (buffer*)calloc(req.count, sizeof (buffer));

	if (!*buffers) {
		fprintf (stderr, "Out of memory\n");
		exit (EXIT_FAILURE);
	}

	for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
		struct v4l2_buffer buf;

		CLEAR (buf);

		buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory      = V4L2_MEMORY_MMAP;
		buf.index       = n_buffers;

		if (-1 == xioctl (fd, VIDIOC_QUERYBUF, &buf))
			errno_exit ("VIDIOC_QUERYBUF");

		//fprintf(stderr, "buf.length = %d\n", buf.length);

		(*buffers)[n_buffers].length = buf.length;
		(*buffers)[n_buffers].start =
			mmap (NULL /* start anywhere */,
			      buf.length,
			      PROT_READ | PROT_WRITE /* required */,
			      MAP_SHARED /* recommended */,
			      fd, buf.m.offset);

		if (MAP_FAILED == (*buffers)[n_buffers].start)
			errno_exit ("mmap");
	}
}

static void
init_device (int fd, char* dev_name, buffer** buffers, unsigned int& n_buffers)
{
	/*** init v4l2 camera ***/
	struct v4l2_capability cap_0;
	struct v4l2_cropcap cropcap_0;
	struct v4l2_crop crop_0;
	struct v4l2_format fmt_0;
	unsigned int min;

	if (-1 == xioctl (fd, VIDIOC_QUERYCAP, &cap_0)) {
		if (EINVAL == errno) {
			fprintf (stderr, "%s is no V4L2 device\n",
				dev_name);
			exit (EXIT_FAILURE);
		} else {
			errno_exit ("VIDIOC_QUERYCAP");
		}
	}

	if (!(cap_0.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		fprintf (stderr, "%s is no video capture device\n",
			dev_name);
		exit (EXIT_FAILURE);
	}

	{
		int index = 0;

		if (-1 == ioctl (fd, VIDIOC_S_INPUT, &index)) {
			perror ("VIDIOC_S_INPUT");
			exit (EXIT_FAILURE);
		}
	}

	{
		struct v4l2_input input;
		int current;

		if (-1 == ioctl (fd, VIDIOC_G_INPUT, &current)) {
		        perror ("VIDIOC_G_INPUT");
		}

		memset (&input, 0, sizeof (input));
		input.index = current;

		if (-1 == ioctl (fd, VIDIOC_ENUMINPUT, &input)) {
		        perror ("VIDIOC_ENUMINPUT");
		} else  {
			fprintf (stderr, "Current input: %s\n", input.name);

        if (input.status & V4L2_IN_ST_NO_H_LOCK)
            fprintf (stderr, "Warning: no video lock detected\n");
		}
	}


	CLEAR (cropcap_0);

        cropcap_0.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (0 == xioctl (fd, VIDIOC_CROPCAP, &cropcap_0)) {
                crop_0.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                crop_0.c = cropcap_0.defrect; /* reset to default */

                if (-1 == xioctl (fd, VIDIOC_S_CROP, &crop_0)) {
                        switch (errno) {
                        case EINVAL:
                                /* Cropping not supported. */
                                break;
                        default:
                                /* Errors ignored. */
                                break;
                        }
                }
        } else {
                /* Errors ignored. */
        }


    CLEAR (fmt_0);
    fmt_0.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    fmt_0.fmt.pix.width =  640;
    fmt_0.fmt.pix.height = 480;

    fmt_0.fmt.pix.pixelformat = V4L2_PIX_FMT_GREY;
    fmt_0.fmt.pix.field       = V4L2_FIELD_ANY;


    if (-1 == xioctl (fd, VIDIOC_S_FMT, &fmt_0))
            errno_exit ("VIDIOC_S_FMT");


	{
		struct v4l2_streamparm parm_0;

		CLEAR(parm_0);
		parm_0.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (0 == xioctl(fd, VIDIOC_G_PARM, &parm_0)) {
			parm_0.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			parm_0.parm.capture.timeperframe.numerator = (1001);
			parm_0.parm.capture.timeperframe.denominator = 30000;
			fprintf(stderr, "Time per frame: %u/%u\n",
				parm_0.parm.capture.timeperframe.numerator,
				parm_0.parm.capture.timeperframe.denominator);
			if (0 != xioctl(fd, VIDIOC_S_PARM, &parm_0))
				errno_exit ("VIDIOC_S_PARM");
		}
	}

    init_mmap (fd,dev_name,buffers,n_buffers);
}
static void
init_bluefox(FunctionInterface& fi_, ImageRequestControl& irc_, Device* pDev_, std::string name){

	/*** init bluefox camera ***/

	fi_.createSetting(name);

	SettingsBlueFOX setting( pDev_, name );

	setting.cameraSetting.aoiWidth.write(WEIGHT);
	setting.cameraSetting.aoiHeight.write(HEIGHT);
	setting.cameraSetting.aoiStartX.write(START);
	setting.cameraSetting.aoiStartY.write(0);
	setting.imageDestination.pixelFormat.write(idpfMono8);


	mvIMPACT::acquire::CameraSettingsBlueFOX cs(pDev_);
//	#if VISIBLE_0 && INFRARED_1
//	TBoolean b(bTrue);
//	cs.getHDRControl().HDREnable.write(b);
//	setting.cameraSetting.getHDRControl().HDREnable.write(b);
//	setting.cameraSetting.getHDRControl().HDRMode.write(cHDRmFixed0);
//	setting.cameraSetting.autoExposeControl.write(aecOff);
//	setting.cameraSetting.autoGainControl.write(agcOff);
////    #else
    setting.cameraSetting.autoExposeControl.write(aecOn);
	setting.cameraSetting.autoGainControl.write(agcOn);
	cs.autoExposeControl.write(aecOn);
	cs.autoGainControl.write(agcOn);
//	#endif // VISIBLE_0

    irc_.setting.writeS(name);


//	cs.autoExposeControl.write(aecOn);
//	cs.autoGainControl.write(agcOn);

}

static void
close_device                    (int fd)
{
	if (-1 == close (fd))
		perror ("close");

	fd = -1;
}

static void
open_device                     (void)
{

	/**** opencv v4l2 camera ****/
	#if INFRARED_0
	struct stat st;
	fprintf(stderr,"dev_name \n %s \n", dev_name_0);

	if (-1 == stat (dev_name_0, &st)) {
		fprintf (stderr, "Cannot identify '%s': %d, %s\n",
			dev_name_0, errno, strerror (errno));
		exit (EXIT_FAILURE);
	}

	if (!S_ISCHR (st.st_mode)) {
		fprintf (stderr, "%s is no device\n", dev_name_0);
		exit (EXIT_FAILURE);
	}

	fd_0 = open(dev_name_0, O_RDWR | O_NONBLOCK, 0);

	if (-1 == fd_0) {
		fprintf (stderr, "Cannot open '%s': %d, %s\n",
			dev_name_0, errno, strerror (errno));
		exit (EXIT_FAILURE);
	}
	#endif // INFRARED_0

	#if INFRARED_1
        #if !INFRARED_0
        struct stat st;
        #endif
	fprintf(stderr,"dev_name \n %s \n", dev_name_1);

	if (-1 == stat (dev_name_1, &st)) {
		fprintf (stderr, "Cannot identify '%s': %d, %s\n",
			dev_name_1, errno, strerror (errno));
		exit (EXIT_FAILURE);
	}

	if (!S_ISCHR (st.st_mode)) {
		fprintf (stderr, "%s is no device\n", dev_name_1);
		exit (EXIT_FAILURE);
	}

	fd_1 = open(dev_name_1, O_RDWR | O_NONBLOCK, 0);

	if (-1 == fd_1) {
		fprintf (stderr, "Cannot open '%s': %d, %s\n",
			dev_name_1, errno, strerror (errno));
		exit (EXIT_FAILURE);
	}

	#endif // INFRARED_1

	/*** open mv_bluefox camera ***/
	#if VISIBLE_0
	try{
		pDev_0->open();
	}
	catch( const ImpactAcquireException& e1 ){
			cout << "An error occurred while opening the device " << pDev_0->serial.read()
				<< "(error code: " << e1.getErrorCodeAsString() << "). Press [ENTER] to end the application..." << endl;
			cin.get();
			exit (EXIT_FAILURE);
		}
    #endif // VISIBLE_0
    #if VISIBLE_1
	try{
		pDev_1->open();
	}
	catch( const ImpactAcquireException& e1 ){
			cout << "An error occurred while opening the device " << pDev_1->serial.read()
				<< "(error code: " << e1.getErrorCodeAsString() << "). Press [ENTER] to end the application..." << endl;
			cin.get();
			exit (EXIT_FAILURE);
		}
    #endif // VISIBLE_1

}

static void
quit_handler(int sig)
{
	exit(EXIT_SUCCESS);
}

static void
pause_handler(int sig)
{

}

static void cleanup(void)
{
	#if INFRARED_0
	stop_capturing (fd_0);
	uninit_device (buffers_0,n_buffers_0);
    close(fd_0);
    #endif // INFRARED_0
    #if INFRARED_1
    stop_capturing (fd_1);
	uninit_device (buffers_1,n_buffers_1);
    close(fd_1);
    #endif // INFRARED_1

	fprintf(stderr, "demo closing files... ");
	fflush(stderr);
	image_data_file.close();

	fprintf(stderr, "done.\n");
}



int main(int argc, char ** argv)
{
    // handling signals
	signal(SIGINT, quit_handler);
	signal(SIGTERM, quit_handler);
	signal(SIGUSR1, pause_handler);

	DeviceManager devMgr;
	#if VISIBLE_0
    pDev_0 = devMgr.getDeviceBySerial(to_string(VISIBLE_0)); //opening bluefox camera

    if( !pDev_0 )
	{	cout << "Unable to open mvBlueFOX!";
		cout << "Press [ENTER] to end the application" << endl;
		cin.get();
		return -1;
	}
    #endif // VISIBLE_0
    #if VISIBLE_1
    pDev_1 = devMgr.getDeviceBySerial(to_string(VISIBLE_1)); //opening bluefox camera

    if( !pDev_1 )
	{	cout << "Unable to open mvBlueFOX!";
		cout << "Press [ENTER] to end the application" << endl;
		cin.get();
		return -1;
	}
    #endif // VISIBLE_1

    #if INFRARED_0
    string cam_0 = "/dev/video"+to_string(INFRARED_0); //retreiving camera name
    dev_name_0 = const_cast<char*>(cam_0.c_str());
    #endif // INFRARED_0
    #if INFRARED_1
    string cam_1 = "/dev/video"+to_string(INFRARED_1); //retreiving camera name
    dev_name_1 = const_cast<char*>(cam_1.c_str());
    #endif // INFRARED_1




	/**** XSens for GPS ****/

//	mtPort = XsPortInfo("/dev/ttyUSB0", XsBaud::numericToRate(115200));
//	if (!device.openPort(mtPort))
//			throw std::runtime_error("Could not open port. Aborting.");
//    if (!device.gotoConfig())
//        throw std::runtime_error("Could not put device into configuration mode. Aborting.");
//
//    mtPort.setDeviceId(device.getDeviceId());
//    if (!mtPort.deviceId().isMt9c() && !mtPort.deviceId().isLegacyMtig() && !mtPort.deviceId().isMtMk4() && !mtPort.deviceId().isFmt_X000())
//        throw std::runtime_error("No MTi / MTx / MTmk4 device found. Aborting.");
//
//    std::cout << "Device: " << device.getProductCode().toStdString() << " opened." << std::endl;
//    std::cout << "Configuring the device..." << std::endl;
//    if (mtPort.deviceId().isMt9c() || mtPort.deviceId().isLegacyMtig())
//    {
//        XsOutputMode outputMode = XO // output orientation data
//        XsOutputSettings outputSettings = XOS_OrientationMode_Quaternion; // output orientation data as quaternion
//
//        // set the device configuration
//        if (!device.setDeviceMode(outputMode, outputSettings))
//        {
//            throw std::runtime_error("Could not configure MT device. Aborting.");
//        }
//    }
//    else if (mtPort.deviceId().isMtMk4() || mtPort.deviceId().isFmt_X000())
//    {
//        XsOutputConfiguration quat(XDI_Quaternion, 100);
//        XsOutputConfigurationArray configArray;
//        configArray.push_back(quat);
//        if (!device.setOutputConfiguration(configArray))
//        {
//
//            throw std::runtime_error("Could not configure MTmk4 device. Aborting.");
//        }
//    }
//    else
//    {
//        throw std::runtime_error("Unknown device while configuring. Aborting.");
//    }
//
//    if (!device.gotoMeasurement())
//        throw std::runtime_error("Could not put device into measurement mode. Aborting.");

/************************************************************************************************/

	cv::namedWindow("left",0);
	cv::namedWindow("right",0);
	open_device ();
	std::string filename = dir+"image_data.txt";
	image_data_file.open(filename.c_str(), std::ios_base::trunc);

    #if VISIBLE_0
	Statistics statistics_0(pDev_0);
	FunctionInterface fi_0(pDev_0);
    ImageRequestControl irc_0(pDev_0);
    #endif // VISIBLE_0
    #if VISIBLE_1
    Statistics statistics_1(pDev_1);
	FunctionInterface fi_1(pDev_1);
    ImageRequestControl irc_1(pDev_1);
    #endif // VISIBLE_1

    #if INFRARED_0
	init_device (fd_0,dev_name_0,&buffers_0,n_buffers_0);
	#endif // INFRARED_0
	#if INFRARED_1
	init_device(fd_1,dev_name_1,&buffers_1,n_buffers_1);
	cout << buffers_1[0].start << endl;
	#endif // INFRARED_1
	#if VISIBLE_0
	init_bluefox(fi_0,irc_0,pDev_0,"mvBlueFox1");
	#endif // VISIBLE_0
	#if VISIBLE_1
	init_bluefox(fi_1,irc_1,pDev_1,"mvBlueFox2");
	#endif // VISIBLE_1

	atexit(cleanup);

    #if INFRARED_0
    start_capturing (fd_0,dev_name_0,buffers_0,n_buffers_0);
    #endif // INFRARED_0
    #if INFRARED_1
    start_capturing(fd_1,dev_name_1,buffers_1,n_buffers_1);
    #endif // INFRARED_1
    #if VISIBLE_0
    start_capturing_bluefox(fi_0,irc_0,pDev_0);
    #endif // VISIBLE_0
    #if VISIBLE_1
    start_capturing_bluefox(fi_1,irc_1,pDev_1);
    #endif // VISIBLE_1

//    FunctionInterface generic(0);
    #if VISIBLE_0 && VISIBLE_1
	mainloop (fi_0,fi_1);
	#elif VISIBLE_0
	mainloop (fi_0,fi_0);
	#elif VISIBLE_1
	mainloop(fi_1,fi_1);
	#else
	Device* pDev_0 = devMgr.getDeviceBySerial("26802713");
	FunctionInterface fi_0(pDev_0);
	mainloop(fi_0,fi_0);
	#endif // VISIBLE_0

    return 0;
}
