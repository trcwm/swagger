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

// global constants
const MAX_RETRIES = 100;

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
    logmsg(LOG_DEBUG, "Reset " + state + "\n");
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
    local retries = 0;
    logmsg(LOG_DEBUG, "Connecting...\n");
    while(retries < 10)
    {
        clearCmdQueue();
        queueUInt8(CMD_TYPE_CONNECT);
        executeCmdQueue();
        local result = popUInt8();
        local idcode = popUInt32();
        if (result == CMD_STATUS_OK)
        {
            logmsg(LOG_DEBUG, "Found IDCODE: " + format("%08X", idcode) + "\n");
            return idcode;
        }
        retries++;
    }
    
    logmsg(LOG_DEBUG, "Connect failed!");
    return -1
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

// queue poll memory
function queuePollMemory(address, mask)
{
    queueUInt8(CMD_TYPE_WAITMEMTRUE);
    queueUInt32(address);
    queueUInt32(mask);
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

// poll AP, returns CMD_STATUS_OK if
// bits specified by mask are all
// ones, or CMD_STATUS_TIMEOUT if 
// a time-out occurred.
function pollAP(address, mask)
{
    //logmsg(LOG_DEBUG, "pollAP\n");
    local retries = 0;
    while(retries < MAX_RETRIES)
    {
        clearCmdQueue();
        queueReadAP(address);
        executeCmdQueue();
        local status = popUInt8();
        if (status != CMD_STATUS_OK)
            return status;
            
        local v = popUInt32();
        if ((v & mask) == mask)
            return CMD_STATUS_OK;
        retries++;
    }
    return CMD_STATUS_TIMEOUT;
}

// poll AP, returns CMD_STATUS_OK if
// bits specified by mask are all
// ones, or CMD_STATUS_TIMEOUT if 
// a time-out occurred.
function pollAP_not(address, mask)
{
    //logmsg(LOG_DEBUG, "pollAP\n");
    local retries = 0;
    while(retries < MAX_RETRIES)
    {
        clearCmdQueue();
        queueReadAP(address);
        executeCmdQueue();
        local status = popUInt8();
        if (status != CMD_STATUS_OK)
            return status;
            
        local v = popUInt32();
        if ((v & mask) != mask)
            return CMD_STATUS_OK;
        retries++;
    }
    return CMD_STATUS_TIMEOUT;
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

// read a number of words from memory
// returns an array containing 
// the memory contents
function readMemoryWords(address, words)
{
    clearCmdQueue(); 
    if (words > 6)
    {
        logmsg(LOG_ERROR, "Error: readMemoryWords failed: too many words requested\n");
        return -1;
    }
    local contents = array(0,0);
    for(local i=0; i<words; i++)
    {
        queueReadMemory(address);
        address = address + 4;
    }
    executeCmdQueue();
    local status = popUInt8();
    if (status == CMD_STATUS_OK)
    {
        for(local i=0; i<words; i++)
        {
            contents.append(popUInt32());
        }
        return contents;
    }
    logmsg(LOG_ERROR, "Error: readMemoryWords failed: " + format("%02X", status) + "\n");
    return status;
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
        logmsg(LOG_ERROR, "writeMemory failed: " + status);
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

// compare the flash with the binary file
function verifyFlash()
{
    logmsg(LOG_INFO, "Verifying " + binFile + "\n");
    local myfile;
    try
    {
        myfile = file(binFile,"rb");
    }
    catch(error)
    {
        logmsg(LOG_ERROR, "Cannot open file " + binFile + "\n");
        return -1;
    }
    
    local myblob = myfile.readblob(myfile.len());    
    logmsg(LOG_INFO, format("Binary data is %d bytes\n", myblob.len()));
    
    // check that the binary file is indeed a whole number of words
    if ((myblob.len() % 4) != 0)
    {
        logmsg(LOG_WARNING, format("Data is not a whole number of 32-bit words!\n"));
    }
    
    local wordsLeft = myblob.len() / 4;
    local address = 0;
    while(wordsLeft > 0)
    {
        local wordCount = wordsLeft;    // number of words to read in one go.
        if (wordCount > 6)
        {
            wordCount = 6;
        }
        
        // get the memory contents in an array
        local targetContents = readMemoryWords(address, wordCount);
        if (typeof targetContents != "array")
        {
            logmsg(LOG_ERROR, "ERROR: Expected array\n");
            return -1;
        }
        
        // compare the memory contents with the file
        for(local i=0; i<wordCount; i++)
        {
            local word = myblob.readn('i'); // read word from file
            if (word != targetContents[i])
            {
                logmsg(LOG_ERROR, "\nVerify failed at address " + format("0x%08X",address+(i*4)) + "\n");
                logmsg(LOG_ERROR, "  flash: " + format("0x%08X",targetContents[i]) + "\n  file : " + format("0x%08X",word) + "\n");
                return -1;
            }
        }
        address += wordCount*4;
        wordsLeft -= wordCount;
    }
    logmsg(LOG_INFO, "\nVerify complete!\n");
    
    myfile.close();    
    return 0;
}

// clear sticky SWD errors
function clearStickyErrors()
{
    clearCmdQueue();
    queueUInt8(CMD_TYPE_WRITEDP);
    queueUInt8(DP_ABORT);
    queueUInt32(1+2+4+8+16);
    if (executeCmdQueue() == 0)
    {
        local result = popUInt8();
        if (result != CMD_STATUS_OK)
        {
            logmsg(LOG_ERROR, "ERROR: clearStickyErrors failed: " + result);
            return status;
        }
        else
        {
            return CMD_STATUS_OK;
        }
    }
    return CMD_STATUS_PROTOERR;
}




logmsg(LOG_DEBUG, "Loaded targetfuncs.nut\n");
