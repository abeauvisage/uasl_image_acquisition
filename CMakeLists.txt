cmake_minimum_required(VERSION 2.8) 
PROJECT(image_acquisition)

add_definitions(-std=c++11)

find_package(OpenCV 2 REQUIRED)#Depend on your version of OpenCV and ROS installed. If your cv_bridge package relies on OpenCV 2, find_package(OpenCV 2 REQUIRED) is what you need. Else, find_package(OpenCV REQUIRED) will automatically look for OpenCV3.
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
		  LIBRARIES bluefox_acq #External libraries created by this package
	)
	include_directories(${catkin_INCLUDE_DIRS})
endif(BUILD_ROS_NODE)

set(SRC_ACQUISITION
	capture.cpp
)

set(SRC_BLUEFOX_ACQ
    src/camera_mvbluefox.cpp
)

set(SRC_MULTIPLE_CAM_SYNC
		src/multiple_cameras_sync.cpp
)

set(SRC_ACQ_SEQ
		src/acquisition.cpp
)

set(SRC_BLUEFOX_ACQ_SEQ
		src/camera_mvbluefox_seq.cpp
)

#mvBLueFOX libraries (64 bits)
set(MVBLUEFOX_TOP_LEVEL_PATH CACHE PATH "Path to the whole mvImpact_acquire folder.")
set(MVBLUEFOX_LIB_PATH ${MVBLUEFOX_TOP_LEVEL_PATH}/lib/x86_64 CACHE PATH "Path the the lib folder of the mvBlueFOX driver.")
set(MVBLUEFOX_INCLUDE_PATH ${MVBLUEFOX_TOP_LEVEL_PATH}/mvIMPACT_CPP ${MVBLUEFOX_TOP_LEVEL_PATH} ${MVBLUEFOX_TOP_LEVEL_PATH}/include/mvIMPACT_CPP ${MVBLUEFOX_TOP_LEVEL_PATH}/include CACHE PATH "Path the the header folder of the mvBlueFOX driver.")

	
find_library(MVDEVICEMANAGER_LIBRARY 
	NAMES mvDeviceManager
	HINTS ${MVBLUEFOX_LIB_PATH})#Only the mvDeviceManager library needs to be linked
	
find_library(MVLIBUSB1_LIBRARY
	NAMES libusb-1.0.so.0
	HINTS ${MVBLUEFOX_LIB_PATH} 
	NO_DEFAULT_PATH)#Explicitly link the libusb to be sure to use this one

include_directories(
  include
  ${OpenCV_INCLUDE_DIRS}
  ${MVBLUEFOX_INCLUDE_PATH}
)

add_library(multiple_cam_sync ${SRC_MULTIPLE_CAM_SYNC})
target_link_libraries(multiple_cam_sync ${OpenCV_LIBRARIES})

add_library(acq_seq ${SRC_ACQ_SEQ})
target_link_libraries(acq_seq ${OpenCV_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

if(MVDEVICEMANAGER_LIBRARY AND MVLIBUSB1_LIBRARY)
target_compile_definitions(acq_seq PRIVATE BLUEFOX_FOUND)

add_executable(image-acquisition ${SRC_ACQUISITION})
target_link_libraries(image-acquisition ${MVDEVICEMANAGER_LIBRARY} ${MVLIBUSB1_LIBRARY} ${OpenCV_LIBRARIES})

add_library(bluefox_acq ${SRC_BLUEFOX_ACQ})
target_link_libraries(bluefox_acq multiple_cam_sync ${MVDEVICEMANAGER_LIBRARY} ${MVLIBUSB1_LIBRARY} ${OpenCV_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

add_library(bluefox_acq_seq ${SRC_BLUEFOX_ACQ_SEQ})
target_link_libraries(bluefox_acq_seq ${MVDEVICEMANAGER_LIBRARY} ${MVLIBUSB1_LIBRARY} ${OpenCV_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

#Testing script
add_executable(test_bluefox_single test/test_bluefox_single.cpp)
target_link_libraries(test_bluefox_single bluefox_acq)

add_executable(test_bluefox_framerate test/test_bluefox_framerate.cpp)
target_link_libraries(test_bluefox_framerate bluefox_acq)

add_executable(test_bluefox_seq test/test_bluefox_seq.cpp)
target_link_libraries(test_bluefox_seq acq_seq bluefox_acq_seq)

#Example code
add_executable(example_single_bluefox examples/example_single_bluefox.cpp)
target_link_libraries(example_single_bluefox bluefox_acq)

add_executable(example_multiple_bluefox examples/example_multiple_bluefox.cpp)
target_link_libraries(example_multiple_bluefox bluefox_acq)
else(MVDEVICEMANAGER_LIBRARY AND MVLIBUSB1_LIBRARY)
message(WARNING "mvBlueFox driver not found, libraries related to this module will be ignored.")
endif(MVDEVICEMANAGER_LIBRARY AND MVLIBUSB1_LIBRARY)



if(BUILD_ROS_NODE)
	add_executable(single_camera_node src/nodes/single_camera_node.cpp)
	target_link_libraries(single_camera_node bluefox_acq ${catkin_LIBRARIES}) 
endif(BUILD_ROS_NODE)


