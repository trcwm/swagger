//
// Swagger target functions
//
// Author...: Niels A. Moseley
// Version..: 0.1
// 
// This is experimental!
//

// *************************************
// global constants
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

const CMD_STATUS_OK         = 0   // command OK
const CMD_STATUS_TIMEOUT    = 1   // command time out
const CMD_STATUS_SWDFAULT   = 2   // SWD fault
const CMD_STATUS_RXOVERFLOW = 3   // RX overflow
const CMD_STATUS_PROTOERR   = 4   // Protocol error
const CMD_STATUS_UNKNOWNCMD = 5   // Unknown command

const SCS_SHCSR             = 0xE000ED24;   // System handler control and state 
const SCS_DFSR              = 0xE000ED30;   // Debug fault status register
const SCS_DHCSR             = 0xE000EDF0;   // Debug halting control and status
const SCS_DCRSR             = 0xE000EDF4;   // Debug core register selector
const SCS_DCRDR             = 0xE000EDF8;   // Debug core data register
const SCS_DEMCR             = 0xE000EDFC;   // Debug exception monitor ctrl reg

const DHCSR_DBGKEY          = 0xA05F0000;   // upper 16 bits of key
const C_DEBUGEN             = 0x00000001;   // DHCSR debug enable bit
const C_HALT                = 0x00000002;   // DHCSR halt core
const C_STEP                = 0x00000004;   // DHCSR single step mode
const C_MASKINTS            = 0x00000008;   // DHCSR mask external ints
const S_REGRDY              = 0x00010000;   // DHCSR register ready bit
const S_HALT                = 0x00020000;   // DHCSR halt status bit
const S_RETIRE_ST           = 0x01000000;   // DHCSR instruction retired bit
const S_RESET_ST            = 0x02000000;   // DHCSR core reset status (sticky)

const DCRSR_WRITE       = 0x00010000;   // core register write enable

const DCRSR_REG_SP      = 0x0D;
const DCRSR_REG_LR      = 0x0E;
const DCRSR_REG_DBGRET  = 0x0F;
const DCRSR_REG_PSRFLAG = 0x10;
const DCRSR_REG_MSP     = 0x11;
const DCRSR_REG_PSP     = 0x12;
const DCRSR_REG_IPSR    = 0x17;
const DCRSR_REG_CONTROL = 0x1B;

// *************************************
// Functions
// *************************************

// set target reset
// if state > 0, the target is put into reset
// else reset is released
function setReset(state)
{  
    logmsg(LOG_DEBUG, "Reset " + state);
    clearCmdQueue();
    queueUInt8(CMD_TYPE_RESET);
    queueUInt8(state);
    executeCmdQueue();
}

// connect to the target
// it returns the IDCODE, if connect is succesful
// or 0xFFFFFFFF if it failed
function connect()
{
    logmsg(LOG_DEBUG, "Connecting...\n");
    clearCmdQueue();
    queueUInt8(CMD_TYPE_CONNECT);
    executeCmdQueue();
    local result = popUInt8();
    local idcode = popUInt32();
    if (result == CMD_STATUS_OK)
    {
        logmsg(LOG_DEBUG, "Found IDCODE: " + format("%08X", idcode));
    }
    else
    {
        logmsg(LOG_DEBUG, "Connect failed!");
        return 0xFFFFFFFF;
    }   
    return idcode;
}

////////////////////////////////////////////////////////////////////////////////
// Queuing functions
////////////////////////////////////////////////////////////////////////////////

// queue a read DP
function queueReadDP(address)
{
    queueUInt8(CMD_TYPE_READDP);
    queueUInt8(address);
}

// queue a write DP 
function queueWriteDP(address, value)
{
    queueUInt8(CMD_TYPE_WRITEDP);
    queueUInt8(address);
    queueUInt32(value);
}

// queue a read AP
function queueReadAP(address)
{
    queueUInt8(CMD_TYPE_READAP);
    queueUInt32(address);
}

// queue a write AP write
function queueWriteAP(address, value)
{
    queueUInt8(CMD_TYPE_WRITEAP);
    queueUInt32(address);
    queueUInt32(value);
}


// queue a read status DP read
function queueReadStat()
{
    queueReadDP(DP_CTRLSTAT);
}

// queue a read status DP write
function queueWriteStat(value)
{
    queueWriteDP(DP_CTRLSTAT, value)
}

// queue a write memory operation
function queueWriteMemory(address, value)
{
    queueUInt8(CMD_TYPE_WRITEMEM);
    queueUInt32(address);
    queueUInt32(value);
}

// queue a read memory operation
function queueReadMemory(address)
{
    queueUInt8(CMD_TYPE_READMEM);
    queueUInt32(address);
}


////////////////////////////////////////////////////////////////////////////////
// Immediate functions
////////////////////////////////////////////////////////////////////////////////

// read the status AP
function readStat()
{
    clearCmdQueue();
    queueReadStat();
    executeCmdQueue();
    local status = popUInt8();
    if (status == CMD_STATUS_OK)
    {
        return popUInt32();
    }
    else
    {
        logmsg(LOG_DEBUG, "readStat failed: " + status);
    }
    return 0;
}

// write the status AP
function writeStat(value)
{
    clearCmdQueue();
    queueWriteStat(value);
    executeCmdQueue();
    local status = popUInt8();
    if (status != CMD_STATUS_OK)
    {
        logmsg(LOG_DEBUG, "writeStat failed: " + status);
    }
    return status;
}

// read the AP
function readAP(address)
{
    clearCmdQueue();
    queueReadAP(address);
    executeCmdQueue();
    local status = popUInt8();
    if (status == CMD_STATUS_OK)
    {
        return popUInt32();
    }
    else
    {
        logmsg(LOG_DEBUG, "readAP failed: " + status + "\n");
    }
    return 0;
}

// write the AP
function writeAP(address, value)
{
    clearCmdQueue();
    queueWriteAP(address, value);
    executeCmdQueue();
    local status = popUInt8();
    if (status != CMD_STATUS_OK)
    {
        logmsg(LOG_DEBUG, "writeAP failed: " + status + "\n");
    }
    return status;
}


// read from memory
function readMemory(address)
{
    clearCmdQueue();
    queueReadMemory(address);
    executeCmdQueue();
    local status = popUInt8();
    if (status == CMD_STATUS_OK)
    {
        return popUInt32();
    }
    else
    {
        logmsg(LOG_DEBUG, "readMemory failed: " + status);
    }
    return 0;
}

// write to memory
function writeMemory(address, value)
{
    clearCmdQueue();
    queueWriteMemory(address, value);
    executeCmdQueue();
    local status = popUInt8();
    if (status != CMD_STATUS_OK)
    {
        logmsg(LOG_DEBUG, "writeMemory failed: " + status);
    }
    return status;
}

// read a core register
// this only works when the core is in debug mode!
function readCoreRegister(regID)
{
    // NOTE: system must be in debug mode!
    // first request a register read
    // then read it!
    writeMemory(SCS_DCRSR, regID);
    return readMemory(SCS_DCRDR);
}

// write to a core register
// this only works when the core is in debug mode!
function writeCoreRegister(regID, value)
{
    // NOTE: system must be in debug mode!
    // first write the register value
    // then request a register write
    writeMemory(SCS_DCRDR, value);        
    return writeMemory(SCS_DCRSR, (regID & 0x1F) | DCRSR_WRITE);
}

// dump all registers to the console
// this only works when the core is in debug mode!
function showRegisters()
{
    for(local i=0; i<16; i+=1)
    {
        print(format("r%d = %08X\n", i, readCoreRegister(i)));
    }
}

// clear sticky SWD errors
function clearStickyErrors()
{
    clearCmdQueue();
    queueUInt8(CMD_TYPE_WRITEDP);
    queueUInt8(DP_ABORT);
    queueUInt32(1+2+4+8+16);
    executeCmdQueue();
    local result = popUInt8();
    if (result != CMD_STATUS_OK)
    {
        logmsg(LOG_DEBUG, "clearStickyErrors failed: " + status);
    }
    return status;
}




logmsg(LOG_DEBUG, "Loaded targetfuncs.nut\n");
