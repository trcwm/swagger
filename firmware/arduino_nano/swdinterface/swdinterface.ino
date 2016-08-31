
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
uint8_t g_rxidx = 0;    // read index into receive buffer

uint8_t g_txbuffer[32]; // packet transmit buffer
uint8_t g_txidx = 1;    // write index into transmit buffer,
                        // leave room for status byte at
                        // the beginning so it can be
                        // added later.

/*************************************************************
 * 
 *  Communications stuff
 * 
 */

ArduinoSWDInterface *g_interface = 0;

// *********************************************************
//   COBS decoder (source: wikipedia)
//
//   Returns the number of bytes decoded.
//
// *********************************************************

uint32_t UnStuffData(const uint8_t *ptr, uint8_t N, uint8_t *dst)
{
  uint32_t bytes = 0;
  const uint8_t *endptr = ptr + N;
  while (ptr < endptr)
  {
    uint8_t code = *ptr++;
    
    for (uint32_t i=1; (ptr<endptr) && (i<code); i++)
    {
      *dst++ = *ptr++;
      bytes++;
    }
      
    if (code < 0xFF)
    {
      *dst++ = 0;
      bytes++;
    }
  }
  return bytes;
}

// *********************************************************
//   COBS encoder (source: wikipedia)
//
//   Appends a zero terminator byte at the end of the 
//   encoded result
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
//   Get a uint32_t from memory address,
//   can be unaligned!
// *********************************************************


uint32_t getUInt32(uint8_t *ptr)
{
  uint32_t w = ptr[0];
  w |= ((uint32_t)ptr[1]) << 8;
  w |= ((uint32_t)ptr[2]) << 16;
  w |= ((uint32_t)ptr[3]) << 24;
  return w;
}

// *********************************************************
//   Queue reply byte
//
//   returns false if TX queue overflowed, else true.
// *********************************************************

bool queueReplyUInt8(uint8_t b)
{
  if (g_txidx >= sizeof(g_txbuffer))
  {
    return false; // TX buffer overflow
  }
  else
  {
    g_txbuffer[g_txidx++] = b;
    return true;
  }
}


// *********************************************************
//   Queue reply 32-bit word
//
//   returns false if TX queue overflowed, else true.
// *********************************************************

bool queueReplyUInt32(uint32_t w)
{
  if ((g_txidx+4) >= sizeof(g_txbuffer))
  {
    return false; // TX buffer overflow
  }
  else
  {
    g_txbuffer[g_txidx++] = w & 0xFF;
    g_txbuffer[g_txidx++] = (w>>8) & 0xFF;
    g_txbuffer[g_txidx++] = (w>>16) & 0xFF;
    g_txbuffer[g_txidx++] = (w>>24) & 0xFF;
    return true;
  }
}


// *********************************************************
//   Send reply
//
//   COBS encodes all the queued data and sends it.
//
// *********************************************************

void sendReply(uint8_t replyStatus)
{
  uint8_t encodebuffer[sizeof(g_txbuffer)+2];
  
  // Send COBS encoded packet 
  g_txbuffer[0] = replyStatus;
  StuffData((uint8_t*)&g_txbuffer, g_txidx, encodebuffer);
  Serial.write((char*)encodebuffer, strlen((char*)encodebuffer)+1);
  Serial.flush(); // wait for TX to be done. (do we need this?)

  g_txidx = 1; // reset tx index, leave room for status byte.
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
  digitalWrite(SWDDAT_PIN, HIGH);
  digitalWrite(RESET_PIN, HIGH);
  digitalWrite(GND_PIN, LOW);
  pinMode(SWDCLK_PIN, OUTPUT);
  pinMode(SWDDAT_PIN, OUTPUT);
  pinMode(RESET_PIN, OUTPUT);

  g_interface = new ArduinoSWDInterface();

  pinMode(GND_PIN, OUTPUT);
  digitalWrite(GND_PIN, LOW);
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

    if (byteRead != 0)
    {
      // store byte, check for overflow! 
      if (g_rxidx >= sizeof(g_rxbuffer))
      {
        // overflow!
        g_txidx = 1; // discard previous information
        g_rxidx = 0; // avoid buffer overrun
        sendReply(RXCMD_STATUS_RXOVERFLOW);
        return;
      }
      g_rxbuffer[g_rxidx++] = byteRead;
    }    
    else
    {      
      digitalWrite(13, HIGH);
      
      //decode COBS packet      
      uint8_t decodebuffer[sizeof(g_rxbuffer)]; // decode bytes is always less than encoded bytes..
      uint32_t bytes = UnStuffData(g_rxbuffer, g_rxidx, decodebuffer);
      bytes--; // remove the zero terminator to avoid triggering additional commands..       
      uint8_t *ptr = (uint8_t*)decodebuffer;
      uint8_t *endptr = ((uint8_t*)decodebuffer) + bytes;
      g_rxidx = 0;

#if 0
      g_txidx = 1;
      for(uint32_t i=0; i<g_rxidx; i++)
      {
        queueReplyUInt8(g_rxbuffer[i]);
      }
      sendReply(RXCMD_STATUS_SWDFAULT);
      g_rxidx = 0; // clear RX buffer
#endif      

      while(ptr < endptr)
      {      
        // execute command
        uint32_t data32, address;
        uint8_t data8;
        uint8_t stat;        
        switch(ptr[0])
        {          
          default:  // unknown command, abort & send reply
            queueReplyUInt32(0xDEADBEEF);
            sendReply(RXCMD_STATUS_UNKNOWNCMD);
            return;
          case TXCMD_TYPE_RESET:
            // set target reset
            g_interface->setReset(ptr[1] > 0);
            ptr += 2;
            break;
          case TXCMD_TYPE_CONNECT:
            stat = g_interface->tryConnect(data32);
            if (stat == RXCMD_STATUS_OK)
            {
              ptr++;              
              queueReplyUInt32(data32); // IDCODE              
            }
            else
            {
              sendReply(stat);
              return;
            }
            break;
          case TXCMD_TYPE_READDP:
            address = ptr[1];
            stat = g_interface->readDP(address, data32);
            if (stat == RXCMD_STATUS_OK)
            {
              queueReplyUInt32(data32);
              ptr+=2;
            }
            else
            {
              sendReply(stat);
              return;
            }
            break;
          case TXCMD_TYPE_WRITEDP:
            address = ptr[1];
            data32 = getUInt32(ptr+2);
            stat = g_interface->writeDP(address, data32);
            if (stat == RXCMD_STATUS_OK)
            {
              ptr+=6;    // 1 cmd byte, 1 byte address, 1 32-bit word
            }
            else
            {
              sendReply(stat);
              return;
            }
            break;           
          case TXCMD_TYPE_READAP:
            address = ptr[1];
            stat = g_interface->readAP(address, data32);
            if (stat == RXCMD_STATUS_OK)
            {
              queueReplyUInt32(data32);
              ptr+=2;    // 1 cmd byte, 1 byte address
            }
            else
            {
              sendReply(stat);
              return;
            }
            break;
          case TXCMD_TYPE_WRITEAP:
            address = ptr[1];
            data32 = getUInt32(ptr+2);
            stat = g_interface->writeAP(address, data32);
            if (stat == RXCMD_STATUS_OK)
            {
              ptr+=6;    // 1 cmd byte, 1 byte address, 1 32-bit word
            }
            else
            {
              sendReply(stat);
              return;
            }
            break;
          case TXCMD_TYPE_READMEM:
            address = getUInt32(ptr+1);
            stat = g_interface->readMemory(address, data32);
            if (stat == RXCMD_STATUS_OK)
            {
              queueReplyUInt32(data32);
              ptr+=6;    // 1 cmd byte, 1 32-bit address
            }
            else
            {
              sendReply(stat);
              return;
            }
            break;
          case TXCMD_TYPE_WRITEMEM:
            address = getUInt32(ptr+1);
            data32 = getUInt32(ptr+5);
            stat = g_interface->writeMemory(address, data32);
            if (stat == RXCMD_STATUS_OK)
            {
              ptr+=9;    // 1 cmd byte, 1 32-bit address, 1 32-bit data
            }
            else
            {
              sendReply(stat);
              g_rxidx = 0;
              return;
            }
            break;
          case TXCMD_TYPE_GETPROGID:
            // get the programmer ID
            queueReplyUInt8(0x01);                  // protocol version
            queueReplyUInt8(sizeof(g_rxbuffer));    // rx buffer size
            queueReplyUInt8(0);                     // rx buffer size (MSB)
            ptr++;
            break;
        } // end switch      
      } // end while
      
      // if we end up here, everything worked out
      // and we can reply
      sendReply(RXCMD_STATUS_OK);     
       
    } // byte zero read
  } // while serial available
}

