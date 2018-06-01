set( CMAKE_BUILD_TYPE RelWithDebInfo )
set( CMAKE_CXX_FLAGS "-std=c++11" )
#set( CMAKE_CXX_FLAGS "-fpermissive -std=c++11" )
set( CMAKE_VERBOSE_MAKEFILE on )
set( TARGET thermalgrabber )
set( ROOT ./ )
set( LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib )

#set(USE_FTDI_LIB 1)


cmake_minimum_required(VERSION 2.8)



if (WIN32)

    MESSAGE("Setup for windows")

    if (USE_FTDI_LIB)

        MESSAGE("Using ftdi lib")
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

        MESSAGE("Using libusb")

        include_directories (
            ${CMAKE_CURRENT_SOURCE_DIR}/src/inc
        )

        link_directories (
            ${CMAKE_CURRENT_SOURCE_DIR}/src/lib/MingGW32/dll
        )

        link_libraries (
            libusb-1.0
        )

    endif(USE_FTDI_LIB)

elseif (NOT WIN32)

    MESSAGE("Setup for linux")

    if (USE_FTDI_LIB)

        MESSAGE("Not implemented")

    elseif (NOT USE_FTDI_LIB)

        MESSAGE("Using libusb")

        include_directories (
            /usr/include/libusb-1.0
            /usr/local/include/libusb-1.0

        )

        link_directories (
            /usr/lib/x86_64-linux-gnu
            /usr/local/lib
        )

        link_libraries (
            usb-1.0
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

add_library( ${TARGET} SHARED ${SOURCES} )
install( TARGETS ${TARGET} DESTINATION lib )

target_link_libraries(
    ${TARGET}
    pthread
)
