cmake_minimum_required(VERSION 2.8) 
PROJECT(multispectral_acquisition)

add_definitions(-std=c++11)

find_package(OpenCV REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 QUIET gtk+-3.0)
find_package(Threads REQUIRED)

if(GTK3_FOUND)
add_definitions(-DGTK3_FOUND)
endif(GTK3_FOUND)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/builder.ui ${CMAKE_CURRENT_BINARY_DIR}/builder.ui COPYONLY)

set(SRC_ACQUISITION
	src/capture.cpp
	src/cameraTau2.cpp
	src/configWindow.cpp
	src/imageHandler.cpp
)

EXECUTE_PROCESS(COMMAND uname -m COMMAND tr -d '\n' OUTPUT_VARIABLE ARCH)#Get the architecture to locate the correct librairies

#mvBLueFOX libraries
if((NOT MVDEVICEMANAGER_LIBRARY) OR (NOT MVPROPHANDLING_LIBRARY))
set(MVBLUEFOX_TOP_LEVEL_PATH_CACHE ${MVBLUEFOX_TOP_LEVEL_PATH} CACHE PATH "Path to the whole mvImpact_acquire folder." FORCE)
set(MVBLUEFOX_LIB_PATH ${MVBLUEFOX_TOP_LEVEL_PATH_CACHE}/lib/${ARCH} CACHE PATH "Path the the lib folder of the mvBlueFOX driver." FORCE)
set(MVBLUEFOX_INCLUDE_PATH ${MVBLUEFOX_TOP_LEVEL_PATH_CACHE}/mvIMPACT_CPP ${MVBLUEFOX_TOP_LEVEL_PATH_CACHE} ${MVBLUEFOX_TOP_LEVEL_PATH_CACHE}/include/mvIMPACT_CPP ${MVBLUEFOX_TOP_LEVEL_PATH}/include CACHE PATH "Path the the header folder of the mvBlueFOX driver." FORCE)
endif((NOT MVDEVICEMANAGER_LIBRARY) OR (NOT MVPROPHANDLING_LIBRARY))

find_library(MVDEVICEMANAGER_LIBRARY 
	NAMES mvDeviceManager
	PATHS ${MVBLUEFOX_LIB_PATH}
	NO_DEFAULT_PATH)
	
find_library(MVPROPHANDLING_LIBRARY 
	NAMES mvPropHandling
	PATHS ${MVBLUEFOX_LIB_PATH}
	NO_DEFAULT_PATH)

find_library(LIBSERIAL_LIBRARY
	NAMES libserial.so.0
)

include_directories(
  include
  ${OpenCV_INCLUDE_DIRS}
  ${MVBLUEFOX_INCLUDE_PATH}
  ${GTK3_INCLUDE_DIRS}
)

if(MVDEVICEMANAGER_LIBRARY AND MVPROPHANDLING_LIBRARY AND LIBSERIAL_LIBRARY)

add_executable(multispectral_acquisition ${SRC_ACQUISITION})
target_link_libraries(multispectral_acquisition ${LIBSERIAL_LIBRARY} ${MVDEVICEMANAGER_LIBRARY} ${OpenCV_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${GTK3_LIBRARIES})

else(MVDEVICEMANAGER_LIBRARY AND MVPROPHANDLING_LIBRARY AND LIBSERIAL_LIBRARY)
message(WARNING "mvBlueFox or libSerial not found, libraries related to this module will be ignored.")
endif(MVDEVICEMANAGER_LIBRARY AND MVPROPHANDLING_LIBRARY AND LIBSERIAL_LIBRARY)
