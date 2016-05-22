/*

  Consistent Overhead Byte Stuffing Encoder/decoder

*/

#include <vector>
#include <stdint.h>

namespace COBS
{
    /** Encode a byte buffer using COBS encoding.
        A NULL terminator is added.
    */
    bool encode(const std::vector<uint8_t> &indata, std::vector<uint8_t> &outdata);

    /** Decode a byte buffer using COBS decoding,
        The NULL terminator must NOT be included
        in the input data!
        A NULL terminator is added at the end of the decode.
    */
    bool decode(const std::vector<uint8_t> &indata, std::vector<uint8_t> &outdata);

    /** perform built-in testing */
    bool test();
}
