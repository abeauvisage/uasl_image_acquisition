/*** cropping for infrared ***/
#define WIDTH 640//   1280
#define HEIGHT 480//   1024
#define START 56

#include "configWindow.h"
#include "imageHandler.h"

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
#include <deque>
#include <thread>
#include <iterator>

#include <time.h>
#include <chrono>
#include <signal.h>

#include "opencv2/opencv.hpp"

/*** bluefox ***/
#include "apps/Common/exampleHelper.h"
#include <mvIMPACT_CPP/mvIMPACT_acquire.h>
#include <mvIMPACT_CPP/mvIMPACT_acquire_GenICam.h>


#define CLEAR(x) memset (&(x), 0, sizeof (x))


using namespace std;
using namespace cv;
using namespace mvIMPACT::acquire;

std::string dir; // directory where to save images
std::ofstream image_data_file;


/*** global variables for each camera ***/

vector<string> inf_names;
vector<int> inf_cams;
vector<Device*> pDev;
vector<FunctionInterface> fi;
vector<ImageHandler> imgHBluefox;
vector<ImageHandler> imgHinf;
vector<thread> threads;
vector<pair<int,double>> timestamps;

unsigned int img_nb=1;

struct buffer {
	void *		start;
	size_t		length;
};

//struct buffer *		buffers_1		= NULL;
static unsigned int	n_buffers	= 5;

vector<buffer*> buffers;

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

void get_infra_frame(const int fd, buffer* buff, cv::Mat& img){

    struct v4l2_buffer buf;
    CLEAR (buf);

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if (-1 == xioctl (fd, VIDIOC_QBUF, &buf)){
        cerr << " query VIDIOC_QBUF" << endl;
    }

    fd_set fds;
    FD_ZERO (&fds);
    FD_SET (fd, &fds);
    /* Timeout. */
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 40000;
    int r= select (fd + 1, &fds, NULL, NULL, &tv);

    if (-1 == r) {
        if (EINTR == errno)
            return;

        errno_exit ("select");
    }
    if (r == 0)
        cerr << " [Error] select timeout " << endl;
    CLEAR (buf);

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if (-1 == xioctl (fd, VIDIOC_DQBUF, &buf)) {
        switch (errno) {
        case EAGAIN:
            return;
        case EIO:
        default:
            fprintf(stderr,"error");
            errno_exit ("VIDIOC_DQBUF");
        }
    }
    if (r == 0)
        return;
    assert (buf.index < n_buffers);

    cv::Mat img_(480, 640, CV_8UC1,buff[buf.index].start);
    img_.copyTo(img);
}

void send_request_bluefox(const FunctionInterface& fi){

    int result;
    bool ready=false;
    for(uint i=0;i<fi.requestCount();i++)
        if(fi.getRequest(i)->requestState.read() != TRequestState::rsWaiting)
            ready = true;

    if(ready)
        result = fi.imageRequestSingle();
    else
	return ;

    if( result != DMR_NO_ERROR){
        cerr << "Error obtaining the request. " << ImpactAcquireException::getErrorCodeAsString( result ) << endl;
    }
}

void get_bluefox_frame(const FunctionInterface& fi, cv::Mat& img){

    int timeout_ms=300;

    int req = INVALID_ID;
    req = fi.imageRequestWaitFor(timeout_ms);
    Request* request = fi.isRequestNrValid( req ) ? fi.getRequest( req ) : 0;

    if (!request){
        cerr << "Failed to retrieve image: TIMEOUT" << endl;
    }
    else if(!request->isOK())
        cerr << "Failed to retrieve image: NOT OK" << endl;
    else{
        ImageBuffer *pib =  request->getImageBufferDesc().getBuffer();
        Mat img_(pib->iHeight,pib->iWidth,CV_8UC1,pib->vpData);
        img_.copyTo(img);
        request->unlock();
    }
}

static void mainloop()
{

	while (1) {

            	bool img_ok = true;
		vector<Mat> imgs_inf(inf_cams.size());
		vector<Mat> imgs_bluefox(fi.size());
		vector<thread> acquisition_threads;

            for(uint i=0;i<inf_cams.size();i++)
                get_infra_frame(inf_cams[i],buffers[i],imgs_inf[i]);

	     for(uint i=0;i<fi.size();i++){
                get_bluefox_frame(fi[i],imgs_bluefox[i]);
		send_request_bluefox(fi[i]);
	     }

	     for(uint i=0;i<imgs_bluefox.size();i++)
                if(imgs_bluefox[i].empty())
                    img_ok = false;

            for(uint i=0;i<imgs_inf.size();i++)
                if(imgs_inf[i].empty())
                    img_ok = false;

	    auto stamp = std::chrono::steady_clock::now();
            if(img_ok){
		timestamps.push_back(pair<int,double>(img_nb,stamp.time_since_epoch().count()));

                for(uint i=0;i<fi.size();i++){
                    imgHBluefox[i].addImage(pair<int,Mat>(img_nb,imgs_bluefox[i]));
                }
                for(uint i=0;i<inf_cams.size();i++){
                    imgHinf[i].addImage(pair<int,Mat>(img_nb,imgs_inf[i]));
                }
                img_nb++;
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
start_capturing(int fd, unsigned int n_buffers)
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
            errno_exit ("INIT VIDIOC_QBUF");
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
init_mmap (int fd, string dev_name, buffer** buffers, unsigned int& n_buffers)
{
	struct v4l2_requestbuffers req;

	CLEAR (req);

	req.count               = n_buffers;
	req.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory              = V4L2_MEMORY_MMAP;


	if (-1 == xioctl (fd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			fprintf (stderr, "%s does not support "
				"memory mapping\n", dev_name.c_str());
			exit (EXIT_FAILURE);
		} else {
			errno_exit ("VIDIOC_REQBUFS");
		}
	}

	if (req.count < n_buffers) {
		fprintf (stderr, "Insufficient buffer memory on %s\n",
			dev_name.c_str());
		exit (EXIT_FAILURE);
	}

	*buffers = (buffer*)calloc(req.count, sizeof (buffer));

	if (!*buffers) {
		fprintf (stderr, "Out of memory\n");
		exit (EXIT_FAILURE);
	}

	for (unsigned int n = 0; n < req.count; ++n) {
		struct v4l2_buffer buf;

		CLEAR (buf);

		buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory      = V4L2_MEMORY_MMAP;
		buf.index       = n;

		if (-1 == xioctl (fd, VIDIOC_QUERYBUF, &buf))
			errno_exit ("VIDIOC_QUERYBUF");

		(*buffers)[n].length = buf.length;
		(*buffers)[n].start =
			mmap (NULL /* start anywhere */,
			      buf.length,
			      PROT_READ | PROT_WRITE /* required */,
			      MAP_SHARED /* recommended */,
			      fd, buf.m.offset);

		if (MAP_FAILED == (*buffers)[n].start)
			errno_exit ("mmap");
	}
}

static void
init_device (int fd, string dev_name, buffer** buffers, unsigned int& n_buffers)
{
	/*** init v4l2 camera ***/
	struct v4l2_capability cap_0;
	struct v4l2_cropcap cropcap_0;
	struct v4l2_crop crop_0;
	struct v4l2_format fmt_0;

	if (-1 == xioctl (fd, VIDIOC_QUERYCAP, &cap_0)) {
		if (EINVAL == errno) {
			std::cerr << dev_name << " is no V4L2 device" << endl;
			exit (EXIT_FAILURE);
		} else {
			errno_exit ("VIDIOC_QUERYCAP");
		}
	}

	if (!(cap_0.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		std::cerr << dev_name << " is no video capture device" << std::endl;
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
			std::cerr << "Current input: " << input.name << std::endl;

        if (input.status & V4L2_IN_ST_NO_H_LOCK)
            std::cerr << "Warning: no video lock detected" << std::endl;
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
			std::cout << "Time per frame: " << parm_0.parm.capture.timeperframe.numerator << "/" <<
				parm_0.parm.capture.timeperframe.denominator << std::endl;
			if (0 != xioctl(fd, VIDIOC_S_PARM, &parm_0))
				errno_exit ("VIDIOC_S_PARM");
		}
	}

    init_mmap (fd,dev_name,buffers,n_buffers);
}

/*static void
close_device                    (int fd)
{
	if (-1 == close (fd))
		perror ("close");

	fd = -1;
}*/

static void open_devices()
{

	/**** opencv v4l2 cameras ****/
    struct stat st;
    for(string cam_name : inf_names){
        cout << "Device : " << cam_name << endl;

        if (-1 == stat (cam_name.c_str(), &st)) {
            cerr <<  "Cannot identify " << cam_name << ": " << errno << ", " << strerror (errno) << endl;
            exit (EXIT_FAILURE);
        }

        if (!S_ISCHR (st.st_mode)) {
            cerr << cam_name << " is no device\n" << cam_name << endl;
            exit (EXIT_FAILURE);
        }

        int fd = open(cam_name.c_str(), O_RDWR | O_NONBLOCK, 0);

        if (-1 == fd) {
            cerr << "Cannot open " << cam_name << ": " << errno << ", " << strerror (errno) << endl;
            exit (EXIT_FAILURE);
        }else
            inf_cams.push_back(fd);

    }

    assert(inf_cams.size() == inf_names.size());

	/*** open mv_bluefox camera ***/
	for(Device* device : pDev){
        cout << "Device : " << device->serial.read() << endl;
        try{device->open();}
        catch( const ImpactAcquireException& e1 ){
            cout << "An error occurred while opening the device " << device->serial.read()
                << "(error code: " << e1.getErrorCodeAsString() << "). Press [ENTER] to end the application..." << endl;
            cin.get();
            exit (EXIT_FAILURE);
        }
	}
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

static void
start_capturing_bluefox(const FunctionInterface& fi)
{

        int result = fi.imageRequestSingle();
        result = fi.imageRequestSingle();
        result = fi.imageRequestSingle();
        if( result != DMR_NO_ERROR){
            cerr << "[init] Error during the request result. " << ImpactAcquireException::getErrorCodeAsString( result ) << endl;
        }
}

static void cleanup(void)
{
    cerr << "[Warning] stopping threads" << endl;
    for(uint i=0;i<imgHBluefox.size();i++)
        imgHBluefox[i].stop();
    for(uint i=0;i<imgHinf.size();i++)
        imgHinf[i].stop();

    cerr << "[Warning] saving image timestamps" << endl;
    for(auto pair : timestamps)
		image_data_file << pair.first << "," << pair.second << ";" << std::endl;
    image_data_file.close();

    cerr << "[Warning] closing cameras" << endl;
    for(uint i=0;i<inf_cams.size();i++){
        stop_capturing (inf_cams[i]);
        uninit_device(buffers[i],n_buffers);
        close(inf_cams[i]);
    }

    for(uint i=0;i<pDev.size();i++)
        pDev[i]->close();
    cerr << "done." << endl;
}

int main(int argc, char ** argv)
{
    // handling signals
	signal(SIGINT, quit_handler);
	signal(SIGTERM, quit_handler);
	signal(SIGUSR1, pause_handler);
	#ifdef GTK3_FOUND
	gtk_init(&argc,&argv);
	#endif

	if(argc < 2){
        cerr << "no directory specified" << endl;
        exit(-1);
	}
	else if(argc < 3){
        cerr << "no camera specified" << endl;
        exit(-1);
	}

	dir = argv[1];
	std::cout << "saving images in: " << dir << std::endl;

	DeviceManager devMgr;
	for(int i=2;i<argc;i++){
        string  cam_name = argv[i];
        cout << "found: " << cam_name << endl;
        if(cam_name.find("/dev/video") != string::npos){
             inf_names.push_back(cam_name);
        }
        else{
            Device* dev = devMgr.getDeviceBySerial(cam_name); //opening bluefox camera
            if(!dev){
                cout << "Unable to open mvBlueFOX: " << cam_name << endl;;
                exit(-1);
            }
            pDev.push_back(dev);
        }
	}

	cout << "Total nb of camera found: " << pDev.size() +inf_names.size() << endl;

    atexit(cleanup);

/************************************************************************************************/
	open_devices();
	std::string filename = dir+"/image_data.csv";
	image_data_file.open(filename.c_str(), std::ios_base::trunc);

	imgHBluefox = vector<ImageHandler>(pDev.size());

	for(uint i=0;i<pDev.size();i++){
        fi.push_back(FunctionInterface(pDev[i]));
        imgHBluefox[i].configure(dir,"cam"+to_string(i),ImageHandler::Action::SHOW);
        threads.push_back(std::thread(&ImageHandler::processImages, &(imgHBluefox[i])));
        threads[i].detach();
	start_capturing_bluefox(fi[i]);
	}

	imgHinf = vector<ImageHandler>(inf_names.size());
	for(uint i=0;i<inf_cams.size();i++){
	buffers.push_back(NULL);
        init_device(inf_cams[i],inf_names[i],&(buffers[i]),n_buffers);
        imgHinf[i].configure(dir,"cam"+to_string(i+pDev.size()),ImageHandler::Action::SHOW);
        threads.push_back(std::thread(&ImageHandler::processImages, &(imgHinf[i])));
        threads[i+pDev.size()].detach();
        start_capturing(inf_cams[i],n_buffers);
	}

	ConfigWindow conf;
    	conf.setPort("/dev/ttyUSB0");
	#ifdef GTK3_FOUND
	conf.createGUI();
	#endif
	
	mainloop();

    return 0;
}
