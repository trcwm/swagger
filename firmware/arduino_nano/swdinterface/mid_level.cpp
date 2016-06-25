/*

    Mid-level SWD interface driver
    Copyright Niels A. Moseley (c) 2016

*/
 
#include <Arduino.h>
#include <stdint.h>
#include "mid_level.h"

//const uint8_t SWD_OK=1;
//const uint8_t SWD_WAIT=2;
//const uint8_t SWD_FAIL=4;

const uint32_t MAX_RETRIES  = 10;

const uint8_t DP_IDCODE     = 0x00; // read only
const uint8_t DP_ABORT      = 0x00; // write only
const uint8_t DP_CTRLSTAT   = 0x04; // control/status register
const uint8_t DP_SELECT     = 0x08; // register select
const uint8_t DP_RDBUFF     = 0x0C; // read buffer

const uint32_t AHB_AP_CSW    = 0x00000000;
const uint32_t AHB_AP_TAR    = 0x00000004;
const uint32_t AHB_AP_DATA   = 0x0000000C;
const uint32_t AHB_AP_ROMTBL = 0x000000F8;
const uint32_t AHB_AP_IDR    = 0x000000FC;

bool ArduinoSWDInterface::tryConnect(uint32_t &idcode)
{
    m_APcache = 0xFFFFFFFF; // invalidate Access port cache
    if (doConnect(idcode) == SWD_OK)
        return true;
        
    return false;
}

bool ArduinoSWDInterface::writeDP(uint32_t address, uint32_t data)
{
    uint32_t A23 = (address >> 2) & 0x03;

    uint32_t retries = 0;
    uint8_t retval;
    while((retval=doWriteTransaction(false, A23, data)) == SWD_WAIT)
    {
        retries++;
        if (retries == MAX_RETRIES)
            return false;
    }
    return (retval == SWD_OK);
}

bool ArduinoSWDInterface::readDP(uint32_t address, uint32_t &data)
{
    uint32_t A23 = (address >> 2) & 0x03;

    uint32_t retries = 0;
    uint8_t retval;
    while((retval=doReadTransaction(false, A23, data)) == SWD_WAIT)
    {
        retries++;
        if (retries == MAX_RETRIES)
            return false;
    }
    return (retval == SWD_OK);
}

bool ArduinoSWDInterface::selectAP(uint32_t address)
{
    uint32_t ap = address & 0xFF0000F0;
    if (ap == m_APcache)
        return true;

    uint32_t retries = 0;
    while(!writeDP(DP_SELECT, ap))
    {        
        //lineIdle();
        retries++;
        if (retries == MAX_RETRIES)
            return false;
    }
    
    //lineIdle();
    m_APcache = ap;
    return true;
}

bool ArduinoSWDInterface::writeAP(uint32_t address, uint32_t data)
{
    if (!selectAP(address))
        return false;
        
    uint32_t A23 = (address >> 2) & 0x03;
    uint32_t retries = 0;
    uint8_t retval;
    while((retval=doWriteTransaction(true, A23, data)) == SWD_WAIT)
    {
        //lineIdle();
        retries++;
        if (retries == MAX_RETRIES)
            return false;
    }
    return (retval == SWD_OK);
}

bool ArduinoSWDInterface::readAP(uint32_t address, uint32_t &data)
{
    if (!selectAP(address))
        return false;
    uint32_t A23 = (address >> 2) & 0x03;
    uint32_t retries = 0;
    uint8_t retval;
    while((retval=doReadTransaction(true, A23, data)) == SWD_WAIT)
    {
        retries++;
        if (retries == MAX_RETRIES)
            return false;
    }
    if (retval != SWD_OK)
        return false;

    return readDP(DP_RDBUFF, data);
}


bool ArduinoSWDInterface::waitForMemory()
{
    uint32_t data;

    uint32_t retries = 0;
    while(readAP(AHB_AP_CSW, data))
    {
        if ((data & 0x80) == 0x00)
            return true;
        
        if (retries == MAX_RETRIES)
            return false;
            
        retries++;
    }
    return false;
}

bool ArduinoSWDInterface::readMemory(uint32_t address, uint32_t &data)
{
  if (!writeAP(AHB_AP_CSW, 0x22000012))
    return false;

  if (!waitForMemory())
    return false;

  if (!writeAP(AHB_AP_TAR, address))
    return false;

  if (!readAP(AHB_AP_DATA, data))
    return false;

  return true;
}

bool ArduinoSWDInterface::writeMemory(uint32_t address, uint32_t data)
{
  if (!writeAP(AHB_AP_CSW, 0x22000012))
    return false;

  if (!waitForMemory())
    return false;

  if (!writeAP(AHB_AP_TAR, address))
    return false;

  if (!writeAP(AHB_AP_DATA, data))
    return false;

  return true;
}


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

