#include "tauimagedecoder.h"

#include <iostream>
#include <string.h>

TauRGBBitmap::TauRGBBitmap(unsigned int width, unsigned int height) : width(width), height(height)
{
    if((this->width>0)&&(this->height>0))
    {
        this->data=new unsigned char[this->width*this->height*3];
    }
    else
    {
        this->data=NULL;
    }
}

unsigned char * TauRGBBitmap::scanLine(unsigned int line)
{
    if(data != NULL)
    {
        return data + (width*3*line);
    }
    else
    {
        return NULL;
    }
}

TauRGBBitmap::~TauRGBBitmap()
{
    if(data!=NULL)
        delete [] data;
}

TauRawBitmap::TauRawBitmap(unsigned int w, unsigned int h) : width(w), height(h), pps_timestamp(0), min(0), max(0)
{
    if((w>0)&&(h>0))
    {
        data = new unsigned short[width*height];
    }
    else
    {
        width=0;
        height=0;
        data=NULL;
    }
}


TauRawBitmap::~TauRawBitmap()
{
    if(data!=NULL)
        delete [] data;
}

TauImageDecoder::TauImageDecoder() : mTauCoreResWidth(0), mTauCoreResHeight(0)
{
    mTauRawBitmap = NULL;
}

TauImageDecoder::~TauImageDecoder()
{
}

void TauImageDecoder::decodeData(vector<uint16_t> buffer)
{
    if (buffer.empty())
        return;

    // Assume the first word to already be data
    unsigned int size = buffer.size();
    uint16_t* image_data = buffer.data();

    //std::cerr << "size " << size << " w/h " << mTauCoreResWidth << "/" << mTauCoreResHeight << std::endl;

    // Prepare TauRawBitmap
    mTauRawBitmap = new TauRawBitmap(mTauCoreResWidth, mTauCoreResHeight);

    // If first value starts with '00' (0b00??|????|????|????),
    // it's a pps value.
    uint16_t pps = buffer.at(0);
    unsigned int beg = 0;

    if (!((pps & 0x8000) || (pps & 0x4000)))     // pps found
    {
        beg++;
        mTauRawBitmap->pps_timestamp = pps;    // get the present pss value
    }

    // If there are any sync words (vsync/hsync begin with '01' or '10')
    // -> skip them
    while (((buffer.at(beg) & 0x8000) && !(buffer.at(beg) & 0x4000))
          || (!(buffer.at(beg) & 0x8000) && (buffer.at(beg) & 0x4000)))
    {
        beg++;
    }

    if (beg>=size)
    {
        delete mTauRawBitmap;
        return;
    }

    // adopt entry points appropriatley to the begin of data
    size -= beg; // size may be reduced by ccs values and sync words
    image_data += beg; // entry point for image data

    unsigned int maxValue = 0;
    unsigned int minValue = 65500;

    unsigned int row = 0;
    unsigned int idx = 0;

    unsigned int  wordsToRead = mTauCoreResWidth*mTauCoreResHeight; // one 16bit word for each pixel

    if (wordsToRead > size)
    {
        delete mTauRawBitmap;
        return; //too few data -> can't be a full frame
    }


    uint16_t value; // store present value temporary for calculations
    uint16_t* data = mTauRawBitmap->data; // use a pointer for begin of data field in TauRawBitmap

    for (row=0; row<mTauCoreResHeight; row++)
    {
        //if (idx+mTauCoreResWidth-beg > size) // prevent index out of bounce
        if (idx+mTauCoreResWidth > size) // prevent index out of bounce
        {
            std::cout << "idx+w oob " << idx+mTauCoreResWidth << "/" << size << "/" << buffer.size() << " in row: " << row << std::endl;
            delete mTauRawBitmap;
            return;
        }

        unsigned int baseIdx = row * mTauCoreResWidth;

        for (unsigned int elem=0; elem<mTauCoreResWidth; elem++)
        {
            value = image_data[idx+elem] & 0x3fff; // remove first 2 bits with mask 0b0011 1111 1111 1111
            data[baseIdx + elem] = value; // store in TauRawBitmap data

            if (value > maxValue)
                maxValue = value;

            else if(value < minValue)
                minValue = value;

//            if (value == 0)
//                std::cout << "zero in row " << row << " idx " << elem << std::endl;
        }

        idx += mTauCoreResWidth; // move index to begin of new row

        if (row < mTauCoreResHeight-1) // no searching for new line after last line of image
        {
            // skip invalid data at end of line (if any)
            while ((image_data[idx] & 0x8000) == 0)
            {
                idx++;
                if (idx >= size) // prevent index out of bounce
                {
                    std::cout << "idx oob 1 " << idx << "/" << size << std::endl;
                    delete mTauRawBitmap;
                    return;
                }
            }
        }
    }
//    std::cout << " beg: " << beg << std::endl;

    mTauRawBitmap->min = minValue;
    mTauRawBitmap->max = maxValue;

    frameDecoded(mTauRawBitmap);
}
