# UASL image acquisition
Program for acquiring images from one or several bluefox/flir cameras.

## Installation

### Dependencies :
- [OpenCV] (http://opencv.org/downloads.html) >= 12.4.8  (Image processing library)

### Optional dependencies :
- [Sensoray 2253 driver](http://www.sensoray.com/products/2253.htm) >= 1.2.14 (for tau2 legacy code) 
- [mvBlueFOX SDK](https://www.matrix-vision.com/USB2.0-industrial-camera-mvbluefox.html) >= 2.17.2 (for MV cameras)
- [ROS] (tested with indigo and above). The program can be placed directly into your catkin workspace, it will by default compile with ROS compatibility, so you do not need to add the BUILD_ROS_NODE (see below) flag to work with catkin.
- [GTK+ 3] for configuring tau2 cameras with sensoray frame grabbers.

### CMake flags :
Mandatory argument: 
- MVBLUEFOX_TOP_LEVEL_PATH : Path to the mvImpact_acquire folder (e.g. /home/user/Libraries/mvIMPACT_acquire-x86_64-2.17.3/". If the library has been installed in the system folders, MVBLUEFOX_LIB_PATH and MVBLUEFOX_INCLUDE_PATH can be used to point toward the path to the libraries and headers respectively. Please note that at that time, only the path to the 64 bits libraries is included.

Optional argument :
- BUILD_ROS_NODE : if true, the ROS node will be built (thus you need ROS). The default is true.
- TAU2_DRIVER : compile libthermallibrary to use tau2 camera with TEAX frame grabbers.
- TAU2_LEGACY_CODE : compile multispectral acquisition with legacy code for tau2 sensoray frame grabbers .

### Example of installation workflow :
- install every dependencies (refer to the links above)
- mkdir build && cd build
- cmake -DMVBLUEFOX_TOP_LEVEL_PATH="/home/user/Libraries/mvIMPACT_acquire-x86_64-2.17.3/" -DBUILD_ROS_NODE=false ..

## Usage
### Examples :
- Look at the "examples" folder for examples on how to use the camera classes. The "test" folder contains test programs that you can use to test the behaviour of the program. Make sure to change the ID of the camera to yours before compiling and running.


### Trigger:

1. Copy trigger/95-ftdi-trigger.rules to udev rules directory if using ftdi usb converter for trigger (or trinket pro):
	cp trigger/95-ftdi-trigger.rules /etc/udev/rules.d/

2. Reload udev rules
	udevadm control --reload

3. plug trigger and check if it is detected
	ls /dev
	ttyTRIGGER should appear

4. Run stereo_example (Don't forget to replace the camera serials/types with your cameras)

/!\ if using Tau2 making sure the camera is configured in Continious mode, trigger won't work in slave mode. (normally it is already configured in the class so it shouldn't be a problem).
