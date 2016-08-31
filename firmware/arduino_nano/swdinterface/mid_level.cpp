/*

    Mid-level SWD interface driver
    Copyright Niels A. Moseley (c) 2016

*/
 
#include <Arduino.h>
#include <stdint.h>
#include "mid_level.h"
#include "protocol.h"

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

uint8_t ArduinoSWDInterface::tryConnect(uint32_t &idcode)
{
    m_APcache = 0xFFFFFFFF; // invalidate Access port cache
    if (doConnect(idcode) == SWD_OK)
        return RXCMD_STATUS_OK;
        
    return RXCMD_STATUS_SWDFAULT;
}

uint8_t ArduinoSWDInterface::writeDP(uint32_t address, uint32_t data)
{
    uint32_t A23 = (address >> 2) & 0x03;

    uint32_t retries = 0;
    uint8_t retval;
    while((retval=doWriteTransaction(false, A23, data)) == SWD_WAIT)
    {
        retries++;
        if (retries == MAX_RETRIES)
            return RXCMD_STATUS_TIMEOUT;
    }
    
    switch(retval)
    {
      case SWD_OK:
        return RXCMD_STATUS_OK;
      default:
        return RXCMD_STATUS_SWDFAULT;         
    }
}

uint8_t ArduinoSWDInterface::readDP(uint32_t address, uint32_t &data)
{
    uint32_t A23 = (address >> 2) & 0x03;

    uint32_t retries = 0;
    uint8_t retval;
    while((retval=doReadTransaction(false, A23, data)) == SWD_WAIT)
    {
        retries++;
        if (retries == MAX_RETRIES)
            return RXCMD_STATUS_TIMEOUT;
    }
    switch(retval)
    {
      case SWD_OK:
        return RXCMD_STATUS_OK;
      default:
        return RXCMD_STATUS_SWDFAULT;         
    }
}

uint8_t ArduinoSWDInterface::selectAP(uint32_t address)
{
    uint32_t ap = address & 0xFF0000F0;
    if (ap == m_APcache)
        return RXCMD_STATUS_OK;

    uint8_t retval = writeDP(DP_SELECT, ap);
    
    //lineIdle();
    if (retval == RXCMD_STATUS_OK)
    {
      m_APcache = ap;
    }
    
    return retval;
}

uint8_t ArduinoSWDInterface::writeAP(uint32_t address, uint32_t data)
{
    uint8_t retval = selectAP(address);
    if (retval != RXCMD_STATUS_OK)
        return retval;
        
    uint32_t A23 = (address >> 2) & 0x03;
    uint32_t retries = 0;
    while((retval=doWriteTransaction(true, A23, data)) == SWD_WAIT)
    {
        //lineIdle();
        retries++;
        if (retries == MAX_RETRIES)
            return RXCMD_STATUS_TIMEOUT;
    }
    switch(retval)
    {
      case SWD_OK:
        return RXCMD_STATUS_OK;
      default:
        return RXCMD_STATUS_SWDFAULT;         
    }
}

uint8_t ArduinoSWDInterface::readAP(uint32_t address, uint32_t &data)
{
    uint8_t retval = selectAP(address);
    if (retval != RXCMD_STATUS_OK)
        return retval;
        
    uint32_t A23 = (address >> 2) & 0x03;
    uint32_t retries = 0;
    while((retval=doReadTransaction(true, A23, data)) == SWD_WAIT)
    {
        retries++;
        if (retries == MAX_RETRIES)
            return RXCMD_STATUS_TIMEOUT;
    }
    if (retval != SWD_OK)
      return RXCMD_STATUS_SWDFAULT;
    
    return readDP(DP_RDBUFF, data);
}


uint8_t ArduinoSWDInterface::waitForMemory()
{
    uint32_t data;
    uint32_t retval;
    uint32_t retries = 0;
    while((retval=readAP(AHB_AP_CSW, data))==RXCMD_STATUS_OK)
    {
        if ((data & 0x80) == 0x00)
            return RXCMD_STATUS_OK;
        
        if (retries == MAX_RETRIES)
            return RXCMD_STATUS_TIMEOUT;
            
        retries++;
    }
    return retval;
}

uint8_t ArduinoSWDInterface::readMemory(uint32_t address, uint32_t &data)
{
  uint8_t retval;

  if ((retval=writeAP(AHB_AP_CSW, 0x22000012)) != RXCMD_STATUS_OK)
    return retval;
  
  if ((retval=waitForMemory()) != RXCMD_STATUS_OK)
    return retval;

  if ((retval=writeAP(AHB_AP_TAR, address)) != RXCMD_STATUS_OK)
    return retval;

  return readAP(AHB_AP_DATA, data);
}

uint8_t ArduinoSWDInterface::writeMemory(uint32_t address, uint32_t data)
{
  uint8_t retval;

  if ((retval=writeAP(AHB_AP_CSW, 0x22000012)) != RXCMD_STATUS_OK)
    return retval;

  if ((retval=waitForMemory()) != RXCMD_STATUS_OK)
    return retval;

  if ((retval=writeAP(AHB_AP_TAR, address)) != RXCMD_STATUS_OK)
    return retval;

  return writeAP(AHB_AP_DATA, data);
}

uint8_t ArduinoSWDInterface::waitMemoryTrue(uint32_t address, uint32_t data, uint32_t mask)
{
  uint8_t retval;
  uint32_t retries = 0;
  uint32_t memData;
  do
  {
    if ((retval=readMemory(address, memData)) != RXCMD_STATUS_OK)
      return retval;
      
    retries++;
    if (retries == MAX_RETRIES)
      return RXCMD_STATUS_TIMEOUT;
    
  } while ((memData & mask) != data);
  
  return RXCMD_STATUS_OK;
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

void ArduinoSWDInterface::setReset(bool v)
{
  // note: active low
  if (v)
    digitalWrite(RESET_PIN, LOW);
  else
    digitalWrite(RESET_PIN, HIGH);    
}

void ArduinoSWDInterface::doDelay()
{
  delayMicroseconds(SWDDELAY_us);
}

