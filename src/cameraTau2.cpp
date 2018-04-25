#include "cameraTau2.h"

using namespace std;
using namespace  LibSerial;

//CameraTau2 CameraTau2::instance;

bool CameraTau2::open(std::string portname)
{
    m_serPort.Open(portname);
    m_serPort.SetBaudRate( SerialStreamBuf::BAUD_57600 );
	m_serPort.SetCharSize(SerialStreamBuf::CHAR_SIZE_8);
	m_serPort.SetParity(SerialStreamBuf::PARITY_NONE);
	m_serPort.SetNumOfStopBits(1);

	return isOpened();
}

unsigned short CameraTau2::send_request(const unsigned char fnumber, const unsigned short argument,const bool set){

    if(!m_serPort)
        cout << " begin Error port" << endl;

    cout << "request : ";
    int nbytes = build_request(fnumber,argument,set);
    for(int i=0;i<nbytes;i++)
        cout << (int) m_request[i] << " ";
    m_serPort.write((char*)&m_request[0],nbytes);
    cout << endl;


    if(!m_serPort)
        cout << " write Error port" << endl;
//    while(!m_serPort){
//        m_serPort.read(m_response,BUFFER_SIZE);
    cout << "response: ";
//    for(int i=0;i<6;i++){
//        char test;m_serPort >> test;
//        cout << (int) (unsigned char) test << " ";
//    }
    int i=0;
    while(m_serPort.get(m_response[i]) && i < 6){
        cout << (int) (unsigned char)m_response[i] << " ";
        i++;
    }

    while(m_serPort.get(m_response[i]) && i < 8+(int)m_response[5]){
        cout << (int) (unsigned char) m_response[i] << " ";
        i++;
    }cout << endl;

    unsigned short result;
    if(m_response[5] > 0)
        result = ((unsigned char)(m_response[7]) << 8 | (unsigned char)(m_response[8]));

//    while(test !=0x6E){
//        cout << "err" << endl;
//    }
//    cout << "test: " << (int)test << endl;
//    }

        if(!m_serPort)
            cout << " read Error port" << endl;
        cout << "test: " << (int) m_response[0] << " " << (int) m_response[2] << endl;
        switch(m_response[1]){
            case 0x03:
                cerr << "CAM_RANGE_ERROR" << endl;
                break;
            case 0x04:
                cerr << "CAM_CHECKSUM_ERROR" << endl;
                break;
            case 0x05:
                cerr << "CAM_UNDEFINED_PROCESS_ERROR" << endl;
                break;
            case 0x06:
                cerr << "CAM_UNDEFINED_FUNCTION_ERROR" << endl;
                break;
            case 0x07:
                cerr << "CAM_TIMEOUT_ERROR" << endl;
                break;
            case 0x09:
                cerr << "CAM_BYTE_COUNT_ERROR" << endl;
            case 0x0A:
                cerr << "CAM_FEATURE_NOT_ENABLE" << endl;
                break;
        }
//    }
//    else
//        cout << "error reading response." << endl;

    if(!m_serPort)
        cout << " end Error port" << endl;
    else
    {}

    return result;
}

int CameraTau2::build_request(const unsigned char fnumber, const unsigned short argument,const bool set){

    m_request[0] = 0x6E;
    m_request[1] = 0x00;
    m_request[2] = 0x00;
    m_request[3] = (char) fnumber;
    m_request[4] = 0x00;
    if(set)
        m_request[5] = 0x02;
    else
        m_request[5] = 0x00;
    unsigned short crc = computeCRC_CCITT(6);
    m_request[6] = (unsigned char)(crc >> 8);
    m_request[7] = (unsigned char)(crc & 0xFF);
    if(argument == 0xFFFF){
        crc = computeCRC_CCITT(8);
        m_request[8] = (unsigned char)(crc >> 8);
        m_request[9] = (unsigned char)(crc & 0xFF);
        return 10;
    }else{
        m_request[8] = (unsigned char)(argument >> 8);
        m_request[9] = (unsigned char)(argument & 0xFF);
        crc = computeCRC_CCITT(10);
        m_request[10] = (unsigned char)(crc >> 8);
        m_request[11] = (unsigned char)(crc & 0xFF);
        return 12;
    }
}

unsigned short CameraTau2::computeCRC_CCITT(int nb_bytes){
    int crc = 0; char i; unsigned char* ptr = m_request;
    while(--nb_bytes >= 0){
        crc = crc ^ (int) *ptr++ << 8;
        i = 8;
        do{
            if(crc & 0x8000)
                crc = crc << 1 ^ 0x1021;
            else
                crc = crc << 1;
        }while(--i);
    }
    return crc;
}
