/*

  Consistent Overhead Byte Stuffing Encoder/decoder

*/

#include "cobs.h"

bool COBS::encode(const std::vector<uint8_t> &indata, std::vector<uint8_t> &outdata)
{
    size_t end = indata.size();
    size_t idx = 0;
    size_t code_idx = 0;
    uint8_t code = 1;

    outdata.clear();
    outdata.push_back(0);

    while(idx < end)
    {
        if (indata[idx] == 0)
        {
            // Finish block
            outdata[code_idx] = code;
            code_idx = outdata.size();
            outdata.push_back(0);   // will be replaced later
            code = 1;
        }
        else
        {
            outdata.push_back(indata[idx]);
            code++;
            if (code == 0xFF)
            {
                // Finish block
                outdata[code_idx] = code;
                code_idx = outdata.size();
                outdata.push_back(0);   // will be replaced later
                code = 1;
            }
        }
        idx++;
    }

    // Finish last block
    outdata[code_idx] = code;
    outdata.push_back(0);
    return true;
}

bool COBS::decode(const std::vector<uint8_t> &indata, std::vector<uint8_t> &outdata)
{
    size_t end = indata.size();
    size_t idx = 0;

    outdata.clear();
    while(idx < end)
    {
        uint8_t code = indata[idx++];
        for(size_t i=1; (idx<end) && (i<code); i++)
        {
            outdata.push_back(indata[idx++]);
        }
        if (code < 0xFF)
            outdata.push_back(0);
    }
    return true;
}

bool COBS::test()
{
    const uint8_t in1[] = {0x11,0x22,0x33,0x44};
    const uint8_t out1[] = {0x05,0x11,0x22,0x33,0x44,0x00};

    const uint8_t in2[] = {0x11, 0x00, 0x00, 0x00};
    const uint8_t out2[] = {0x02, 0x11, 0x01, 0x01, 0x01, 0x00};

    const uint8_t in3[] = {0x11,0x22,0x00,0x33};
    const uint8_t out3[] = {0x03,0x11,0x22,0x02,0x33,0x00};

    std::vector<uint8_t> indata, outdata, cmpdata;

    // test 1
    indata.insert(indata.end(), in1, in1+sizeof(in1));
    cmpdata.insert(cmpdata.end(), out1, out1+sizeof(out1));
    COBS::encode(indata, outdata);

    if (cmpdata != outdata)
    {
        return false;
    }

    cmpdata.clear();
    outdata.resize(outdata.size()-1);   // remove NULL terminator
    COBS::decode(outdata, cmpdata);
    indata.push_back(0);
    if (cmpdata != indata)
    {
        return false;
    }

    // test 2
    indata.clear();
    cmpdata.clear();
    outdata.clear();
    indata.insert(indata.end(), in2, in2+sizeof(in2));
    cmpdata.insert(cmpdata.end(), out2, out2+sizeof(out2));
    COBS::encode(indata, outdata);

    if (cmpdata != outdata)
    {
        return false;
    }

    cmpdata.clear();
    outdata.resize(outdata.size()-1);   // remove NULL terminator
    COBS::decode(outdata, cmpdata);
    indata.push_back(0);
    if (cmpdata != indata)
    {
        return false;
    }

    // test 3
    indata.clear();
    cmpdata.clear();
    outdata.clear();
    indata.insert(indata.end(), in3, in3+sizeof(in3));
    cmpdata.insert(cmpdata.end(), out3, out3+sizeof(out3));
    COBS::encode(indata, outdata);

    if (cmpdata != outdata)
    {
        return false;
    }

    cmpdata.clear();
    outdata.resize(outdata.size()-1);   // remove NULL terminator
    COBS::decode(outdata, cmpdata);
    indata.push_back(0);
    if (cmpdata != indata)
    {
        return false;
    }

    return true;
}
