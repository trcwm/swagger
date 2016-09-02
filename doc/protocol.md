# Swagger programming protocol


Version 1.0
---

#Introduction
This document describes the protocol used by the host PC to communicate with the programming hardware. 

##Protocol summary
The protocol is summarised as follows:

* The protocol is based on packets.
* [COBS encoding](https://en.wikipedia.org/wiki/Consistent_Overhead_Byte_Stuffing) in combination with a zero terminator is used to signify an end-of-packet.
* Each packet contains one or more commands.
* Upon completion of the commands, the programming hardware responds with a result packet.
* The result packet always contains a status code.
* The result packet may contain additional data, such as results of a read operation.
* During the execution of the commands, no further packets will be accepted by the programming hardware.
* All words are little-endian -- deal with it.

##Definitions

Packets originating from the host PC are termed _host packets_. Packets originating from the programming hardware are termed _client packets_.


##Hardware requirements 
The programming hardware is connected to the target's single-wire debug (SWD) port using the following pins:

* SWDIO (bi-directional data line)
* SWCLK (the SWD clock)
* /RESET (the target reset, active low)
* GND (the target ground)

The programming hardware must implement a time-out on all commands. 

##Host packets

All host packets have the following layout (before COBS encoding, excluding the 0x00 terminator):

	< cmd ID:uint8 >[command payload] .. more commands .. < 0x00:uint8 >

| Cmd ID | Short | Payload | Result |
|--------|---------------|----------------------------------------------------------------|
| 0x00   | CONNECT | _none_ | < idcode:u32 > |
| 0x01   | RESET | < state:u8 > | _none_ |
| 0x02   | READ ACCESS PORT  | < portNum:u32 > | < value:u32 > |
| 0x03   | WRITE ACCESS PORT  | < portNum:u32 > < value:u32 > | _none_ |
| 0x04   | READ DEBUG PORT  | < portNum:u8 >  | < value:u32 > |
| 0x05   | WRITE DEBUG PORT  | < portNum:u8 > < value:u32 > | _none_ |
| 0x06   | READ MEMORY  | < addr:u32 >  | < value:u32 > |
| 0x07   | WRITE MEMORY  | < addr:u32 >  < value:u32 >| _none_ |
| 0x08   | WAIT MEMORY TRUE | < addr:u32 >  < mask:u32 >| < result:u8 > |
| 0xFF   | GET INTERFACE INFO | _none_ | < protoVer:u8 > < rxBufSize:u16 > |

###Execution of commands

The commands in each packet are executed in order. When one of the commands fail, the execution of futher commands is aborted and a packet containing an error status code is generated.

The SWD interface may generate an SWD_WAIT response. When this happens, the debugging hardware must re-issue the command until it succeeds (SWD_OK), it fails (SWD_FAIL) or a time-out condition is triggered.

Some commands generate data. This data is appended to the client packet. The client packet must contain result data of all the commands that succeeded, even though there was an error.

### CMD 0X00: CONNECT
This commands sends a special SWD sequence to reset the SWD controller. To check data can be exchanged, the command also queries the target device for its IDCODE. The IDCODE is returned as a u32.

If this command fails due to something other than an SWD error, 0xFFFFFFFF must be returned as the IDCODE.

### CMD 0x01: RESET
This command controls the state of the target's /RESET pin. When < state > is 0, the target is taken out of reset. When < state > is greater than zero, the target is put in reset.

This command will always succeed.

### CMD 0x02: READ ACCESS PORT
This command reads a 32-bit word from an access port specified by < portNum >. A 32-bit result is returned.

### CMD 0x03: WRITE ACCESS PORT
This command writes a 32-bit word to an access port specified by < portNum >.

### CMD 0x04: READ DEBUG PORT
This command reads a 32-bit word from an debug port specified by < portNum >. A 32-bit result is returned.

### CMD 0x05: WRITE DEBUG PORT
This command writes a 32-bit word to an debug port specified by < portNum >.

### CMD 0x06: READ MEMORY
This command reads a 32-bit word from a memory address specified by < addr >. A 32-bit result is returned.

### CMD 0x07: WRITE MEMORY
This command writes a 32-bit word to a memory address specified by < addr >. The 32-bit data is given by < value >.

### CMD 0x08: WAIT MEMORY TRUE
This command waits until a specific 32-bit data pattern is present at a memory address specified by < addr >. Returns OK if (memdata & mask) == mask,
or TIME-OUT if not found within 100(?) retries.

### CMD 0xFF: GET INTERFACE INFO
This command queries the programming hardware for its supported version number and the receive buffer size (in bytes). Issuing this command is the recommended way of identifying that the hardware is listening on the selected COM port.

Notes:
* The receive buffer must be at least 32 bytes large.
* The protocol version must return 0x01.

##Client packets

All client packets have the following layout (before COBS encoding, excluding the 0x00 terminator):

	< status code:uint8 >[result] .. more results .. < 0x00:uint8 >

The status code must be one of the following:

| Status code | Short | Description |
|--------|---------------|----------------------------------------------------------------|
| 0x00   |     OK        | All commands were succesfully executed  |
| 0x01   |     TIME-OUT  | One of the commands caused a time-out   |
| 0x02   |    SWD FAULT  | One of the commands caused an SWD FAULT |
| 0x03   |   RX OVERFLOW | The client's receive buffer overflowed  |
| 0x04   |    PROTO ERR  | The client received an erroneous packet |
| 0x05   |   UNKNOWN CMD | An unknown command was encountered      |

For clarity: only one status code is generated for every packet, even if there are multiple commands.

### STATUS 0x00: OK
Everything is ok.
Recommended action: continue being happy.

### STATUS 0x01: TIME-OUT
A command continued to receive an SWD_WAIT response and timed out, or a WAIT MEMORY command timed out.
Recommended action: (reset the system and) retry, or give up.

### STATUS 0x02: SWD FAULT
An SWD command responded with SWD_FAULT. 
Recommended action: reset the sticky error bits by writing to the ABORT data port and try again.

### STATUS 0x03: RX OVERFLOW
The host sent a packet that didn't fit into the debug hardware's receive buffer.
Recommended action: use smaller/more packets and retry.

### STATUS 0x04: PROTO ERR
The COBS decoder on the debug hardware detected a protocol error.
Recommended action: (reset the system and) retry, make communication more robust, debug your code, or give up.

### STATUS 0x05: UNKNOWN CMD
The commmand interpreter running on the debug hardware encountered a command not defined in the supported version of the protocol. This may be a communication/reliability problem.
Recommended action: (reset the system and) retry, make communication more robust, debug your code, or give up.
