/*
 *  SWD interface code for Arduino Nano
 * 
 *  Niels A. Moseley
 * 
 */
 
#include <stdint.h>

uint8_t rxbuffer[32];
uint8_t idx = 0;

const uint8_t OKPacket[] = {
  0x01,
  0x03,
  0x01,
  0xFF,
  0x01,
  0x01,
  0x01,
  0x00};


//FIXME: get this info from the main program.
struct HardwareTXCommand
{
    uint8_t     cmdType;    // 0 = reset condition, 1 = read register, 2 = write register
    uint8_t     address;   // bits [2:3] of register address
    uint8_t     APnDP;      // Address or data register access (1 = address)
    uint32_t    data;       // reset line state or write register data
};

/** format of packets received from the hardware interface
    Note that this data will be encapsulated in a
    COBS packet. */
struct HardwareRXCommand
{
    uint8_t     status;     // status of communication 0 = OK
    uint8_t     swdcode;    // swd return code
    uint32_t    data;       // read register data
};


// COBS decoder (source: wikipedia)
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

// COBS encoder: (source: wikipedia)

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
      if (idx < sizeof(rxbuffer))
      {
        uint8_t decodebuffer[sizeof(rxbuffer)]; // decode bytes is always less than encoded bytes..
        UnStuffData(rxbuffer, idx, decodebuffer);
        HardwareTXCommand *cmd = (HardwareTXCommand *)&decodebuffer;
        if (cmd->cmdType == 0)
        {
          if (cmd->data == 1)
          {
            digitalWrite(13, HIGH);
          }
          else
          {
            digitalWrite(13, LOW);
          }
        }
      }
      idx = 0;
      
      // Send COBS encoded packet
      HardwareRXCommand cmd;
      cmd.status  = 0;  // OK
      cmd.swdcode = 1;  // ACK
      cmd.data    = 0x01010101;  // 1 for better COBS encoding
      uint8_t encodebuffer[sizeof(HardwareRXCommand)+2];
      StuffData((uint8_t*)&cmd, sizeof(cmd), encodebuffer);
      Serial.write((char*)encodebuffer);
    }
    else
    {
      if (idx < sizeof(rxbuffer))
      {
        rxbuffer[idx++] = byteRead;
      }
      else
      {
        // avoid buffer overrun :-(
        // TODO: send error packet
        idx = 0; 
      }
    }
  }
}

