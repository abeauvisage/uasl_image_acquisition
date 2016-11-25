cmake_minimum_required(VERSION 2.8) 
PROJECT(image-acquisition)

add_definitions(-std=c++11)#Currently used in to_string

find_package(OpenCV REQUIRED)
find_package(Threads REQUIRED)

set(SRC_ACQUISITION
	capture.cpp
)

#Header list is useful to generate the headers for some IDE
set(HEADER_ACQUISITION 
    include/camera.hpp
    include/camera_mvbluefox.hpp
    include/util_clock.hpp
    include/util_signal.hpp
)

set(SRC_BLUEFOX_ACQ
    src/camera_mvbluefox.cpp
)

#mvBLueFOX libraries (64 bits)
set(MVBLUEFOX_TOP_LEVEL_PATH CACHE PATH "Path to the whole mvImpact_acquire folder.")
set(MVBLUEFOX_LIB_PATH "${MVBLUEFOX_TOP_LEVEL_PATH}/lib/x86_64" CACHE PATH "Path the the lib folder of the mvBlueFOX driver.")
set(MVBLUEFOX_INCLUDE_PATH ${MVBLUEFOX_TOP_LEVEL_PATH}/mvIMPACT_CPP ${MVBLUEFOX_TOP_LEVEL_PATH} CACHE PATH "Path the the header folder of the mvBlueFOX driver.")

	
find_library(MVDEVICEMANAGER_LIBRARY 
	NAMES mvDeviceManager
	HINTS ${MVBLUEFOX_LIB_PATH})#Only the mvDeviceManager library needs to be linked

include_directories(
  include
  ${OpenCV_INCLUDE_DIRS}
  ${MVBLUEFOX_INCLUDE_PATH}
)

add_executable(image-acquisition ${SRC_ACQUISITION})
target_link_libraries(image-acquisition ${MVDEVICEMANAGER_LIBRARY} ${OpenCV_LIBRARIES})

add_library(bluefox_acq ${SRC_BLUEFOX_ACQ} ${HEADER_ACQUISITION})
target_link_libraries(bluefox_acq ${MVDEVICEMANAGER_LIBRARY} ${OpenCV_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

#Testing script
add_executable(test_bluefox_single test/test_bluefox_single.cpp)
target_link_libraries(test_bluefox_single bluefox_acq)

#Example code
add_executable(example_single_bluefox examples/example_single_bluefox.cpp)
target_link_libraries(example_single_bluefox bluefox_acq)

