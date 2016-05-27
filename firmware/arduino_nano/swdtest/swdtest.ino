/*
 *  SWD interface test for Arduino Due
 * 
 *  Niels A. Moseley
 * 
 */
 
#include <stdint.h>

#define LEDPIN 13
#define SWDDAT_PIN 2
#define SWDCLK_PIN 3

#define SWDDELAY_us 100

bool calcParity32(uint32_t x) 
{
   uint32_t y;
   y = x ^ (x >> 1);
   y = y ^ (y >> 2);
   y = y ^ (y >> 4);
   y = y ^ (y >> 8);
   y = y ^ (y >>16);
   return (y & 1) != 0;
}

// *********************************************************
//   Perform SWD transaction
// *********************************************************

void SWDInitPins()
{
  digitalWrite(SWDCLK_PIN, LOW); // SWD clock is normally low (?)
  digitalWrite(SWDDAT_PIN, LOW);  // SWD data is normally low
  pinMode(SWDDAT_PIN, OUTPUT);
  pinMode(SWDCLK_PIN, OUTPUT);
}


void SWDClock()
{
  delayMicroseconds(SWDDELAY_us);
  digitalWrite(SWDCLK_PIN, HIGH);
  delayMicroseconds(SWDDELAY_us);
  digitalWrite(SWDCLK_PIN, LOW);
}

void SWDWriteBit(bool bit)
{
  // clock must be low
  // dat must be driven
  
  digitalWrite(SWDDAT_PIN, bit ? HIGH : LOW);
  SWDClock();
}

bool SWDReadBit()
{
  // clock must be low
  // dat must be high-z

  bool b = (digitalRead(SWDDAT_PIN) == HIGH) ? true : false;
  SWDClock();
  return b;
}

// send bits over SWD
void SWDWord(uint16_t data)
{
  // SWDDAT must be set in OUTPUT mode    
  
  for(uint8_t i=0; i<16; i++)
  {
    SWDWriteBit(data >= 0x8000);
    data <<= 1;
  }
}

void SWDLineReset()
{
  pinMode(SWDDAT_PIN, OUTPUT);

  for(uint8_t i=0; i<64; i++)
  {
    SWDWriteBit(true);
  }
}


// write zero bits to keep the interface clocked
void SWDIdle()
{
  // dat pin must be in drive..
  pinMode(SWDDAT_PIN, OUTPUT);
  for(uint8_t i=0; i<8; i++)
  {
    SWDWriteBit(false);
  }
}



uint32_t SWDTransaction(bool APnDP, bool RnW, uint8_t A23, uint32_t data)
{
  uint32_t result = 0;
  // SWDCLK is assumed to be low at the start

  digitalWrite(SWDDAT_PIN, LOW);
  pinMode(SWDDAT_PIN, OUTPUT);

  //start with a zero bit..
  SWDWriteBit(false);

  bool parity = APnDP ^ RnW ^ ((A23 & 0x01) > 0) ^ ((A23 & 0x02) > 0);

  SWDWriteBit(true);    // start bit
  SWDWriteBit(APnDP);   //
  SWDWriteBit(RnW);     // 
  SWDWriteBit( (A23 & 0x01) > 0); // LSB first!
  SWDWriteBit( (A23 & 0x02) > 0); // then MSB  
  SWDWriteBit(parity);  
  SWDWriteBit(false);   // stop  
  SWDWriteBit(true);    // park bit

  pinMode(SWDDAT_PIN, INPUT);
  // turn around time
  SWDReadBit();         // trn

  // read acknowledge
  uint8_t ackbits = 0;
  
  if (SWDReadBit())
  {
    ackbits |= 0x04;
  }
  ackbits >>= 1;

  if (SWDReadBit())
  {
    ackbits |= 0x04;
  }
  ackbits >>= 1;

  if (SWDReadBit())
  {
    ackbits |= 0x04;
  }

  if (ackbits != 1)
  {
    // TODO: handle this exception!
    // FIXME: if we get a WAIT or FAULT response,
    // the packet ends here ..     
    Serial.println("Error in transaction!");
    return 0;
  }
  else 
  {
    Serial.println("OK");
  }

  if (!RnW)
  {    
    // write data!
    pinMode(SWDDAT_PIN, OUTPUT);

    // turn around time
    SWDReadBit();         // trn

    bool parity = calcParity32(data);
    for(uint8_t i=0; i<32; i++)
    { 
      if ((data & 1) != 0)
        SWDWriteBit(true);
      else
        SWDWriteBit(false);
        
      data >>= 1;
    }

    SWDWriteBit(parity);

    // turn-around    
    SWDReadBit();    
  }
  else
  {
    // read data
    result = 0;
    for(uint8_t i=0; i<32; i++)
    {
      result >>= 1;
      if (SWDReadBit())
        result |= 0x80000000;
    }

    bool parity = SWDReadBit();

    // turn-around    
    SWDReadBit();
    
    return result;
  }
}

void unlockSWD()
{  
  pinMode(SWDDAT_PIN, OUTPUT);
  delayMicroseconds(1000);

  for(uint32_t i=0; i<64; i++)
  {
    SWDWriteBit(true);
  }
  
  SWDWord(0x79E7);

  for(uint32_t i=0; i<64; i++)
  {
    SWDWriteBit(true);
  }
  
  SWDIdle();
  
  //READ IDCODE, hopefully..
  Serial.print("Part IDCODE: "); 
  Serial.println(SWDTransaction(false, true, 0, 0), HEX);
  
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

  SWDInitPins();
  

  // wait for serial comms to open
  while( !Serial) {}

  unlockSWD();

  SWDIdle();
  
  //SWDTransaction(false, false, 1, CSYSPWRUPREQ | CDBGRSTREQ);
  
  while(1) {}
  {
    SWDIdle();
    Serial.print("CTRL/STAT: ");
    Serial.println(SWDTransaction(false, true, 1, 0), HEX);
    delayMicroseconds(500000);
  };

}

void loop() 
{

}

