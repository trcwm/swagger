/*
 *  SWD interface code for Arduino Nano
 * 
 *  Niels A. Moseley
 * 
 */
 
#include <stdint.h>

// Note: The arduino include systems is severly borked
// you must make a special library directory in the
// Arduino install directory, e.g. C:\Program Files (x86)\Arduino\libraries\swagger
// can copy the protocol.h file in there.. :-/
//

#include "protocol.h"

#define LEDPIN 13

uint8_t g_rxbuffer[32]; // packet receive buffer
uint8_t g_idx = 0;      // write index into receive buffer

// *********************************************************
//   COBS decoder (source: wikipedia)
// *********************************************************

void UnStuffData(const uint8_t *ptr, uint8_t N, uint8_t *dst)
{
  const uint8_t *endptr = ptr + N;
  
  while (ptr < endptr)
  {
    int code = *ptr++;
    
    for (int i=1; i<code; i++)
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

  FinishBlock(code);
}

// *********************************************************
//   Execute Reset
// *********************************************************

bool executeReset(uint8_t pin)
{
  digitalWrite(LEDPIN, (pin>0) ? HIGH:LOW);
  return true;
}

// *********************************************************
//   Execute Read Register
// *********************************************************

bool executeReadRegister(uint8_t APnDP, uint8_t address, uint32_t &data)
{
  data = 0x1234567;
  return true;
}

// *********************************************************
//   Execute Write Register
// *********************************************************

bool executeWriteRegister(uint8_t APnDP, uint8_t address, uint32_t &data)
{
  return true;
}

// *********************************************************
//   Send reply
// *********************************************************

void reply(uint8_t rxcmd_status, uint8_t swdcode, uint32_t data)
{
  // Send COBS encoded packet
  HardwareRXCommand cmd;
  cmd.status  = rxcmd_status;
  cmd.swdcode = swdcode;
  cmd.data    = data;
  uint8_t encodebuffer[sizeof(HardwareRXCommand)+2];
  StuffData((uint8_t*)&cmd, sizeof(cmd), encodebuffer);
  Serial.write((char*)encodebuffer);
  Serial.flush(); // wait for TX to be done. (do we need this?)
}

// *********************************************************
//   Main program
// *********************************************************

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
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
      //decode COBS packet
      if (g_idx < sizeof(g_rxbuffer))
      {
        uint8_t decodebuffer[sizeof(g_rxbuffer)]; // decode bytes is always less than encoded bytes..
        UnStuffData(g_rxbuffer, g_idx, decodebuffer);
        HardwareTXCommand *cmd = (HardwareTXCommand *)&decodebuffer;

        // execute command
        uint32_t data;
        switch(cmd->cmdType)
        {
          default:
          case TXCMD_TYPE_UNKNOWN:
            break;
          case TXCMD_TYPE_RESETPIN:
            if (executeReset(cmd->data))
              reply(RXCMD_STATUS_OK, SWDCODE_ACK, 0x01010101);
            else
              reply(RXCMD_STATUS_FAIL, SWDCODE_ACK, 0x01010101);
            break;
          case TXCMD_TYPE_READREG:
            if (executeReadRegister(cmd->APnDP, cmd->address, data))
            {
              reply(RXCMD_STATUS_OK, SWDCODE_ACK, data);
            }
            else
            {
              reply(RXCMD_STATUS_FAIL, SWDCODE_ACK, 0x01010101);
            }
            break;
          case TXCMD_TYPE_WRITEREG:
            if (executeWriteRegister(cmd->APnDP, cmd->address, cmd->data))
            {
              reply(RXCMD_STATUS_OK, SWDCODE_ACK, data);
            }
            else
            {
              reply(RXCMD_STATUS_FAIL, SWDCODE_ACK, 0x01010101);
            }
            break;
        }
      }
      g_idx = 0;
    }
    else
    {
      if (g_idx < sizeof(g_rxbuffer))
      {
        g_rxbuffer[g_idx++] = byteRead;
      }
      else
      {
        // avoid buffer overrun :-(
        // TODO: send error packet
        g_idx = 0; 
      }
    }
  }
}

