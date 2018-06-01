#include <taucom.h>
#include <iostream>
#include <crc.h>

#define TAU_FRAME_START 0x6e

struct status_code
{
    uint8_t code;
    char* text;
};

struct status_code status_codes[]={
{0x00,(char*)"CAM_OK"},
{0x02,(char*)"CAM_NOT_READY"},
{0x03,(char*)"CAM_RANGE_ERROR"},
{0x04,(char*)"CAM_CHECKSUM_ERROR"},
{0x05,(char*)"CAM_UNDEFINED_PROCESS_ERROR"},
{0x06,(char*)"CAM_UNDEFINED_FUNCTION_ERROR"},
{0x07,(char*)"CAM_TIMEOUT_ERROR"},
{0x09,(char*)"CAM_BYTE_COUNT_ERROR"},
{0x0A,(char*)"CAM_FEATURE_NOT_ENABLED"},
{0x00,NULL }
};

TauCom::TauCom()
{
    packet_index=0;
    packet_size=0;
}


void TauCom::parseInFrame(uint8_t* in_buffer,uint32_t size)
{
    uint32_t i=0;

    while(i<size)
    {
        //overflow - reset parser
        if(packet_index==sizeof(packet_buffer))
        {
            packet_index=0;
        }

        if(packet_index==0)
        {
            if(in_buffer[i]==TAU_FRAME_START)
            {
                packet_buffer[packet_index++]=in_buffer[i++];
            }
            else
            {
                i++;
            }
        }
        else
        {
            packet_buffer[packet_index++]=in_buffer[i++];
        }

        if(packet_index==7)
        {
            packet_size=((packet_buffer[4]<<8)|(packet_buffer[5]))+10;
        }
        else if(packet_index>7)
        {
            if(packet_index==packet_size)
            {
                if(checkInFrame(packet_buffer,packet_size))
                {
                    processSerialPacket(packet_buffer, packet_size);
                }
                else
                {
                    std::cerr << "Error: Frame check failed" << std::endl;
                }
                packet_index=0;
            }
        }
    }
}

char* TauCom::decodeStatusCode(uint8_t code)
{
    uint8_t code_found=0;
    uint32_t i=0;

    while((code_found==0)&&(status_codes[i].text!=NULL))
    {
        if(status_codes[i].code==code)
        {
            code_found=1;
        }
        else
        {
            i++;
        }
    }

    if(code_found)
    {
        return status_codes[i].text;
    }
    else
    {
        return (char*)"";
    }

}


int TauCom::checkInFrame(uint8_t* in_buffer,uint32_t in_size)
{
    if(in_size<6)
    {
        std::cerr << " TauCom: Data to short\n" << std::endl;
        return 0;
    }

    uint16_t crc1=calc_crc(in_buffer,6);

    if(((crc1>>8)!=in_buffer[6])||((crc1&0xFF)!=in_buffer[7]))
    {
        std::cerr << " TauCom: Header CRC error\n" << std::endl;
        return 0;
    }

    if(in_buffer[1]!=0)
    {
        std::cerr << " TauCom: Error:" << in_buffer[1] << ":" << decodeStatusCode(in_buffer[1]) << std::endl;
        return 0;
    }

    uint32_t data_length=(in_buffer[4]<<8)|in_buffer[5];

    if(data_length>500)
    {
        std::cerr << " TauCom: Data overflow" << std::endl;
    }

    uint16_t crc2=calc_crc(in_buffer+8,data_length);

    if(((crc2>>8)!=in_buffer[data_length+8])||((crc2&0xFF)!=in_buffer[data_length+9]))
    {
        std::cerr << " TauCom: Data CRC error" << std::endl;
        return 0;
    }

    return 1;
}

int TauCom::genTauFrameNoValue(uint8_t function,uint8_t * out_buf)
{
    return genTauFrame(function,0,NULL,out_buf);
}



int TauCom::genTauFrame16BitValue(uint8_t function,uint16_t data,uint8_t * out_buf)
{
    uint8_t dataBuf[2];

    dataBuf[0]=data>>8;
    dataBuf[1]=data;

    return genTauFrame(function,2,dataBuf,out_buf);
}

int TauCom::genTauFrame32BitValue(uint8_t function,uint32_t data,uint8_t * out_buf)
{
    uint8_t dataBuf[4];

    dataBuf[0]=data>>24;
    dataBuf[1]=data>>16;
    dataBuf[2]=data>>8;
    dataBuf[3]=data;

    return genTauFrame(function,4,dataBuf,out_buf);
}

int TauCom::genTauFrame(uint8_t function,uint8_t data_size,uint8_t* data,uint8_t* out_buf)
{
    out_buf[0]=0x6E;
    out_buf[1]=0;
    out_buf[2]=0;
    out_buf[3]=function;
    out_buf[4]=0x00;
    out_buf[5]=data_size;

    uint16_t crc1=calc_crc(out_buf,6);

    out_buf[6]=crc1>>8;
    out_buf[7]=crc1;

    uint32_t i=0;

    for(i=0;i<data_size;i++)
    {
        out_buf[8+i]=data[i];
    }

    uint16_t crc2=calc_crc(out_buf,8+data_size);

    out_buf[8+data_size]=crc2>>8;
    out_buf[9+data_size]=crc2;

    return 10 + data_size;
}





