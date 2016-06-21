/*

    Low-level SWD interface driver
    Copyright Niels A. Moseley (c) 2016

*/

#ifndef low_level_h
#define low_level_h

#include <stdint.h>

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
    virtual uint8_t doConnect(uint32_t &idcode);

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

#endif
