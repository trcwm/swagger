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

// logging/debug function
function logmsg(level, str)
{
    if (::debug >= level)
        print(str)
}

// *********************************************************
//   Specific functions
// *********************************************************

// Read the status and control register
function readstat()
{
    local retval = readDP(DP_CTRLSTAT);
    if (retval.status != STAT_OK)
        logmsg(::LOG_ERROR, "readstat() reports a failure\n");
    
    return retval;    
}

// Write the status and control register
function writestat(data)
{    
    local retval = writeDP(DP_CTRLSTAT,data);
            
    if (retval.status != STAT_OK)
        logmsg(LOG_ERROR, "writestat() reports a failure\n");
    
    return retval;    
}

// *********************************************************
//   Very Specific functions
// *********************************************************


function enableDebugPower()
{
    // check if debug power is already enabled
    //local retval = readDP(DP_CTRLSTAT);
    //if ((retval.data & 0xF0000000) == 0xF0000000)
    //{
    //    return retval;
    //}
    
    local retval = writestat(0x50000000);  // enable power to DEBUG and system
    if (retval.status != STAT_OK)
        logmsg(LOG_ERROR, "enableDebugPower() reports a failure\n");
    
    // wait for confirmation
    retval = readDP(DP_CTRLSTAT);
    if (retval.status != STAT_OK)
        logmsg(LOG_ERROR, "enableDebugPower() reports a failure\n");    
    
    while((retval.data & 0xF0000000) != 0xF0000000)
    {
        // rely on the programmer to cause a delay due to protocol overhead
        retval = readDP(DP_CTRLSTAT);
        if (retval.status == STAT_FAULT)
            return retval;
    }
    
    //if (retval.status != STAT_OK)
        //logmsg(LOG_ERROR, "enableDebugPower() reports a failure\n");
        
    if (retval.status == STAT_OK)
        logmsg(LOG_DEBUG, "Debug system powered up!\n");
        
    return retval;
}



function clearStickyErrors()
{
    logmsg(LOG_DEBUG,"Clearing sticky errors\n");
    return writeDP(DP_ABORT, 1+2+4+8+16);
}

// initialize the processor
function init()
{
    dofile("..//targets//targets.nut");
    connect();
    clearStickyErrors();
    enableDebugPower();
}
