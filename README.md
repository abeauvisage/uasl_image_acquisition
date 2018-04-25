# uasl_image_acquisition
Program for acquiring images from one or several bluefox/flir cameras.


DEPENDENCIES:
- OpenCV >= 12.4.8  http://opencv.org/downloads.html
- Sensoray 2253 driver >= 1.2.14 (for flir cameras) http://www.sensoray.com/products/2253.htm
- mvBlueFOX SDK >= 2.17.2 (for MV cameras)  https://www.matrix-vision.com/USB2.0-industrial-camera-mvbluefox.html

OPTIONAL DEPENDENCIES:
- ROS (tested with indigo and above). The program can be placed directly into your catkin workspace, it will by default compile with ROS compatibility, so you do not need to add the BUILD_ROS_NODE (see below) flag to work with catkin.

CMAKE FLAGS:
Mandatory argument: 
- MVBLUEFOX_TOP_LEVEL_PATH : Path to the mvImpact_acquire folder (e.g. /home/user/Libraries/mvIMPACT_acquire-x86_64-2.17.3/". If the library has been installed in the system folders, MVBLUEFOX_LIB_PATH and MVBLUEFOX_INCLUDE_PATH can be used to point toward the path to the libraries and headers respectively. Please note that at that time, only the path to the 64 bits libraries is included.

Optional argument :
-BUILD_ROS_NODE : if true, the ROS node will be built (thus you need ROS). The default is true.

INSTALLATION:
- install every dependencies (refer to the links above)
- mkdir build && cd build
- cmake -DMVBLUEFOX_TOP_LEVEL_PATH="/home/user/Libraries/mvIMPACT_acquire-x86_64-2.17.3/" -DBUILD_ROS_NODE=false ..

EXAMPLES :
- Look at the "examples" folder for examples on how to use the camera classes. The "test" folder contains test programs that you can use to test the behaviour of the program. Make sure to change the ID of the camera to yours before compiling and running.

