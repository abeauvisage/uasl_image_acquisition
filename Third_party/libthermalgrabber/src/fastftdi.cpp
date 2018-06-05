/*
 * fastftdi.c - A minimal FTDI FT2232H interface for which supports bit-bang
 *              mode, but focuses on very high-performance support for
 *              synchronous FIFO mode. Requires libusb-1.0
 *
 * Copyright (C) 2009 Micah Elizabeth Scott
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include "fastftdi.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <iostream>
#include <thread>

#ifdef _WIN32

#include <Windows.h>
#include <stdint.h> // portable: uint64_t   MSVC: __int64

int gettimeofday(struct timeval * tp, struct timezone * tzp)
{
    // Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
    static const uint64_t EPOCH = ((uint64_t) 116444736000000000ULL);

    SYSTEMTIME  system_time;
    FILETIME    file_time;
    uint64_t    time;

    GetSystemTime( &system_time );
    SystemTimeToFileTime( &system_time, &file_time );
    time =  ((uint64_t)file_time.dwLowDateTime )      ;
    time += ((uint64_t)file_time.dwHighDateTime) << 32;

    tp->tv_sec  = (long) ((time - EPOCH) / 10000000L);
    tp->tv_usec = (long) (system_time.wMilliseconds * 1000);
    return 0;
}

#endif

typedef struct {
    FTDIStreamCallback *callback;
    void *userdata;
    int result;
    FTDIProgressInfo progress;
} FTDIStreamState;

static int
DeviceInit(FTDIDevice *dev)
{

#ifdef USE_FTDI // now already in DeviceOpen...

//    FT_STATUS ftStatus;

//    if (iSerialUSB == 0 || iSerialUSB[0] == '\0')
//    {
//        ftStatus = FT_OpenEx(const_cast<char*>(FTDI_DESCRIPTION), FT_OPEN_BY_DESCRIPTION, &dev->handle);
//    }
//    else
//    {
//        ftStatus = FT_OpenEx(iSerialUSB, FT_OPEN_BY_SERIAL_NUMBER,&dev->handle);
//    }




//    if (ftStatus == FT_OK)
//    {
//        printf("device found\n");
//    }
//    else
//    {
//        printf("device not found\n");
//        FT_Close(dev->handle);
//        return -1;
//    }
//    return 0;

//    if (baudRate)
//    {
//        if (FT_OK != FT_SetBaudRate(dev->handle, baudRate))
//           return -1;
//    }

//    return err;

    return 0;

#else

    int err, interface;

    for (interface = 0; interface < 2; interface++) {
        if (libusb_kernel_driver_active(dev->handle, interface) == 1) {
            if ((err = libusb_detach_kernel_driver(dev->handle, interface))) {
                perror("Error detaching kernel driver");
                return err;
            }
        }
    }

    if ((err = libusb_set_configuration(dev->handle, 1))) {
        perror("Error setting configuration");
        return err;
    }

    for (interface = 0; interface < 2; interface++) {
        if ((err = libusb_claim_interface(dev->handle, interface))) {
            perror("Error claiming interface");
            return err;
        }
    }

    return 0;

#endif

}

static int
DeviceRelease(FTDIDevice *dev)
{

#ifdef USE_FTDI

    std::cout << "Not implemented yet" << std::endl;

#else

    if (dev->handle != 0)
    {
        int interface, err;

        for (interface = 0; interface < 2; interface++) {
            if ((err = libusb_release_interface(dev->handle, interface))) {
                perror("Error releasing interface");
                return err;
            }
        }
    }

#endif

    return 0;
}

void FTDI_PrintDeviceList()
{

#ifdef USE_FTDI

    // refere to ftdi document FT_000071
    FT_STATUS ftStatus;
    FT_DEVICE_LIST_INFO_NODE *devInfo;
    DWORD numDevs;
    // create the device information list
    ftStatus = FT_CreateDeviceInfoList(&numDevs);

    if (ftStatus == FT_OK)
    {
        printf("Number of devices is %d\n",numDevs);
    }

    if (numDevs > 0)
    {
        // allocate storage for list based on numDevs
        devInfo = (FT_DEVICE_LIST_INFO_NODE*)malloc(sizeof(FT_DEVICE_LIST_INFO_NODE)*numDevs);
        // get the device information list
        ftStatus = FT_GetDeviceInfoList(devInfo,&numDevs);

        if (ftStatus == FT_OK)
        {
            for (int i = 0; i < numDevs; i++)
            {
                printf("Dev %d:\n",i);
                printf("  Flags=0x%x\n",devInfo[i].Flags);
                printf("  Type=0x%x\n",devInfo[i].Type);
                printf("  ID=0x%x\n",devInfo[i].ID);
                printf("  LocId=0x%x\n",devInfo[i].LocId);
                printf("  SerialNumber=%s\n",devInfo[i].SerialNumber);
                printf("  Description=%s\n",devInfo[i].Description);
                printf("  ftHandle=0x%x\n",devInfo[i].ftHandle);
            }
        }
    }

#else

    libusb_device **devs; // dev list
    libusb_context *ctx = NULL;
    int ret;
    ssize_t cnt; //number of devices
    ret = libusb_init(&ctx); //initialize a library session

    if(ret < 0)
    {
        std::cerr << "Unable to init lib usb session" << std::endl;
        return;
    }

    libusb_set_debug(ctx, 3); //verbosity level to 3
    cnt = libusb_get_device_list(ctx, &devs); //get the list of devices

    if(cnt < 0)
    {
        std::cerr << "Error when trying to get device list" << std::endl;
    }

    //std::cout << "Number of devices is " << cnt << std::endl;
    ssize_t i;

    for(i = 0; i < cnt; i++)
    {
        libusb_device_descriptor desc;
        int ret = libusb_get_device_descriptor(devs[i], &desc);

        if (ret < 0)
        {
            std::cerr << "Failed to get device descriptor" << std::endl;
            return;
        }

        if (desc.idVendor == 1027) // ftdi vendor in hex 0x0403 -> 1027
        {
            std::cout << "FTDI device found:" << std::endl;
            std::cout << "VendorID: " << std::hex << "0x" << ((desc.idVendor<0x10)?"0":"") << desc.idVendor << std::dec << std::endl;
            std::cout << "ProductID: " << std::hex << "0x" << ((desc.idProduct<0x10)?"0":"") << desc.idProduct << std::dec << std::endl;
            std::cout << "Number of possible configurations: " << (int)desc.bNumConfigurations << std::endl;
            std::cout << "Device Class: " << (int)desc.bDeviceClass << std::endl;

            if (desc.iSerialNumber)
            {
                struct libusb_device_handle *handle = NULL;
                int e = libusb_open(devs[i], &handle);

                if (e<0)
                {
                    std::cerr << "error opening device" << std::endl;
                }
                else
                {
                    unsigned char data[64];
                    e = libusb_get_string_descriptor_ascii(handle, desc.iSerialNumber, data, 64);
                    std::cout << "Serial: " << data << std::endl;
                }
            }

            libusb_config_descriptor *config;
            libusb_get_config_descriptor(devs[i], 0, &config);

            std::cout << "Interfaces: " << (int)config->bNumInterfaces << std::endl;
            const libusb_interface *inter;
            const libusb_interface_descriptor *interdesc;
            const libusb_endpoint_descriptor *epdesc;

            for(int i=0; i<(int)config->bNumInterfaces; i++)
            {
                inter = &config->interface[i];
                std::cout << "Number of alternate settings: " << inter->num_altsetting << std::endl;

                for(int j=0; j<inter->num_altsetting; j++)
                {
                    interdesc = &inter->altsetting[j];
                    std::cout << "    Interface Number: " << (int)interdesc->bInterfaceNumber << std::endl;
                    std::cout << "    Number of endpoints: " << (int)interdesc->bNumEndpoints << std::endl;

                    for(int k=0; k<(int)interdesc->bNumEndpoints; k++)
                    {
                        epdesc = &interdesc->endpoint[k];
                        std::cout << "        Descriptor Type: " << (int)epdesc->bDescriptorType << std::endl;
                        std::cout << "        EP Address: " << std::hex << "0x" << ((desc.idVendor<0x10)?"0":"") << (int)epdesc->bEndpointAddress << std::dec << std::endl;
                    }
                }
            }
            std::cout << std::endl << std::endl;
            libusb_free_config_descriptor(config);
        }
    }
    libusb_free_device_list(devs, 1); //free the list, unref the devices in it
    libusb_exit(ctx); //close the session

#endif

}

int
FTDIDevice_Open(FTDIDevice *dev, char* iSerialUSB)
{

#ifdef USE_FTDI

    // refere to ftdi document FT_000071
    FT_STATUS ftStatus;

    if (iSerialUSB == 0 || iSerialUSB[0] == '\0')
    {
        std::cout << "Trying to connect first available ThermalCapture GrabberUSB" << std::endl;
        //ftStatus = FT_OpenEx(const_cast<char*>(FTDI_DESCRIPTION), FT_OPEN_BY_DESCRIPTION, &dev->handle);

        FT_DEVICE_LIST_INFO_NODE *devInfo;
        DWORD numDevs;
        // create the device information list
        ftStatus = FT_CreateDeviceInfoList(&numDevs);

        if (ftStatus == FT_OK)
        {
            printf("Number of found devices: %d\n",numDevs);
        }

        if (numDevs > 0)
        {
            // allocate storage for list based on numDevs
            devInfo = (FT_DEVICE_LIST_INFO_NODE*)malloc(sizeof(FT_DEVICE_LIST_INFO_NODE)*numDevs);
            // get the device information list
            ftStatus = FT_GetDeviceInfoList(devInfo, &numDevs);

            char foundSerial[32] = {0};

            if (ftStatus == FT_OK)
            {
                printf("Parsing list of ftdi devices\n");

                for (int i = 0; i < numDevs; i++)
                {
                    printf("Dev %d:\n",i);
                    printf("  Flags=0x%x\n", devInfo[i].Flags);
                    printf("  Type=0x%x\n", devInfo[i].Type);
                    printf("  ID=0x%x\n", devInfo[i].ID);
                    printf("  LocId=0x%x\n", devInfo[i].LocId);
                    printf("  SerialNumber=%s\n", devInfo[i].SerialNumber);
                    printf("  Description=%s\n", devInfo[i].Description);
                    printf("  ftHandle=0x%x\n", devInfo[i].ftHandle);

                    // --------------------------------------------------------
                    // Catch misc versions of TC GrabberUSB with simmilar
                    // but different description.
                    // Descriptor always contains a "TAU" and an ending with " A".
                    // If found -> try to connect
                    // If connected -> return success
                    // If connect failed(e.g. already connected) -> go on searching for next device
                    // If not found -> go on searching for next device

                    if (strstr(devInfo[i].Description, "TAU") && strstr(devInfo[i].Description, " A"))
                    {
                        strncpy(&foundSerial[0], devInfo[i].SerialNumber, sizeof(foundSerial));

                        std::cout << "Found Serial: " << foundSerial << std::endl;
                        std::cout << "Trying to connect ThermalCapture GrabberUSB with USB iSerial: " << foundSerial << std::endl;
                        ftStatus = FT_OpenEx(foundSerial, FT_OPEN_BY_SERIAL_NUMBER, &dev->handle);
                        if (ftStatus == FT_OK)
                            return 0;
                    }
                    else
                    {
                        ftStatus = FT_DEVICE_NOT_OPENED;
                        std::cout << devInfo[i].SerialNumber << " not adequate" << std::endl;
                    }
                }
            }
        }
    }
    else
    {
        std::cout << "Trying to connect ThermalCapture GrabberUSB with USB iSerial: " << iSerialUSB << std::endl;
        ftStatus = FT_OpenEx(iSerialUSB, FT_OPEN_BY_SERIAL_NUMBER, &dev->handle);
    }

    if (ftStatus == FT_OK)
    {
        printf("device found\n");
    }
    else
    {
        printf("device not found\n");
        FT_Close(dev->handle);
        return -1;
    }
    return 0;
//    return DeviceInit(dev);

#else

    int err;

    memset(dev, 0, sizeof *dev);

    if ((err = libusb_init(&dev->libusb))) {
        return err;
    }

    libusb_set_debug(dev->libusb, 3);

    libusb_device **devs; // dev list
    int ret;
    ssize_t cnt; //number of devices

    cnt = libusb_get_device_list(dev->libusb, &devs); //get the list of devices

    if(cnt < 0)
    {
        std::cerr << "Error when trying to get device list" << std::endl;
    }

    //std::cout << "Number of found devices: " << cnt << std::endl;
    int i = 0;

    int return_value=-1;

    for (i=0; i<cnt; i++)
    {
        libusb_device_descriptor desc;
        int ret = libusb_get_device_descriptor(devs[i], &desc);

        if (ret < 0)
        {
            std::cerr << "Failed to get device descriptor" << std::endl;
            return -1;
        }

        if (desc.idVendor == 1027) // ftdi vendor in hex 0x0403 -> 1027
        {
            unsigned char data[64] = {0};

            std::cout << "FTDI device found:" << std::endl;
            std::cout << "VendorID: " << std::hex << "0x" << ((desc.idVendor<0x10)?"0":"") << desc.idVendor << std::dec << std::endl;
            std::cout << "ProductID: " << std::hex << "0x" << ((desc.idProduct<0x10)?"0":"") << desc.idProduct << std::dec << std::endl;

            if (desc.iSerialNumber)
            {
                struct libusb_device_handle *handle = NULL;
                int e = libusb_open(devs[i], &handle);

                if (e<0)
                {
                    std::cerr << "error opening device" << std::endl;
                }
                else
                {
                    e = libusb_get_string_descriptor_ascii(handle, desc.iSerialNumber, data, 64);
                    std::cout << "iSerial: " << data << std::endl;
                }
                libusb_close(handle);
            }
            std::cout << std::endl << std::endl;

            // If there is no info about a unique iSerial that should be connected
            // take the first that is available
            if (iSerialUSB == 0 || iSerialUSB[0] == '\0')
            {
                std::cout << "Trying to connect first available ThermalCapture GrabberUSB" << std::endl;
                int success = libusb_open(devs[i], &dev->handle); // try to open device

                if (success == 0)
                {
                    return_value = DeviceInit(dev); // try to init device

                    if (return_value == 0) //success?
                        break; // usb device open -> quit loop
                }
            }
            else
            {
                std::cout << "Trying to connect ThermalCapture GrabberUSB with USB iSerial: " << iSerialUSB << std::endl;
                //result compare
                int rc = strcmp(iSerialUSB, (char*)&data[0]);

                if (!rc)
                {
                    std::cout << "Success: iSerial found (requested/found) " << iSerialUSB << "/" << &data[0] << std::endl;
                    int success = libusb_open(devs[i], &dev->handle); // try to open device

                    if (success == 0)
                    {
                        return_value = DeviceInit(dev); // try to init device
                        if (return_value == 0) //success?
                            break; // usb device open -> quit loop
                    }
                }
                else
                {
                    std::cout << "Skipping: iSerial not found (requested/found) " << iSerialUSB << "/" << &data[0] << std::endl;
                }
            }
        }
    }

    // free the device list
    libusb_free_device_list(devs, 1);

    if (!dev->handle) {
        return LIBUSB_ERROR_NO_DEVICE;
    }

    return return_value;

#endif

}


void
FTDIDevice_Close(FTDIDevice *dev)
{
std::cout << "FTDIDevice_Close..." << std::endl;
#ifdef USE_FTDI

    FT_Close(dev->handle);

#else

    DeviceRelease(dev);

    libusb_close(dev->handle);
    libusb_exit(dev->libusb);

#endif

}


int
FTDIDevice_Reset(FTDIDevice *dev)
{

#ifdef USE_FTDI

	FT_ResetDevice(dev->handle);

#else

    int err;
    int interface;

    err = libusb_reset_device(dev->handle);

    if (!err)
        err = DeviceRelease(dev);

    if (err)
        return err;
#endif
    return DeviceInit(dev);
}


int
FTDIDevice_SetMode(FTDIDevice *dev, FTDIInterface interface,
                   FTDIBitmode mode, uint8_t pinDirections,
                   int baudRate)
{

    int err = 0;

#ifdef USE_FTDI

    UCHAR Mask = 0xff;
    UCHAR Mode = mode;

    if (FT_OK != FT_SetBitMode(dev->handle, Mask, Mode))
    {
        err = -1;
    }

    if (FT_OK != FT_SetTimeouts(dev->handle, 0, 0))
    {
        err = -1;
    }

    if (baudRate)
    {
        if (FT_OK != FT_SetBaudRate(dev->handle, baudRate))
            err = -1;
    }

#else

    err = libusb_control_transfer(dev->handle,
                                  LIBUSB_REQUEST_TYPE_VENDOR
                                  | LIBUSB_RECIPIENT_DEVICE
                                  | LIBUSB_ENDPOINT_OUT,
                                  FTDI_SET_BITMODE_REQUEST,
                                  pinDirections | (mode << 8),
                                  interface,
                                  NULL, 0,
                                  FTDI_COMMAND_TIMEOUT);
    if (err)
        return err;

    if (baudRate) {
        int divisor;

        if (mode == FTDI_BITMODE_BITBANG)
            baudRate <<= 2;

        divisor = 240000000 / baudRate;
        if (divisor < 1 || divisor > 0xFFFF) {
            return LIBUSB_ERROR_INVALID_PARAM;
        }

        err = libusb_control_transfer(dev->handle,
                                      LIBUSB_REQUEST_TYPE_VENDOR
                                      | LIBUSB_RECIPIENT_DEVICE
                                      | LIBUSB_ENDPOINT_OUT,
                                      FTDI_SET_BAUD_REQUEST,
                                      divisor,
                                      interface,
                                      NULL, 0,
                                      FTDI_COMMAND_TIMEOUT);
        if (err)
            return err;
    }

#endif

    return err;
}


/*
 * Internal callback for cleaning up async writes.
 */

static void
WriteAsyncCallback(struct libusb_transfer *transfer)
{

#ifdef USE_FTDI

    std::cerr << "WriteAsyncCallback not implemented" << std::endl;

#else

    free(transfer->buffer);
    libusb_free_transfer(transfer);

#endif

}


/*
 * Write to an FTDI interface, either synchronously or asynchronously.
 * Async writes have no completion callback, they finish 'eventually'.
 */

int
FTDIDevice_Write(FTDIDevice *dev, FTDIInterface interface,
                 uint8_t *data, size_t length, bool async)
{

#ifdef USE_FTDI

    FT_STATUS ftStatus;
    if (async)
    {
        std::cout << "not implemented yet" << std::endl;
    }
    else
    {
        DWORD BytesWritten;
        //std::cout << "bytes written: " << BytesWritten << std::endl;
        ftStatus = FT_Write(dev->handle, data, length, &BytesWritten);
        //std::cout << "bytes written: " << BytesWritten << std::endl;
    }

    if (FT_OK != ftStatus)
        return -1;
    return 0;

#else

    int err;

    if (async) {
        struct libusb_transfer *transfer = libusb_alloc_transfer(0);

        if (!transfer) {
            return LIBUSB_ERROR_NO_MEM;
        }

        libusb_fill_bulk_transfer(transfer, dev->handle, FTDI_EP_OUT(interface),
                                  (unsigned char*)malloc(length), length, WriteAsyncCallback, 0, 0);

        if (!transfer->buffer) {
            libusb_free_transfer(transfer);
            return LIBUSB_ERROR_NO_MEM;
        }

        memcpy(transfer->buffer, data, length);
        err = libusb_submit_transfer(transfer);

    }
    else
    {

//        std::cout << std::endl << "Request with " << length << " bytes: ";
//        std::cout << std::hex;
//        for (int i=0; i<length; i++)
//            std::cout << (unsigned int)data[i] << " ";
//        std::cout << std::dec << std::endl;

        int transferred;
        err = libusb_bulk_transfer(dev->handle, FTDI_EP_OUT(interface),
                                   data, length, &transferred,
                                   FTDI_COMMAND_TIMEOUT);

        //std::cout << "len = " << length << " transfered = " << transferred << std::endl;
    }

    if (err < 0)
        return err;
    else
        return 0;

#endif

}


int
FTDIDevice_WriteByteSync(FTDIDevice *dev, FTDIInterface interface, uint8_t byte)
{
    return FTDIDevice_Write(dev, interface, &byte, sizeof byte, false);
}


int
FTDIDevice_ReadByteSync(FTDIDevice *dev, FTDIInterface interface, uint8_t *byte)
{

#ifdef USE_FTDI

    std::cerr << "ReadByteSync not implemented" << std::endl;
    return -1;

#else

    /*
   * This is a simplified synchronous read, intended for bit-banging mode.
   * Ignores the modem/buffer status bytes, returns just the data.
   *
   */

    uint8_t packet[3];
    int transferred, err;

    err = libusb_bulk_transfer(dev->handle, FTDI_EP_IN(interface),
                               packet, sizeof packet, &transferred,
                               FTDI_COMMAND_TIMEOUT);
    if (err < 0) {
        return err;
    }
    if (transferred != sizeof packet) {
        return -1;
    }

    if (byte) {
        *byte = packet[sizeof packet - 1];
    }

    return 0;


#endif
}


/*
 * Internal callback for one transfer's worth of stream data.
 * Split it into packets and invoke the callbacks.
 */

static void
ReadStreamCallback(struct libusb_transfer *transfer)
{

#ifdef USE_FTDI

    std::cerr << "ReadStreamCallback not implemented" << std::endl;

#else

    FTDIStreamState *state = (FTDIStreamState*)transfer->user_data;
    int err;

    if (state->result == 0) {
        if (transfer->status == LIBUSB_TRANSFER_COMPLETED) {

            int i;
            uint8_t *ptr = transfer->buffer;
            int length = transfer->actual_length;
            int numPackets = (length + FTDI_PACKET_SIZE - 1) >> FTDI_LOG_PACKET_SIZE;

            for (i = 0; i < numPackets; i++) {
                int payloadLen;
                int packetLen = length;

                if (packetLen > FTDI_PACKET_SIZE)
                    packetLen = FTDI_PACKET_SIZE;

                payloadLen = packetLen - FTDI_HEADER_SIZE;
                state->progress.current.totalBytes += payloadLen;

                state->result = state->callback(ptr + FTDI_HEADER_SIZE, payloadLen,
                                                NULL, state->userdata);

                if (state->result)
                    break;

                ptr += packetLen;
                length -= packetLen;
            }

        } else {
            state->result = LIBUSB_ERROR_IO;
        }
    }

    if (state->result == 0) {
        transfer->status = (libusb_transfer_status)-1;
        state->result = libusb_submit_transfer(transfer);
    }

#endif

}


static double
TimevalDiff(const struct timeval *a, const struct timeval *b)
{
    return (a->tv_sec - b->tv_sec) + 1e-6 * (a->tv_usec - b->tv_usec);
}


/*
 * Use asynchronous transfers in libusb-1.0 for high-performance
 * streaming of data from a device interface back to the PC. This
 * function continuously transfers data until either an error occurs
 * or the callback returns a nonzero value. This function returns
 * a libusb error code or the callback's return value.
 *
 * For every contiguous block of received data, the callback will
 * be invoked.
 */

int
FTDIDevice_ReadStream(FTDIDevice *dev, FTDIInterface interface,
                      FTDIStreamCallback *callback, void *userdata,
                      int packetsPerTransfer, int numTransfers)
{

#ifdef USE_FTDI

    FT_STATUS ftStatus = FT_OK;
    DWORD RxBytes = packetsPerTransfer * FTDI_PACKET_SIZE; // ~256*512 -> 128kB
    char RxBuffer[packetsPerTransfer * FTDI_PACKET_SIZE];
    DWORD BytesReceived;

    // vars for progress struct
    FTDIStreamState state = {callback, userdata};
    FTDIProgressInfo *progress = &state.progress;

    const double progressInterval = 0.1;
    struct timeval now;

    gettimeofday(&state.progress.first.time, NULL);

    unsigned int timeout_cnt = 0;
    bool connectionProblemOccured = false;

    FT_SetTimeouts(dev->handle, 1000, 0);

    while (ftStatus == FT_OK)
    {
        //std::cout << "read to buffer" << std::cout;
        ftStatus = FT_Read(dev->handle, &RxBuffer[0], RxBytes, &BytesReceived);

        if (BytesReceived != RxBytes)
        {
            timeout_cnt++;
//            std::cerr << "byte rcv: " << BytesReceived << " rx: " << RxBytes << std::endl;
//            std::cerr << "time out " << timeout_cnt << " occured: quit" << std::endl;
            if (timeout_cnt == 10)
                break;
        }
        else
            timeout_cnt = 0;

        if (ftStatus == FT_OK)
        {
            // If enough time has elapsed, update the progress
            gettimeofday(&now, NULL);

            //std::cout << "time diff: " << TimevalDiff(&now, &progress->current.time) << std::endl;
            if (TimevalDiff(&now, &progress->current.time) >= progressInterval)
            {
                progress->current.time = now;

                if (progress->prev.totalBytes)
                {
                    // We have enough information to calculate rates
                    double currentTime;
                    progress->totalTime = TimevalDiff(&progress->current.time, &progress->first.time);
                    currentTime = TimevalDiff(&progress->current.time, &progress->prev.time);
                    progress->totalRate = progress->current.totalBytes / progress->totalTime;
                    progress->currentRate = (progress->current.totalBytes - progress->prev.totalBytes) / currentTime;
                }
            }

            state.result = state.callback((uint8_t*)&RxBuffer[0], BytesReceived, progress, state.userdata);
            progress->prev = progress->current;
        }
        else
        {
            connectionProblemOccured = true;
            std::cout << "reading data: failed" << std::endl;
        }

        if (state.result)
            break;
    }

#if defined(WIN32) && defined(USE_FTDI)

    if (!connectionProblemOccured) // following only makes sense, if connection is ok
    {
        // Change the ftdi mode from ft245 to ft232 on exit.
        // This way we keept compatible with config software.
        std::cout << std::endl << "Change mode on windows exit" << std::endl;

        ftStatus = FT_ResetDevice(dev->handle);

        if (ftStatus == FT_OK)
            std::cout << "Device mode reset on exit done" << std::endl;

        else
            std::cout << "Device mode reset on exit failed" << std::endl;
    }

#endif


    ftStatus = FT_Close(dev->handle);

    if (ftStatus == FT_OK)
    {
        std::cout << "device cleanly closed" << std::endl;
        return 0;
    }
    else
    {
        std::cout << "error occured on device closed" << std::endl;
        return -1;
    }

#else

    struct libusb_transfer **transfers;
    FTDIStreamState state = { callback, userdata };
    int bufferSize = packetsPerTransfer * FTDI_PACKET_SIZE;
    int xferIndex;
    int err = 0;
//std::cout << "----------------------> 1" << std::endl;
    /*
    * Set up all transfers
    */
    transfers = (libusb_transfer**)calloc(numTransfers, sizeof *transfers);

    if (!transfers) {
        err = LIBUSB_ERROR_NO_MEM;
        goto cleanup;
    }
//std::cout << "----------------------> 2" << std::endl;
    for (xferIndex = 0; xferIndex < numTransfers; xferIndex++)
    {
        struct libusb_transfer *transfer;

        transfer = libusb_alloc_transfer(0);
        transfers[xferIndex] = transfer;
        if (!transfer) {
            err = LIBUSB_ERROR_NO_MEM;
            goto cleanup;
        }

        libusb_fill_bulk_transfer(transfer, dev->handle, FTDI_EP_IN(interface),
                                  (unsigned char*)malloc(bufferSize), bufferSize, ReadStreamCallback,
                                  &state, 0);

        if (!transfer->buffer) {
            err = LIBUSB_ERROR_NO_MEM;
            goto cleanup;
        }

        transfer->status = (libusb_transfer_status)-1;
        err = libusb_submit_transfer(transfer);
        if (err)
            goto cleanup;
    }
//std::cout << "----------------------> 3" << std::endl;
    /*
    * Run the transfers, and periodically assess progress.
    */
    gettimeofday(&state.progress.first.time, NULL);

    do {
        FTDIProgressInfo  *progress = &state.progress;
        const double progressInterval = 0.1;
        struct timeval timeout = { 0, 10000 };
        struct timeval now;

        int err = libusb_handle_events_timeout(dev->libusb, &timeout);
        if (!state.result) {
            state.result = err;
//            std::cout << "----------------------> 4.1" << std::endl;
        }
//        else
//            std::cout << "----------------------> 4.2" << std::endl;

        // If enough time has elapsed, update the progress
        gettimeofday(&now, NULL);
        if (TimevalDiff(&now, &progress->current.time) >= progressInterval) {
//std::cout << "----------------------> 4.3" << std::endl;
            progress->current.time = now;

            if (progress->prev.totalBytes) {
                // We have enough information to calculate rates

                double currentTime;

                progress->totalTime = TimevalDiff(&progress->current.time,
                                                  &progress->first.time);
                currentTime = TimevalDiff(&progress->current.time,
                                          &progress->prev.time);

                progress->totalRate = progress->current.totalBytes / progress->totalTime;
                progress->currentRate = (progress->current.totalBytes -
                                         progress->prev.totalBytes) / currentTime;
            }

            state.result = state.callback(NULL, 0, progress, state.userdata);
            progress->prev = progress->current;
        }
//        else
//            std::cout << "----------------------> 4.4" << std::endl;
    } while (!state.result);

    /*
    * Cancel any outstanding transfers, and free memory.
    */
//std::cout << "----------------------> 5" << std::endl;
cleanup:
    if (transfers) {
        bool done_cleanup = false;
        while (!done_cleanup)
        {
            done_cleanup = true;

            for (xferIndex = 0; xferIndex < numTransfers; xferIndex++) {
                struct libusb_transfer *transfer = transfers[xferIndex];

                if (transfer) {
                    // If a transfer is in progress, cancel it
                    if (transfer->status == -1) {
                        libusb_cancel_transfer(transfer);

                        // And we need to wait until we get a clean sweep
                        done_cleanup = false;

                        // If a transfer is complete or cancelled, nuke it
                    } else if (transfer->status == 0 ||
                               transfer->status == LIBUSB_TRANSFER_CANCELLED) {
                        free(transfer->buffer);
                        libusb_free_transfer(transfer);
                        transfers[xferIndex] = NULL;
                    }
                }
            }

            // pump events
            struct timeval timeout = { 0, 10000 };
            libusb_handle_events_timeout(dev->libusb, &timeout);
        }
        free(transfers);
    }


    if (err)
    {
        std::cout << "error occured on device closed" << std::endl;
        return err;
    }
    else
    {
        std::cout << "device cleanly closed" << std::endl;
        return state.result;
    }

#endif

}

/* MPSSE mode support -- see
 * http://www.ftdichip.com/Support/Documents/AppNotes/AN_108_Command_Processor_for_MPSSE_and_MCU_Host_Bus_Emulation_Modes.pdf
 */

int
FTDIDevice_MPSSE_Enable(FTDIDevice *dev, FTDIInterface interface)
{
    int err;

    /* Reset interface */

    err = FTDIDevice_SetMode(dev, interface, FTDI_BITMODE_RESET, 0, 0);
    if (err)
        return err;

    /* Enable MPSSE mode */

    err = FTDIDevice_SetMode(dev, interface, FTDI_BITMODE_MPSSE,
                             FTDI_SET_BITMODE_REQUEST, 0);

    return err;
}

int
FTDIDevice_MPSSE_SetDivisor(FTDIDevice *dev, FTDIInterface interface,
                            uint8_t ValueL, uint8_t ValueH)
{
    uint8_t buf[3] = {FTDI_MPSSE_SETDIVISOR, 0, 0};

    buf[1] = ValueL;
    buf[2] = ValueH;

    return FTDIDevice_Write(dev, interface, buf, 3, false);
}

int
FTDIDevice_MPSSE_SetLowByte(FTDIDevice *dev, FTDIInterface interface, uint8_t data, uint8_t dir)
{
    uint8_t buf[3] = {FTDI_MPSSE_SETLOW, 0, 0};

    buf[1] = data;
    buf[2] = dir;

    return FTDIDevice_Write(dev, interface, buf, 3, false);
}

int
FTDIDevice_MPSSE_SetHighByte(FTDIDevice *dev, FTDIInterface interface, uint8_t data, uint8_t dir)
{
    uint8_t buf[3] = {FTDI_MPSSE_SETHIGH, 0, 0};

    buf[1] = data;
    buf[2] = dir;

    return FTDIDevice_Write(dev, interface, buf, 3, false);
}

int
FTDIDevice_MPSSE_GetLowByte(FTDIDevice *dev, FTDIInterface interface, uint8_t *byte)
{
    int err;

    err = FTDIDevice_WriteByteSync(dev, interface, FTDI_MPSSE_GETLOW);
    if (err)
        return err;

    return FTDIDevice_ReadByteSync(dev, interface, byte);
}

int
FTDIDevice_MPSSE_GetHighByte(FTDIDevice *dev, FTDIInterface interface, uint8_t *byte)
{
    int err;

    err = FTDIDevice_WriteByteSync(dev, interface, FTDI_MPSSE_GETHIGH);
    if (err)
        return err;

    return FTDIDevice_ReadByteSync(dev, interface, byte);
}
