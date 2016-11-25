# image-acquisition
program for acquiring images from one or several bluefox/flir cameras.


DEPENDENCIES:
- OpenCV >= 12.4.8  http://opencv.org/downloads.html
- Sensoray 2253 driver >= 1.2.14 (for flir cameras) http://www.sensoray.com/products/2253.htm
- mvBlueFOX SDK >= 2.17.2 (for MV cameras)  https://www.matrix-vision.com/USB2.0-industrial-camera-mvbluefox.html

INSTALLATION:
- install every dependencies (refer to the links above)
- run makefile

COMPILE:
Path to the header files should be known.
The following libraries should be linked:
- libmvBlueFOX
- libmvDeviceManager
- libmvPropHandling
- libopencv_core
- libopencv_highgui

CMAKE:
Mandatory argument: 
- MVBLUEFOX_TOP_LEVEL_PATH : Path to the mvImpact_acquire folder (e.g. /home/user/Libraries/mvIMPACT_acquire-x86_64-2.17.3/". If the library has been installed in the system folders, MVBLUEFOX_LIB_PATH and MVBLUEFOX_INCLUDE_PATH can be used to point toward the path to the libraries and headers respectively. Please note that at that time, only the path to the 64 bits libraries is included.

Typical usage : 
- mkdir build && cd build
- cmake -DMVBLUEFOX_TOP_LEVEL_PATH="/home/user/Libraries/mvIMPACT_acquire-x86_64-2.17.3/" ..
- make

Code examples :
- Look at the "examples" folder for examples on how to use the camera classes.

TODO:
- create makefile for the program to be compiled on linux :)
