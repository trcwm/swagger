//
// Swagger utility functions
//
// Author...: Niels A. Moseley
// Version..: 0.1
// 
// This is experimental!
//

// global constants
const MAX_RETRIES = 100;

const LOG_ERROR     = 0;
const LOG_WARNING   = 1;
const LOG_DEBUG     = 2;

const DP_IDCODE     = 0x00; // read only
const DP_ABORT      = 0x00; // write only
const DP_CTRLSTAT   = 0x04; // control/status register
const DP_SELECT     = 0x08; // register select
const DP_RDBUFF     = 0x0C; // read buffer

const AHB_AP_CSW    = 0x00000000;
const AHB_AP_TAR    = 0x00000004;
const AHB_AP_DATA   = 0x0000000C;
const AHB_AP_ROMTBL = 0x000000F8;
const AHB_AP_IDR    = 0x000000FC;

// global variables
g_AP_SELECT_CACHE <- 0

// logging/debug function
function logmsg(level, str)
{
    if (::debug >= level)
        print(str)
}

// write Debug port
function writeDP(address, data)
{
    local AD23 = (address >> 2) & 0x03;
    return dwrite(0, AD23, data);
}

// read Debug port
// returns a table:
//   .swdcode - the SWD code
//   .data    - the 32-bit data
function readDP(address)
{
    local AD23 = (address >> 2) & 0x03;
    return dread(0, AD23);
}

// *********************************************************
//   Specific functions
// *********************************************************

// select the access port
function selectAP(reg_address)
{
    // check if the AP has been selected
    local ap = reg_address & 0xFF0000F0;   
    if (ap == g_AP_SELECT_CACHE)
        return;
    
    logmsg(LOG_DEBUG, format("Selecting AP=%08X\n", ap));
    
    // write the Access Port select register    
    local retval = writeDP(DP_SELECT, ap);
        
    // retry if we get a WAIT response
    local retries = 0;
    while(retval.swdcode == SWD_WAIT)
    {
        // retry if there is a wait code
        retries++;
        if (retries > MAX_RETRIES)
        {
            return retval;
        }
        retval = writeDP(DP_SELECT, ap);
    }

    if (retval.swdcode == SWD_FAULT)
        logmsg(::LOG_ERROR, "APselect() reports a failure\n");
    
    g_AP_SELECT_CACHE = ap;
    
    return retval;
}

// write Access port
function writeAP(address, data)
{
    // 1) select the correct access port,
    //    depending on the register address
    // 3) write to the AP.

    selectAP(address);
    local AD23 = (address >> 2) & 0x03;
    return dwrite(1, AD23, data);
}


// read Access port
// returns a table:
//   .swdcode - the SWD code
//   .data    - the 32-bit data
function readAP(address)
{
    // 1) select the correct access port,
    //    depending on the register address
    // 2) read the AP.
    
    selectAP(address);
    local AD23 = (address >> 2) & 0x03;
    local retval = dread(1, AD23);  // read AP register
    if (retval.swdcode != SWD_OK)
        return retval;
    
    return readDP(DP_RDBUFF);
}

// Read the status and control register
function readstat()
{
    local retval = readDP(DP_CTRLSTAT);
    
    // retry if we get a WAIT response
    local retries = 0;
    while(retval.swdcode == SWD_WAIT)
    {
        // retry if there is a wait code
        retries++;
        if (retries > MAX_RETRIES)
        {
            return retval;
        }
        retval = readDP(DP_CTRLSTAT);
    }
    
    if (retval.swdcode == SWD_FAULT)
        logmsg(::LOG_ERROR, "readstat() reports a failure\n");
    
    return retval;    
}

// Write the status and control register
function writestat(data)
{    
    local retval = writeDP(DP_CTRLSTAT,data);
    
    // retry if we get a WAIT response
    local retries = 0;
    while(retval.swdcode == SWD_WAIT)
    {
        // retry if there is a wait code
        retries++;
        if (retries > MAX_RETRIES)
        {
            return retval;
        }
        retval = writeDP(DP_CTRLSTAT,data);
    }

    if (retval.swdcode == SWD_FAULT)
        logmsg(LOG_ERROR, "writestat() reports a failure\n");
    
    return retval;    
}

// *********************************************************
//   Very Specific functions
// *********************************************************


function enableDebug()
{
    local retval = writestat(0x50000000);  // enable power to DEBUG and system
    
    // wait for confirmation
    retval = readDP(DP_CTRLSTAT);
    
    while((retval.data & 0xF0000000) != 0xF0000000)
    {
        // rely on the programmer to cause a delay due to protocol overhead
        retval = readDP(DP_CTRLSTAT);        
    }
    
    switch(retval.swdcode)
    {
    case SWD_FAULT:
        logmsg(LOG_ERROR, "enableDebug() returned FAIL\n");
        break;
    case SWD_WAIT:
        logmsg(LOG_DEBUG, "time-out waiting for debug confirmation\n");
    case SWD_OK:
        logmsg(LOG_DEBUG, "debugging enabled");
        break;
    default:
        logmsg(LOG_ERROR, "unrecognized SWD return code\n");
        break;
    }
    return retval;
}



function clearStickyErrors()
{
    logmsg(LOG_DEBUG,"Clearing sticky errors\n");
    return writeDP(DP_ABORT, 1+2+4+8+16);
}

function waitForMemory()
{
    local retval;
    retval = readAP(AHB_AP_CSW);
    local retries = MAX_RETRIES;
    while((retval.data & 0x80) == 0x80)
    {
        retval = readAP(AHB_AP_CSW);
        if (retval.swdcode == SWD_FAULT)
        {
            logmsg(LOG_ERROR, "waitForMemory reports a fault!\n");
            return retval;
        }
    }
    return retval;
}

function readMemory(address)
{
    local retval;
    retval = writeAP(AHB_AP_CSW, 0x22000012);   // set configuration register
    if (retval.swdcode == SWD_FAULT)
    {
        logmsg(LOG_ERROR, "readMemory reported an SWD FAULT\n");
        return retval;
    }
    
    waitForMemory();
    
    retval = writeAP(AHB_AP_TAR, address);      // set read memory address
    if (retval.swdcode == SWD_FAULT)
    {
        logmsg(LOG_ERROR, "readMemory reported an SWD FAULT\n");
        return retval;
    }
    
    retval = readAP(AHB_AP_DATA);               // read register\
    if (retval.swdcode == SWD_FAULT)
    {
        logmsg(LOG_ERROR, "readMemory reported an SWD FAULT\n");
    }    
    return retval;
}

function writeMemory(address, data)
{
    local retval;
    retval = writeAP(AHB_AP_CSW, 0x22000012);   // set configuration register
    if (retval.swdcode == SWD_FAULT)
    {
        logmsg(LOG_ERROR, "writeMemory reported an SWD FAULT\n");
        return retval;
    }
    
    waitForMemory();
    
    retval = writeAP(AHB_AP_TAR, address);      // set the write address
    if (retval.swdcode == SWD_FAULT)
    {
        logmsg(LOG_ERROR, "writeMemory reported an SWD FAULT\n");
        return retval;
    }
    
    retval = writeAP(AHB_AP_DATA, data);        // perform write
    if (retval.swdcode == SWD_FAULT)
    {
        logmsg(LOG_ERROR, "writeMemory reported an SWD FAULT\n");
    }   
    return retval;
}

// initialize the processor
function init()
{
    dofile("..//targets//targets.nut");
    connect();
    clearStickyErrors();
    writeDP(DP_SELECT,0);
    ::g_AP_SELECT_CACHE = 0;
    enableDebug();
}
