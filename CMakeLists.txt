cmake_minimum_required(VERSION 2.8) 
PROJECT(uasl_image_acquisition)

add_definitions(-std=c++11)

#Function to purge a list of paths of non existing paths
function(purge_non_exist_paths input_path)
	foreach(path IN LISTS ${input_path})
		if(EXISTS ${path})
			set(TEMP_PATH_LIST ${TEMP_PATH_LIST} ${path})
		endif()
	endforeach()
	set(${input_path} ${TEMP_PATH_LIST} PARENT_SCOPE)
endfunction()

find_package(OpenCV REQUIRED)#Depend on your version of OpenCV and ROS installed. If your cv_bridge package relies on OpenCV 2, find_package(OpenCV 2 REQUIRED) is what you need. Else, find_package(OpenCV REQUIRED) will automatically look for OpenCV3.
find_package(Threads REQUIRED)

option(BUILD_ROS_NODE "Build ROS node" ON)
option(TAU2_DRIVER "Build libthermalgrabber" ON)

#RPATH parameters
SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)


if(BUILD_ROS_NODE)
	find_package(catkin REQUIRED COMPONENTS roscpp cv_bridge image_transport)
endif(BUILD_ROS_NODE)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
endif()

EXECUTE_PROCESS(COMMAND uname -m COMMAND tr -d '\n' OUTPUT_VARIABLE ARCH)#Get the architecture to locate the correct librairies

file(GLOB HEADERS "include/${PROJECT_NAME}/*.hpp")

#search for mvBLueFOX libraries (64 bits) ----------------------------------------------
if(NOT MVBLUEFOX_TOP_LEVEL_PATH)
set(MVBLUEFOX_TOP_LEVEL_PATH /opt/mvIMPACT_acquire)
endif(NOT MVBLUEFOX_TOP_LEVEL_PATH)
set(MVBLUEFOX_TOP_LEVEL_PATH_CACHE ${MVBLUEFOX_TOP_LEVEL_PATH} CACHE PATH "Path to the whole mvImpact_acquire folder." FORCE)
set(MVBLUEFOX_LIB_PATH ${MVBLUEFOX_TOP_LEVEL_PATH_CACHE}/lib/${ARCH} CACHE PATH "Path the the lib folder of the mvBlueFOX driver." FORCE)
set(MVBLUEFOX_INCLUDE_PATH ${MVBLUEFOX_TOP_LEVEL_PATH_CACHE}/mvIMPACT_CPP ${MVBLUEFOX_TOP_LEVEL_PATH_CACHE} ${MVBLUEFOX_TOP_LEVEL_PATH_CACHE}/include/mvIMPACT_CPP ${MVBLUEFOX_TOP_LEVEL_PATH}/include CACHE PATH "Path the the header folder of the mvBlueFOX driver." FORCE)

find_library(MVDEVICEMANAGER_LIBRARY 
	NAMES mvDeviceManager
	PATHS ${MVBLUEFOX_LIB_PATH}
	NO_DEFAULT_PATH)
	
find_library(MVPROPHANDLING_LIBRARY 
	NAMES mvPropHandling
	PATHS ${MVBLUEFOX_LIB_PATH}
	NO_DEFAULT_PATH)

#Define path for tau2 driver (no need to search since it is included)
if(TAU2_DRIVER)
set(TAU2_DRIVER_INCLUDE_PATH "Third_party/libthermalgrabber/inc")
endif(TAU2_DRIVER)
	
#Check for libraries
if(MVDEVICEMANAGER_LIBRARY AND MVPROPHANDLING_LIBRARY)
set(SPECIFIC_CAM_LIBS ${SPECIFIC_CAM_LIBS} bluefox_acq ${MVDEVICEMANAGER_LIBRARY} ${MVPROPHANDLING_LIBRARY})
set(SPECIFIC_CAM_INCLUDE ${SPECIFIC_CAM_INCLUDE} ${MVBLUEFOX_INCLUDE_PATH})
endif(MVDEVICEMANAGER_LIBRARY AND MVPROPHANDLING_LIBRARY)

if(TAU2_DRIVER)
set(SPECIFIC_CAM_LIBS ${SPECIFIC_CAM_LIBS} tau2_acq)
set(SPECIFIC_CAM_INCLUDE ${SPECIFIC_CAM_INCLUDE} ${TAU2_DRIVER_INCLUDE_PATH})
endif(TAU2_DRIVER)

#Purge the non existing paths from the list
purge_non_exist_paths(SPECIFIC_CAM_INCLUDE)

#Note : since catkin_package must be called before any call to add_library or add_executable,
#but we need to know wich libraries have been detected to call catkin_package, we need to split
#the library detection and library use in two

if(BUILD_ROS_NODE)
	catkin_package(
		  DEPENDS OpenCV
		  CATKIN_DEPENDS roscpp image_transport cv_bridge
		  INCLUDE_DIRS include ${SPECIFIC_CAM_INCLUDE}
		  LIBRARIES trigger acq_seq ${SPECIFIC_CAM_LIBS}#External libraries created by this package
	)
	include_directories(${catkin_INCLUDE_DIRS})
endif(BUILD_ROS_NODE)

#Trigger code
add_library(trigger src/trigger.cpp)

add_library(acq_seq src/acquisition.cpp ${HEADERS})
target_link_libraries(acq_seq trigger ${OpenCV_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})


if(MVDEVICEMANAGER_LIBRARY AND MVPROPHANDLING_LIBRARY)
	add_library(bluefox_acq src/camera_mvbluefox.cpp)
	target_link_libraries(bluefox_acq ${MVDEVICEMANAGER_LIBRARY} ${MVPROPHANDLING_LIBRARY} ${OpenCV_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

	#Testing scripts
	add_executable(test_bluefox_framerate test/test_bluefox_framerate.cpp)
	target_link_libraries(test_bluefox_framerate acq_seq bluefox_acq)

	add_executable(test_bluefox_image_format test/test_bluefox_image_format.cpp)
	target_link_libraries(test_bluefox_image_format acq_seq bluefox_acq)

	#Example code
	add_executable(example_bluefox examples/example_bluefox.cpp)
	target_link_libraries(example_bluefox acq_seq bluefox_acq)
	
	if(BUILD_ROS_NODE)
		add_executable(single_camera_node src/nodes/single_camera_node.cpp)
		target_link_libraries(single_camera_node acq_seq bluefox_acq ${catkin_LIBRARIES}) 
	endif(BUILD_ROS_NODE)
else(MVDEVICEMANAGER_LIBRARY AND MVPROPHANDLING_LIBRARY )
	message(WARNING "MVBlueFox driver not found, libraries related to this module will be ignored.")
endif(MVDEVICEMANAGER_LIBRARY AND MVPROPHANDLING_LIBRARY )
	

#Tau2 libraries ----------------------------------------------

#find_library(LIBSERIAL_LIBRARY
#	NAMES libserial.so.0
#)
if(TAU2_DRIVER)
						
	add_subdirectory(Third_party/libthermalgrabber)						
	
	add_library(tau2_acq src/camera_tau2.cpp)					
	
	target_link_libraries(tau2_acq thermalgrabber ${OpenCV_LIBRARIES})
		
	add_executable(test_tau2_framerate test/test_tau2_framerate.cpp)
	target_link_libraries(test_tau2_framerate acq_seq tau2_acq )

	#Example code
	add_executable(example_tau2 examples/example_tau2.cpp)
	target_link_libraries(example_tau2 acq_seq tau2_acq)
endif(TAU2_DRIVER)

#All libs ----------------------------------------------
if(MVDEVICEMANAGER_LIBRARY AND TAU2_DRIVER)
	add_executable(stereo_example examples/stereo_example.cpp)
	target_link_libraries(stereo_example acq_seq tau2_acq bluefox_acq)
endif(MVDEVICEMANAGER_LIBRARY AND TAU2_DRIVER)



include_directories(
  include/${PROJECT_NAME}
  ${TAU2_DRIVER_INCLUDE_PATH}
  ${OpenCV_INCLUDE_DIRS}
  ${MVBLUEFOX_INCLUDE_PATH}
)


