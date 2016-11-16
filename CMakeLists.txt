cmake_minimum_required(VERSION 2.8) 
PROJECT(image-acquisition)

add_definitions(-std=c++11)#Currently used in to_string

find_package(OpenCV REQUIRED)

set(SRC_ACQUISITION
	capture.cpp
)

#Header list is useful to generate the headers for some IDE
set(HEADER_ACQUISITION 

)

#mvBLueFOX libraries (64 bits)
set(MVBLUEFOX_TOP_LEVEL_PATH CACHE PATH "Path to the whole mvImpact_acquire folder.")
set(MVBLUEFOX_LIB_PATH "${MVBLUEFOX_TOP_LEVEL_PATH}/lib/x86_64" CACHE PATH "Path the the lib folder of the mvBlueFOX driver.")
set(MVBLUEFOX_INCLUDE_PATH ${MVBLUEFOX_TOP_LEVEL_PATH}/mvIMPACT_CPP ${MVBLUEFOX_TOP_LEVEL_PATH} CACHE PATH "Path the the header folder of the mvBlueFOX driver.")

find_library(MVBLUEFOX_LIBRARY 
	NAMES mvBlueFOX
	HINTS ${MVBLUEFOX_LIB_PATH})
	
find_library(MVDEVICEMANAGER_LIBRARY 
	NAMES mvDeviceManager
	HINTS ${MVBLUEFOX_LIB_PATH})
	
find_library(MVPROPHANDLING_LIBRARY 
	NAMES mvPropHandling
	HINTS ${MVBLUEFOX_LIB_PATH})

find_library(USB-1.0_LIBRARY 
	NAMES libusb-1.0.so.0 
	PATHS ${MVBLUEFOX_LIB_PATH} NO_DEFAULT_PATH)#Avoid conflicting with a potiential existing version of libusb-1.0
	
set(BLUEFOX_LIBRARIES ${MVBLUEFOX_LIBRARY} ${MVDEVICEMANAGER_LIBRARY} ${MVPROPHANDLING_LIBRARY} ${USB-1.0_LIBRARY})

include_directories(
  include
  ${OpenCV_INCLUDE_DIRS}
  ${MVBLUEFOX_INCLUDE_PATH}
)

add_executable(image-acquisition ${SRC_ACQUISITION})

target_link_libraries(image-acquisition ${BLUEFOX_LIBRARIES} ${OpenCV_LIBRARIES})

