cmake_minimum_required(VERSION 2.8) 
PROJECT(uasl_image_acquisition)

add_definitions(-std=c++11)

find_package(OpenCV REQUIRED)#Depend on your version of OpenCV and ROS installed. If your cv_bridge package relies on OpenCV 2, find_package(OpenCV 2 REQUIRED) is what you need. Else, find_package(OpenCV REQUIRED) will automatically look for OpenCV3.
find_package(Threads REQUIRED)

add_subdirectory(Third_party/libthermalgrabber)

EXECUTE_PROCESS(COMMAND uname -m COMMAND tr -d '\n' OUTPUT_VARIABLE ARCH)
message(STATUS "Architecture: ${ARCH}")

option(BUILD_ROS_NODE "Build ROS node" ON)
set(CATKIN_BUILD_TYPE "Debug")
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

option(TAU2_DRIVER "Build libthermalgrabber" ON)

set(SRC_ACQUISITION
	capture.cpp
)

set(SRC_BLUEFOX_ACQ
    src/camera_mvbluefox.cpp
)

set(SRC_TAU2_ACQ
	src/camera_tau2.cpp
)

set(SRC_ACQ
		src/acquisition.cpp
)
