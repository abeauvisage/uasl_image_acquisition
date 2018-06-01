#ifndef TAUIMAGEDECODER_H
#define TAUIMAGEDECODER_H

#include <thermalgrabber.h>

#include <inttypes.h>
#include <vector>

using namespace std;

class TauImageDecoder
{

public:

    TauImageDecoder();
    ~TauImageDecoder();
    virtual void frameDecoded(TauRawBitmap* frame)=0;
    void decodeData(vector<uint16_t> buffer);
    void processRawData(TauRawBitmap* tauRawBitmap, TauRGBBitmap* tRGBBitmap);

protected:

    unsigned int mTauCoreResWidth;
    unsigned int mTauCoreResHeight;

private:

    TauRawBitmap* mTauRawBitmap;
    void processDataChunk(uint16_t image_data[], unsigned int size);
    uint16_t prepare16bitData(uint16_t input);
    bool isDataLineValid(std::vector<unsigned int> &rawLine,
                         int referenceLineWidth,
                         int line_count,
                         uint16_t currentPixelData);
    unsigned int _currentImageHeight;
    unsigned int _currentImageWidth;
    unsigned int _minRawValue;
    unsigned int _maxRawValue;

};

#endif // TAUIMAGEDECODER_H
