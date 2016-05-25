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
//
#include <protocol.h>

#define LEDPIN 13
#define SWDDAT_PIN 3
#define SWDCLK_PIN 4

uint8_t g_rxbuffer[32]; // packet receive buffer
uint8_t g_idx = 0;      // write index into receive buffer

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
  Serial.write((char*)encodebuffer, strlen((char*)encodebuffer)+1);
  Serial.flush(); // wait for TX to be done. (do we need this?)
}

// *********************************************************
//   Perform SWD transaction
// *********************************************************

void SWDInitPins()
{
  pinMode(SWDDAT_PIN, INPUT);     // High-Z state
  pinMode(SWDCLK_PIN, OUTPUT);
  digitalWrite(SWDCLK_PIN, LOW);  // SWD clock is normally low
}

// Data must be output to the target after the falling edge of SWDCLK
// Data will be set by the target on the rising edge of SWDCLK

#define SWDDELAY_us 1000

// send bits over SWD
void SWDSendBits(uint8_t b, uint8_t bits)
{
  // SWDDAT must be set in OUTPUT mode
  data <<= (bits-8);
    
  for(uint8_t i=0; i<bits; i++)
  {
    digitalWrite(SWDCLK_PIN, HIGH);
    if (data<127)
      digitalWrite(SWDDAT_PIN, HIGH);
    else
      digitalWrite(SWDDAT_PIN, LOW);
      
    delayMicroseconds(SWDDELAY_us);
    
    digitalWrite(SWDCLK_PIN, LOW);
    delayMicroseconds(SWDDELAY_us);
  }
}

void SWDLineReset()
{
  pinMode(SWDDAT_PIN, OUTPUT);
  
  for(uint8_t i=0; i<50; i++)
  {
    digitalWrite(SWDCLK_PIN, HIGH);
    digitalWrite(SWDDAT_PIN, HIGH);
    delayMicroseconds(SWDDELAY_us);
    digitalWrite(SWDCLK_PIN, LOW);
    delayMicroseconds(SWDDELAY_us);    
  }
}

void SWDTransaction()
{
  // SWDCLK is assumed to be low at the start

  pinMode(SWDDAT_PIN, OUTPUT);
  delayMicroseconds(SWDDELAY_us);
  
}

void unlockSWD()
{
  pinMode(SWDDAT_PIN, OUTPUT);
  delayMicroseconds(1000);
  
  SWDSendBits(0x79, 8);
  SWDSendBits(0xE7, 8);

  SWDLineReset();
}

// *********************************************************
//   Main program
// *********************************************************

void setup() {
  // put your setup code here, to run once:
  Serial.begin(19200);
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
            reply(RXCMD_STATUS_FAIL, SWDCODE_FAIL, 0x01010101);
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

