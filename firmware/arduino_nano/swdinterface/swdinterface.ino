
/*
 *  SWD interface code for Arduino Nano
 * 
 *  Niels A. Moseley
 * 
 */
 
#include <stdint.h>

// Note: The arduino include systems is severely borked
// you must make a special library directory in the
// Arduino install directory, e.g. C:\Program Files (x86)\Arduino\libraries\swagger
// can copy the include/protocol.h file in there.. :-/
// Then add it using the Sketch->Add Library option.
//

#include "protocol.h"
#include "mid_level.h"

#define LEDPIN 13

uint8_t g_rxbuffer[32]; // packet receive buffer
uint8_t g_idx = 0;      // write index into receive buffer

const uint8_t SWD_OK=1;
const uint8_t SWD_WAIT=2;
const uint8_t SWD_FAIL=4;

/*************************************************************
 * 
 *  Communications stuff
 * 
 */

ArduinoSWDInterface *g_interface = 0;

// Arduino programmer packet
const uint8_t programmerNamePacket[] = 
{
  0x1D,0x01,0x01,0x01,0x01,0x01,0x01,0x41,0x72,0x64,0x75,0x69,0x6E,0x6F,0x20,
  0x55,0x6E,0x6F,0x20,0x50,0x72,0x6F,0x67,0x72,0x61,0x6D,0x6D,0x65,0x72,0x01,0x00
};

// *********************************************************
//   COBS decoder (source: wikipedia)
// *********************************************************

void UnStuffData(const uint8_t *ptr, uint8_t N, uint8_t *dst)
{
  const uint8_t *endptr = ptr + N;
  while (ptr < endptr)
  {
    uint8_t code = *ptr++;

    for (uint32_t i=1; (ptr<endptr) && (i<code); i++)
      *dst++ = *ptr++;
      
    if (code < 0xFF)
      *dst++ = 0;
  }
}

// *********************************************************
//   COBS encoder (source: wikipedia)
// *********************************************************

#define FinishBlock(X) (*code_ptr = (X), code_ptr = dst++, code = 0x01)

void StuffData(const uint8_t *ptr, uint8_t N, uint8_t *dst)
{
  const unsigned char *endptr = ptr + N;
  unsigned char *code_ptr = dst++;
  unsigned char code = 0x01;

  while (ptr < endptr)
  {
    if (*ptr == 0)
      FinishBlock(code);
    else
    {
      *dst++ = *ptr;
      if (++code == 0xFF)
        FinishBlock(code);
    }
    ptr++;
  }

  *dst = 0;
  FinishBlock(code);  
}




// *********************************************************
//   Send reply
// *********************************************************

void reply(uint8_t rxcmd_status, uint32_t data)
{
  // Send COBS encoded packet
  HardwareRXCommand cmd;
  cmd.status  = rxcmd_status;
  cmd.data    = data;
  uint8_t encodebuffer[sizeof(HardwareRXCommand)+2];
  StuffData((uint8_t*)&cmd, sizeof(cmd), encodebuffer);
  Serial.write((char*)encodebuffer, strlen((char*)encodebuffer)+1);
  Serial.flush(); // wait for TX to be done. (do we need this?)
}

// *********************************************************
//   Main program
// *********************************************************

void setup() 
{
  // put your setup code here, to run once:
  Serial.begin(19200);
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
  pinMode(SWDCLK_PIN, OUTPUT);

  g_interface = new ArduinoSWDInterface();
  g_interface->initPins();
}

void loop() 
{
  // read data from the serial port
  // until we get a zero byte
  // now we have a complete packet 
  // in the buffer
  while (Serial.available()) 
  {
    uint8_t byteRead = Serial.read();
    if (byteRead == 0)
    {
      digitalWrite(13, HIGH);      
      //decode COBS packet
      if (g_idx < sizeof(g_rxbuffer))
      {
        uint8_t decodebuffer[sizeof(g_rxbuffer)]; // decode bytes is always less than encoded bytes..
        UnStuffData(g_rxbuffer, g_idx, decodebuffer);
        HardwareTXCommand *cmd = (HardwareTXCommand *)&decodebuffer;

        // execute command
        uint32_t data;
        uint8_t stat;
        switch(cmd->cmdType)
        {          
          default:
          case TXCMD_TYPE_UNKNOWN:
            reply(RXCMD_STATUS_FAIL, cmd->cmdType);
            break;
          case TXCMD_TYPE_RESETPIN:
            // as of yet unsupported
            reply(RXCMD_STATUS_OK, 0);
            break;
          case TXCMD_TYPE_CONNECT:
            data = 0xDEADBEAF;
            stat = g_interface->doConnect(data);
            reply((stat==SWD_OK) ? RXCMD_STATUS_OK : RXCMD_STATUS_FAIL, data);  // return IDCODE
            break;
          case TXCMD_TYPE_READDP:
            stat = g_interface->readDP(cmd->address, data);
            reply(stat ? RXCMD_STATUS_OK : RXCMD_STATUS_FAIL, data);
            break;
          case TXCMD_TYPE_WRITEDP:
            stat = g_interface->writeDP(cmd->address, cmd->data);
            reply(stat ? RXCMD_STATUS_OK : RXCMD_STATUS_FAIL, 0);
            break;            
          case TXCMD_TYPE_READAP:
            stat = g_interface->readAP(cmd->address, data);
            reply(stat ? RXCMD_STATUS_OK : RXCMD_STATUS_FAIL, data);
            break;
          case TXCMD_TYPE_WRITEAP:
            stat = g_interface->writeAP(cmd->address, cmd->data);
            reply(stat ? RXCMD_STATUS_OK : RXCMD_STATUS_FAIL, 0);
            break;
          case TXCMD_TYPE_READMEM:
            stat = g_interface->readMemory(cmd->address, data);
            reply(stat ? RXCMD_STATUS_OK : RXCMD_STATUS_FAIL, data);
            break;
          case TXCMD_TYPE_WRITEMEM:
            stat = g_interface->writeMemory(cmd->address, cmd->data);
            reply(stat ? RXCMD_STATUS_OK : RXCMD_STATUS_FAIL, 0);
            break;
          case TXCMD_TYPE_GETPROGID:
            Serial.write((char*)programmerNamePacket, strlen((char*)programmerNamePacket)+1);
            Serial.flush();
            break;
        }
      }
      g_idx = 0;
    }
    else
    {
      if (g_idx >= sizeof(g_rxbuffer))
      {
        //FIXME: better handling of buffer overruns ?
        g_idx = 0; // avoid buffer overrun
      }

      g_rxbuffer[g_idx++] = byteRead;
    }
  }
}

