/*

    Mid-level SWD interface driver
    Copyright Niels A. Moseley (c) 2016

*/

#ifndef mid_level_h
#define mid_level_h

#include <stdint.h>
#include "low_level.h"

#define SWDDELAY_us 20
#define LEDPIN 13
#define SWDDAT_PIN 2
#define SWDCLK_PIN 3


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

    /** Read a memory word */
    bool readMemory(uint32_t address, uint32_t &data);

    /** Write a memory word */
    bool writeMemory(uint32_t address, uint32_t data);

    /** write to an access port */
    bool writeAP(uint32_t address, uint32_t data);

    /** read form an access port */
    bool readAP(uint32_t address, uint32_t &data);

    /** write to a debug port */
    bool writeDP(uint32_t address, uint32_t data);

    /** read from a debug port */
    bool readDP(uint32_t address, uint32_t &data);

    /** connect */
    bool tryConnect(uint32_t &idcode);

  protected:
    // helper functions
    bool waitForMemory();
    bool selectAP(uint32_t address);

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
