#ifndef TAUCOM_H
#define TAUCOM_H

#include <inttypes.h>

#define TAU_NO_OP_CMD 0x00
#define TAU_CAMERA_RESET_CMD 0x02
#define TAU_SERIAL_NUMBER_CMD 0x04
#define TAU_GET_REVISION_CMD 0x05
#define TAU_DO_FFC_CMD 0x0C

#define TAU_DIGITAL_OUTPUT_MODE_CMD 0x12
#define TAU_DIGITAL_OUTPUT_MODE_ENABLED 0x0000
#define TAU_DIGITAL_OUTPUT_MODE_DISABLED 0x0002

#define TAU_READ_SENSOR_CMD 0x20
#define TAU_READ_SENSOR_DEGREE_CELSIUS 0x0000 // c*10

#define TAU_CAMERA_PART_CMD 0x66 //  get camera part number


class TauCom
{

public:

    void parseInFrame(uint8_t* in_buffer,uint32_t size);

    int genTauFrameNoValue(uint8_t function,uint8_t * out_buf);
    int genTauFrame16BitValue(uint8_t function,uint16_t data,uint8_t * out_buf);
    int genTauFrame32BitValue(uint8_t function,uint32_t data,uint8_t * out_buf);
    int genTauFrame(uint8_t function,uint8_t data_size,uint8_t* data,uint8_t* out_buf);

    TauCom();

    virtual void processSerialPacket(uint8_t* buffer,uint32_t size)=0;

private:

    uint32_t packet_index;
    uint8_t packet_buffer[512];
    uint32_t packet_size;

    char* decodeStatusCode(uint8_t code);
    int checkInFrame(uint8_t* in_buffer,uint32_t in_size);

};

#endif // TAUCOM_H
