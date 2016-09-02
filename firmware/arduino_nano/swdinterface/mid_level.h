/*

    Mid-level SWD interface driver
    Copyright Niels A. Moseley (c) 2016

*/

#ifndef mid_level_h
#define mid_level_h

#include <stdint.h>
#include "low_level.h"

#define SWDDELAY_us 1
#define LEDPIN 13
#define SWDCLK_PIN 2
#define SWDDAT_PIN 3
#define RESET_PIN 4
#define GND_PIN 5


/*************************************************************

    Mid-level SWD driver - functions that handle SWD
                           wait gracefully.
*/

class ArduinoSWDInterface : private SWDInterfaceBase
{
  public:
    ArduinoSWDInterface() : SWDInterfaceBase()
    {
        initPins();
        m_APcache = 0xFFFFFFFF; // invalidate cache
    }
    
    /** Set the state of the target reset 
     *  v = true puts target into reset
    */
    virtual void setReset(bool v);
    
    /** Read a memory word */
    uint8_t readMemory(uint32_t address, uint32_t &data);

    /** Write a memory word */
    uint8_t writeMemory(uint32_t address, uint32_t data);

    /** write to an access port */
    uint8_t writeAP(uint32_t address, uint32_t data);

    /** read form an access port */
    uint8_t readAP(uint32_t address, uint32_t &data);

    /** write to a debug port */
    uint8_t writeDP(uint32_t address, uint32_t data);

    /** read from a debug port */
    uint8_t readDP(uint32_t address, uint32_t &data);

    /** wait until data at memory address equals (data & mask) */
    uint8_t waitMemoryTrue(uint32_t address, uint32_t data, uint32_t mask);

    /** connect */
    uint8_t tryConnect(uint32_t &idcode);

  protected:
    // helper functions

    /** wait until the memory controller becomes available */
    uint8_t waitForMemory();

    /** set the current the access port */
    uint8_t selectAP(uint32_t address);

    //
    // pin related functions
    //
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

    // current select Access port
    uint32_t m_APcache;
};

#endif
