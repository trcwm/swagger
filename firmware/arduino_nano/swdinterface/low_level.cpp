/*

    Low-level SWD interface driver
    Copyright Niels A. Moseley (c) 2016

*/



#include "low_level.h"

SWDInterfaceBase::SWDInterfaceBase()
{
}

uint32_t SWDInterfaceBase::calcParity(uint32_t x)
{
   uint32_t y;
   y = x ^ (x >> 1);
   y = y ^ (y >> 2);
   y = y ^ (y >> 4);
   y = y ^ (y >> 8);
   y = y ^ (y >>16);
   return (y & 1) != 0;
}

void SWDInterfaceBase::initPins()
{
  setDataPin(false);    // '0'
  setClockPin(false);   // '0'
  configDataPin(true);  // set output
}

#if 0
void SWDInterfaceBase::clockStrobe()
{
  doDelay();
  setClockPin(true);
  doDelay();
  setClockPin(false);
}

void SWDInterfaceBase::writeBit(bool v)
{
  setDataPin(v);
  clockStrobe();
}

bool SWDInterfaceBase::readBit()
{
  bool b = getDataPin();
  clockStrobe();
  return b;
}

#else
// inline with CMSIS-DAP
void SWDInterfaceBase::clockStrobe()
{
  setClockPin(false);
  doDelay();
  setClockPin(true);
  doDelay();
}

void SWDInterfaceBase::writeBit(bool v)
{
  setDataPin(v);
  clockStrobe();
}

bool SWDInterfaceBase::readBit()
{  
  setClockPin(false);
  doDelay();
  bool b = getDataPin();
  setClockPin(true);
  doDelay();
  return b;
}

#endif


void SWDInterfaceBase::writeByte(uint8_t data)
{
  for(uint8_t i=0; i<8; i++)
  {
    writeBit((data & 1) != 0);
    data >>= 1;
  }
}

void SWDInterfaceBase::writeWord32(uint32_t data)
{
  for(uint8_t i=0; i<32; i++)
  {
    writeBit((data & 1) != 0);
    data >>= 1;
  }
}

uint8_t SWDInterfaceBase::readAck()
{
  uint8_t swdcode = 0;

  configDataPin(false); // input

  if (readBit())
    swdcode |= 1;

  if (readBit())
    swdcode |= 2;

  if (readBit())
    swdcode |= 4;

  return swdcode;
}

void SWDInterfaceBase::lineReset()
{
  configDataPin(true); // set as output
  for(int8_t i=0; i<64; i++)
    writeBit(true);
}

void SWDInterfaceBase::lineIdle()
{
  setDataPin(false);
  configDataPin(true); // set as output
  for(int8_t i=0; i<8; i++)
    writeBit(false);
}

uint8_t SWDInterfaceBase::doConnect(uint32_t &idcode)
{
  idcode = 0;
  configDataPin(true); // output
  lineReset();
  writeWord32(0xE79E);  // JTAG to SWD v5.x unlock code
  lineReset();
  lineIdle();
  setDataPin(true);
  return doReadTransaction(false, 0, idcode);
}

uint8_t SWDInterfaceBase::doReadTransaction(bool APnDP, uint8_t address, uint32_t &data)
{
  /*
   *
   *  Read transaction:
   *
   *  1) write zero bit (to allow for start bit detection.
   *  2) write start bit (1)
   *  3) Access Port (1) or Debug port (0) bit
   *  4) '1' for read operation
   *  5) even parity bit
   *  6) stop bit (0)
   *  7) park bit (1)
   *  8) read SWD response code
   *  9) if ok (001) then proceed
   *     otherwise perform turn-around, line idle and exit
   *  10) read 32 bit, LSB first
   *  11) turn-around
   *  12) line idle
   */
  configDataPin(true);  // output
  writeBit(false);      // start with a zero bit

  bool parity = 1 ^ APnDP ^ ((address & 0x01) > 0) ^ ((address & 0x02) > 0);

  writeBit(true);       // start bit
  writeBit(APnDP);
  writeBit(true);       // read operation
  writeBit( (address & 0x01) > 0);
  writeBit( (address& 0x02) > 0);
  writeBit(parity);
  writeBit(false);      // stop bit
  writeBit(true);       // park bit

  configDataPin(false); // input
  readBit();            // turn-around

  uint8_t swdcode = readAck();

  // check the acknowledge status
  if (swdcode == SWD_OK)
  {
    data = 0;
    for(uint8_t i=0; i<32; i++)
    {
      data >>= 1;
      if (readBit())
        data |= 0x80000000;
    }
  
    parity = readBit();
    if (calcParity(data) != parity)
    {
      //FIXME: what to do?
      // error, parity check fails!
      // for now, return a fail
      swdcode = 4;
    }
  
    // turn-around
    configDataPin(true);
    writeBit(false);
  
    // line idle so the SWD subsystem can process the transaction
    lineIdle();
    setDataPin(true);
      
    return swdcode; 
  }
  else if ((swdcode == SWD_FAIL) || (swdcode == SWD_WAIT))
  {
    // turn-around    
    readBit();
    configDataPin(true);
    //lineIdle();
    setDataPin(true);
    return swdcode;
  }

  // protocol error..
  // eat data + parity bit 
  for(uint8_t i=0; i<33; i++)
  {
    readBit();
  }
  
  //setDataPin(false);
  //configDataPin(true);
  lineIdle();
  setDataPin(true);
  
  return swdcode;
}

uint8_t SWDInterfaceBase::doWriteTransaction(bool APnDP, uint8_t address, uint32_t data)
{
  /*
   *
   *  Write transaction:
   *
   *  1) write zero bit (to allow for start bit detection.
   *  2) write start bit (1)
   *  3) Access Port (1) or Debug port (0) bit
   *  4) '0' for write operation
   *  5) even parity bit
   *  6) stop bit (0)
   *  7) park bit (1)
   *  8) read SWD response code
   *  9) if ok (001) then proceed
   *     otherwise perform turn-around, line idle and exit
   *  10) turn-around
   *  11) write 32 bit, LSB first
   *  12) write parity bit
   *  12b) line idle
   */
  configDataPin(true);  // output
  writeBit(false);      // start with a zero bit

  bool parity = 0 ^ APnDP ^ ((address & 0x01) > 0) ^ ((address & 0x02) > 0);

  writeBit(true);       // start bit
  writeBit(APnDP);
  writeBit(false);      // write operation
  writeBit( (address & 0x01) > 0);
  writeBit( (address & 0x02) > 0);
  writeBit(parity);
  writeBit(false);      // stop bit
  writeBit(true);       // park bit

  configDataPin(false); // input
  readBit();            // turn-around

  uint8_t swdcode = readAck();

  // check the acknowledge status
  if (swdcode == SWD_OK)
  {
    // turn around
    readBit(); 
    configDataPin(true);  // output
        
    writeWord32(data);
  
    writeBit(calcParity(data));
  
    // line idle so the SWD subsystem can process the transaction
    lineIdle();
    setDataPin(true);
    return swdcode;   
  }
  else if ((swdcode == SWD_FAIL) || (swdcode == SWD_WAIT))
  {
    // turn around    
    readBit();  
    configDataPin(true);  // output
    //lineIdle();
    setDataPin(true);
    return swdcode;
  }

  // protocol error..
  // eat data + parity bit 
  for(uint8_t i=0; i<33; i++)
  {
    readBit();
  }
  
  //setDataPin(false);
  //configDataPin(true);
  lineIdle();
  setDataPin(true);  

  return swdcode; // everything OK!
}
