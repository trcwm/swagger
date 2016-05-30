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

#include <protocol.h>

#define SWDDELAY_us 1000
#define LEDPIN 13
#define SWDDAT_PIN 2
#define SWDCLK_PIN 3

uint8_t g_rxbuffer[32]; // packet receive buffer
uint8_t g_idx = 0;      // write index into receive buffer



/*************************************************************
 * 
 *  Base class for SWD interface
 * 
 */

class SWDInterfaceBase
{
  public:
    SWDInterfaceBase();
    virtual ~SWDInterfaceBase() {}
    
    /** Initialize/configure SWD pins.
     *  Note: it is assumed that the clock pin has already been
     *  configured as an output.
     */
    
    void initPins();

    /** Connect to the target
     *  idcode will contain the IDCODE if operation was succesfull.
     *  Returns the SWD ack code
     */
    uint8_t doConnect(uint32_t &idcode);

    /** Perform a write transaction
     *  APnDP - true if transaction is an Access Port transaction, else Debug Port.
     *  A23   - represent the A[2:3] bits: A[2] must be stored in the LSB!
     *  data  - the 32-bit data being written.
     *  returns the SWD code
     */
    uint8_t doWriteTransaction(bool APnDP, uint8_t A23, uint32_t data);

    /** Perform a read transaction
     *  APnDP - true if transaction is an Access Port transaction, else Debug Port.
     *  A23   - represent the A[2:3] bits: A[2] must be stored in the LSB!
     *  data  - will contain the data, if operation was succesfull
     */
    uint8_t doReadTransaction(bool APnDP, uint8_t A23, uint32_t &data);

  protected:

    /** Set the state of the output pin.
     *  if output == true then configure as output
     *  if output == false then configure as High-Z input
    */  
    virtual void configDataPin(bool output) = 0;

    /** Set the state of the data pin */
    virtual void setDataPin(bool v) = 0;

    /** Get the state of the data pin */
    virtual bool getDataPin() = 0;

    /** Set the state of the clock pin */
    virtual void setClockPin(bool v) = 0;

    /** wait for 1/2 a bit time */
    virtual void doDelay() = 0;

    /** write a single bit to the data pin */
    void writeBit(bool v);

    /** read a signle bit from the data pin */
    bool readBit();

    /** write a byte to the data pin, LSB first */
    void writeByte(uint8_t data);

    /** write a 32-bit word to the data pin, LSB first */
    void writeWord32(uint32_t data);

    /** reads the three ACK bits, LSB first */
    uint8_t readAck();

    /** perform a 64- cycle line reset */
    void lineReset();

    /** perform an 8-cycle line idle */
    void lineIdle();

    /** strobe the clock '1' and then '0' */
    void clockStrobe();

    /** calculate the odd parity for a 32-bit data word */
    uint32_t calcParity(uint32_t data);
};

SWDInterfaceBase::SWDInterfaceBase()
{
  //initPins();
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

  if (swdcode != 1)     // SWD OK!
  {
    // turn-around
    configDataPin(true);
    writeBit(false);     
    return swdcode;
  }

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
  
  return swdcode; // everything OK!
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

  // turn around
  configDataPin(true);  // output
  readBit();

  if (swdcode != 1)     // SWD OK!
  {
   
    lineIdle();
    return swdcode; // error!
  }

  writeWord32(data);

  writeBit(calcParity(data));

  // line idle so the SWD subsystem can process the transaction
  lineIdle();
  
  return swdcode; // everything OK!
}

/*************************************************************
 * 
 *  Arduino programmer class
 * 
 */

class ArduinoSWDInterface : public SWDInterfaceBase
{
  public:
    ArduinoSWDInterface() : SWDInterfaceBase() {}

  protected:
    /** Set the state of the output pin.
     *  if output == true then configure as output
     *  if output == false then configure as High-Z input
    */  
    virtual void configDataPin(bool output);

    /** Set the state of the data pin */
    virtual void setDataPin(bool v);

    /** Get the state of the data pin */
    virtual bool getDataPin();

    /** Set the state of the clock pin */
    virtual void setClockPin(bool v);

    /** wait for 1/2 a bit time */
    virtual void doDelay();
};


void ArduinoSWDInterface::configDataPin(bool output)
{
  if (output)
    pinMode(SWDDAT_PIN, OUTPUT);
  else
    pinMode(SWDDAT_PIN, INPUT);
}

void ArduinoSWDInterface::setDataPin(bool v)
{
  if (v)
    digitalWrite(SWDDAT_PIN, HIGH);
  else
    digitalWrite(SWDDAT_PIN, LOW);
}

bool ArduinoSWDInterface::getDataPin()
{
  return (digitalRead(SWDDAT_PIN) == HIGH);
}

void ArduinoSWDInterface::setClockPin(bool v)
{
  if (v)
    digitalWrite(SWDCLK_PIN, HIGH);
  else
    digitalWrite(SWDCLK_PIN, LOW);  
}

void ArduinoSWDInterface::doDelay()
{
  delayMicroseconds(SWDDELAY_us);
}


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
        uint8_t swdcode;
        switch(cmd->cmdType)
        {
          default:
          case TXCMD_TYPE_UNKNOWN:
            reply(RXCMD_STATUS_OK, SWDCODE_FAIL, cmd->cmdType);
            break;
          case TXCMD_TYPE_RESETPIN:
            // as of yet unsupported
            reply(RXCMD_STATUS_OK, SWDCODE_ACK, 0);
            break;
          case TXCMD_TYPE_CONNECT:
            swdcode = g_interface->doConnect(data);
            reply(RXCMD_STATUS_OK, SWDCODE_ACK, data);  // return IDCODE
            break;
          case TXCMD_TYPE_READREG:
            swdcode = g_interface->doReadTransaction((cmd->APnDP) > 0, cmd->address, data);
            reply(RXCMD_STATUS_OK, swdcode, data);
            break;
          case TXCMD_TYPE_WRITEREG:
            swdcode = g_interface->doWriteTransaction((cmd->APnDP) > 0, cmd->address, cmd->data);
            reply(RXCMD_STATUS_OK, swdcode, cmd->data);
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

