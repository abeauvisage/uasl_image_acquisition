cmake_minimum_required(VERSION 2.8) 
project(thermalgrabber)
set( CMAKE_CXX_FLAGS "-std=c++11" )
#set( CMAKE_CXX_FLAGS "-fpermissive -std=c++11" )
set( TARGET thermalgrabber )
set( ROOT ./ )
set( LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib )

#set(USE_FTDI_LIB 1)


if (WIN32)

    MESSAGE(STATUS "Setup for windows")

    if (USE_FTDI_LIB)

        MESSAGE(STATUS "thermalgrabber : Using ftdi lib")
        ADD_DEFINITIONS(-DUSE_FTDI)

        include_directories (
            ${CMAKE_CURRENT_SOURCE_DIR}/src/inc
        )

        link_directories (
            ${CMAKE_CURRENT_SOURCE_DIR}/src/lib/ftdi_i386
        )

        link_libraries (
            ftd2xx
        )
  
    elseif (NOT USE_FTDI_LIB)

	   MESSAGE(STATUS "thermalgrabber : Using libusb")

	   include_directories (
		   ${CMAKE_CURRENT_SOURCE_DIR}/src/inc
	   )

	   find_library(LIB_USB_1 REQUIRED
		            NAMES usb-1.0)
		            
		if(NOT LIB_USB_1)
		 message(FATAL_ERROR "libusb-1.0 not found")
		endif(NOT LIB_USB_1)
	
	endif(USE_FTDI_LIB)


elseif (NOT WIN32)

    MESSAGE(STATUS "thermalgrabber : Setup for linux")

    if (USE_FTDI_LIB)

        MESSAGE(STATUS "thermalgrabber : Not implemented")

    elseif (NOT USE_FTDI_LIB)

        MESSAGE(STATUS "thermalgrabber : Using libusb")
		find_library(LIB_USB_1
					NAMES usb-1.0)
					
		if(NOT LIB_USB_1)
  		message(FATAL_ERROR "libusb-1.0 not found")
		endif(NOT LIB_USB_1)
		
        include_directories (
            /usr/include/libusb-1.0
            /usr/local/include/libusb-1.0

        )

    endif (USE_FTDI_LIB)

endif(WIN32)

#unix:CONFIG(release, debug|release): LIBS += -L/usr/include/libusb-1.0 -lusb-1.0

set( SOURCES

    ${CMAKE_CURRENT_SOURCE_DIR}/src/thermalgrabber.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/inc/thermalgrabber.h

    ${CMAKE_CURRENT_SOURCE_DIR}/src/thermograbber.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/thermograbber.h

    ${CMAKE_CURRENT_SOURCE_DIR}/src/tauinterface.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tauinterface.h

    ${CMAKE_CURRENT_SOURCE_DIR}/src/tauimagedecoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tauimagedecoder.h

    ${CMAKE_CURRENT_SOURCE_DIR}/src/taucom.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/taucom.h

    ${CMAKE_CURRENT_SOURCE_DIR}/src/fastftdi.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/fastftdi.h

    ${CMAKE_CURRENT_SOURCE_DIR}/src/crc.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/crc.h
)

include_directories (
    ${ROOT}
    ${THERMALGRABBER}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/inc # public header
    ${CMAKE_CURRENT_SOURCE_DIR}/src # internal headers
)

link_directories (
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lib
    #${CMAKE_CURRENT_SOURCE_DIR}/src/lib/ftdi_i386
)

add_library( ${TARGET} ${SOURCES} )
install( TARGETS ${TARGET} DESTINATION lib )

find_package(Threads REQUIRED)
target_link_libraries(
    ${TARGET}
    ${CMAKE_THREAD_LIBS_INIT}
    ${LIB_USB_1}
)

get_filename_component(LIB_USB_1_PATH_REAL ${LIB_USB_1} REALPATH)
get_filename_component(LIB_USB_1_DIRECTORY ${LIB_USB_1_PATH_REAL} DIRECTORY)

SET(CMAKE_INSTALL_RPATH "${LIBRARY_OUTPUT_PATH}"
						"${LIB_USB_1_DIRECTORY}"
						"${CMAKE_INSTALL_RPATH}"
						PARENT_SCOPE)
