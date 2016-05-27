/*

  Swagger - A tool for programming ARM processors using the SWD protocol

  Niels A. Moseley

  This file holds all the information to describe the communication
  protocol between the PC and the programming device.

  All structures are little endian and have byte packing, i.e.
  no additional stuffing applied.

  All packets are COBS encoded (https://en.wikipedia.org/wiki/Consistent_Overhead_Byte_Stuffing)
  When present, zero bytes are removed from the payload data, allowing
  the zero byte to be used as a packet delimiter. This greatly simplifies
  reading data over asynchronous interfaces.

  The protocol used is a very light-weight; all the intelligence
  is in the PC, not the programmer hardware. The programmer hardware
  listens to the PC for commands and does not initiate anything
  by itself.

  The PC sends a command in the form of the HardwareTXCommand
  to the programmer. The programmer executes this command and
  sends the result back to the PC using the HardwareRXCommand.

  Although the PC side is programmed in C++, the programmer
  might have a C-only compiler. Therefore, this file needs
  to be compilable in C mode.

*/

#ifndef protocol_include_h
#define protocol_include_h

// *********************************************************
// ** Define command types for HardwareTXCommand structure
// *********************************************************

#define TXCMD_TYPE_UNKNOWN      0   // uninitialized structure
#define TXCMD_TYPE_RESETPIN     1   // set /RESET pin state
#define TXCMD_TYPE_READREG      2   // read a register
#define TXCMD_TYPE_WRITEREG     3   // write a register
#define TXCMD_TYPE_CONNECT      4   // connect to target, return IDCODE.
#define TXCMD_TYPE_GETPROGID    100 // get programmer ID string (appended after HardwareRXCommand struct)

#define RXCMD_STATUS_UNKNOWN    0   // unintialized structure
#define RXCMD_STATUS_OK         1   // command processed OK
#define RXCMD_STATUS_FAIL       2   // command failed
#define RXCMD_STATUS_PROTOERR   3   // protocol/framing error
#define RXCMD_STATUS_TARGETLOST 4   // can't commmunicate with target CPU

// SWD status code, as defined by the ARM standard
#define SWDCODE_ACK  1
#define SWDCODE_WAIT 2
#define SWDCODE_FAIL 4


#pragma pack(push,1)


/** format of packets sent to the hardware interface
    Note that this data will be encapsulated in a
    COBS packet. */
typedef struct
{
    uint8_t     cmdType;    // packet command type (see defines)
    uint8_t     address;    // bits [2:3] of register address stored in bits [1:0]
    uint8_t     APnDP;      // Address or data register access (1 = address)
    uint32_t    data;       // reset line state or write register data
} HardwareTXCommand;


/** format of packets received from the hardware interface
    Note that this data will be encapsulated in a
    COBS packet. */
typedef struct
{
    uint8_t     status;     // status of communication
    uint8_t     swdcode;    // swd return code
    uint32_t    data;       // read register data
} HardwareRXCommand;


#pragma pack(pop)

#endif // sentry

