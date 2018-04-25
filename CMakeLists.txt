cmake_minimum_required(VERSION 2.8) 
PROJECT(uasl_image_acquisition)

add_definitions(-std=c++11)

find_package(OpenCV REQUIRED)#Depend on your version of OpenCV and ROS installed. If your cv_bridge package relies on OpenCV 2, find_package(OpenCV 2 REQUIRED) is what you need. Else, find_package(OpenCV REQUIRED) will automatically look for OpenCV3.
find_package(Threads REQUIRED)

option(BUILD_ROS_NODE "Build ROS node" ON)
if(BUILD_ROS_NODE)
  ADD_DEFINITIONS(-DBUILD_ROS_NODE)
endif(BUILD_ROS_NODE)

if(BUILD_ROS_NODE)
	find_package(catkin REQUIRED COMPONENTS roscpp cv_bridge image_transport)
	catkin_package(
		  DEPENDS OpenCV
		  CATKIN_DEPENDS roscpp image_transport cv_bridge
		  INCLUDE_DIRS include
		  LIBRARIES bluefox_acq trigger acq_seq bluefox_acq#External libraries created by this package
	)
	include_directories(${catkin_INCLUDE_DIRS})
endif(BUILD_ROS_NODE)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
endif()

set(SRC_ACQUISITION
	capture.cpp
)

set(SRC_BLUEFOX_ACQ
    src/camera_mvbluefox.cpp
)

set(SRC_ACQ
		src/acquisition.cpp
)

#mvBLueFOX libraries (64 bits)
set(MVBLUEFOX_TOP_LEVEL_PATH_CACHE ${MVBLUEFOX_TOP_LEVEL_PATH} CACHE PATH "Path to the whole mvImpact_acquire folder." FORCE)
set(MVBLUEFOX_LIB_PATH ${MVBLUEFOX_TOP_LEVEL_PATH_CACHE}/lib/x86_64 CACHE PATH "Path the the lib folder of the mvBlueFOX driver." FORCE)
set(MVBLUEFOX_INCLUDE_PATH ${MVBLUEFOX_TOP_LEVEL_PATH_CACHE}/mvIMPACT_CPP ${MVBLUEFOX_TOP_LEVEL_PATH_CACHE} ${MVBLUEFOX_TOP_LEVEL_PATH_CACHE}/include/mvIMPACT_CPP ${MVBLUEFOX_TOP_LEVEL_PATH}/include CACHE PATH "Path the the header folder of the mvBlueFOX driver." FORCE)

	
find_library(MVDEVICEMANAGER_LIBRARY 
	NAMES mvDeviceManager
	HINTS ${MVBLUEFOX_LIB_PATH})#Only the mvDeviceManager library needs to be linked
	
find_library(MVLIBUSB1_LIBRARY
	NAMES libusb-1.0.so.0
	HINTS ${MVBLUEFOX_LIB_PATH} 
	NO_DEFAULT_PATH)#Explicitly link the libusb to be sure to use this one
	
find_library(LIBSERIAL_LIBRARY
	NAMES libserial.so.0
)#Explicitly link the libusb to be sure to use this one

include_directories(
  include/uasl_image_acquisition
  ${OpenCV_INCLUDE_DIRS}
  ${MVBLUEFOX_INCLUDE_PATH}
)

#Trigger code
add_library(trigger src/trigger.cpp)

add_library(acq_seq ${SRC_ACQ})
target_link_libraries(acq_seq trigger ${OpenCV_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

if(MVDEVICEMANAGER_LIBRARY AND MVLIBUSB1_LIBRARY)
target_compile_definitions(acq_seq PRIVATE BLUEFOX_FOUND)

if(LIBSERIAL_LIBRARY)
add_library(tau2 src/cameraTau2.cpp)
target_link_libraries(tau2 serial)

add_executable(acquisition_old_board ${SRC_ACQUISITION})
target_link_libraries(acquisition_old_board ${MVDEVICEMANAGER_LIBRARY} ${MVLIBUSB1_LIBRARY} ${OpenCV_LIBRARIES} tau2)
else(LIBSERIAL_LIBRARY)
message(WARNING "LibSerial not found, libraries related to this module will be ignored.")
endif(LIBSERIAL_LIBRARY)

add_library(bluefox_acq ${SRC_BLUEFOX_ACQ})
target_link_libraries(bluefox_acq ${MVDEVICEMANAGER_LIBRARY} ${MVLIBUSB1_LIBRARY} ${OpenCV_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

#Testing script
add_executable(test_bluefox_framerate test/test_bluefox_framerate.cpp)
target_link_libraries(test_bluefox_framerate acq_seq bluefox_acq)

add_executable(test_bluefox_image_format test/test_bluefox_image_format.cpp)
target_link_libraries(test_bluefox_image_format acq_seq bluefox_acq)

#Example code
add_executable(example_bluefox examples/example_bluefox.cpp)
target_link_libraries(example_bluefox acq_seq bluefox_acq)

else(MVDEVICEMANAGER_LIBRARY AND MVLIBUSB1_LIBRARY)
message(WARNING "MVBlueFox driver not found, libraries related to this module will be ignored.")
endif(MVDEVICEMANAGER_LIBRARY AND MVLIBUSB1_LIBRARY)



if(BUILD_ROS_NODE)
	add_executable(single_camera_node src/nodes/single_camera_node.cpp)
	target_link_libraries(single_camera_node acq_seq bluefox_acq ${catkin_LIBRARIES}) 
endif(BUILD_ROS_NODE)


