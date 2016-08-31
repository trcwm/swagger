//
// Swagger target functions
//
// Author...: Niels A. Moseley
// Version..: 0.1
// 
// This is experimental!
//
// 
//
//
//
//
//
//
//

// *************************************
// global variables
// *************************************

const CMD_TYPE_CONNECT      = 0
const CMD_TYPE_RESET        = 1
const CMD_TYPE_READAP       = 2   // read access port register
const CMD_TYPE_WRITEAP      = 3   // write access port register
const CMD_TYPE_READDP       = 4   // read access port register
const CMD_TYPE_WRITEDP      = 5   // write access port register
const CMD_TYPE_READMEM      = 6   // read a memory address
const CMD_TYPE_WRITEMEM     = 7   // write to a memory address
const CMD_TYPE_WAITMEMTRUE  = 8   // wait for memory contents


function targetReset(state)
{  
    logmsg(LOG_DEBUG, "Reset " + state);
    clearCmdQueue();
    queueUInt8(CMD_TYPE_RESET);
    queueUInt8(state);
    executeCmdQueue();
}

function targetConnect()
{
    logmsg(LOG_DEBUG, "Connecting...\n");
    clearCmdQueue();
    queueUInt8(CMD_TYPE_CONNECT);
    executeCmdQueue();
    local result = popUInt8();
    local idcode = popUInt32();
    if (result == 0)
        logmsg(LOG_DEBUG, "Found IDCODE: " + format("%08X", idcode));
    else
        logmsg(LOG_DEBUG, "Connect failed!");
        
    return idcode;
}

logmsg(LOG_DEBUG, "Loaded targetfuncs.nut\n");
