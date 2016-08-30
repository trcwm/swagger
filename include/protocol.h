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

#include <stdint.h>

// *********************************************************
// ** Define command types for HardwareTXCommand structure
// *********************************************************

#define TXCMD_TYPE_CONNECT      0   // connect to target
#define TXCMD_TYPE_RESET        1   // set /RESET pin state
#define TXCMD_TYPE_READAP       2   // read access port register
#define TXCMD_TYPE_WRITEAP      3   // write access port register
#define TXCMD_TYPE_READDP       4   // read access port register
#define TXCMD_TYPE_WRITEDP      5   // write access port register
#define TXCMD_TYPE_READMEM      6   // read a memory address
#define TXCMD_TYPE_WRITEMEM     7   // write to a memory address
#define TXCMD_TYPE_WAITMEMTRUE  8   // wait for memory contents
#define TXCMD_TYPE_WAITMEMFALSE 9   // wait for memory contents

#define TXCMD_TYPE_GETPROGID    0xFF // get programmer ID string (appended after HardwareRXCommand struct)

#define RXCMD_STATUS_OK         0   // command OK
#define RXCMD_STATUS_TIMEOUT    1   // command time out
#define RXCMD_STATUS_SWDFAULT   2   // SWD fault
#define RXCMD_STATUS_RXOVERFLOW 3   // RX overflow
#define RXCMD_STATUS_PROTOERR   4   // Protocol error
#define RXCMD_STATUS_UNKNOWNCMD 5   // Unknown command

#endif // sentry

