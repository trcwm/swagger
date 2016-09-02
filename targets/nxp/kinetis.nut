//
// Swagger utility functions
//
// Author...: Niels A. Moseley
// Version..: 0.1
//
// support for kinetis FLASH programming
// 
// This is experimental!
//

const MDM_AP_STAT   = 0x01000000;   // debug status
const MDM_AP_CTRL   = 0x01000004;   // debug control
const MDM_AP_IDR    = 0x010000FC;   // debug ID

const MDM_CTRL_FLASHERASE   = 0x01; // flash erase in progress
const MDM_CTRL_DEBUGDIS     = 0x02; // debug disable
const MDM_CTRL_DEBUGREQ     = 0x04; // debug request
const MDM_CTRL_SYSRESETREQ  = 0x08; // system reset request
const MDM_CTRL_COREHOLD     = 0x10; // hold core just after reset

const MDM_STAT_FLASHACK     = 0x0001;   // flash mass erase ack
const MDM_STAT_FLASHREADY   = 0x0002;   // flash ready
const MDM_STAT_SYSSECURITY  = 0x0004;   // system security
const MDM_STAT_SYSRESET_N   = 0x0008;   // systtem reset state (active low)
const MDM_STAT_MASSERASE    = 0x0020;   // mass erase enable
const MDM_STAT_BACKDOOR     = 0x0040;   // backdoor access key enable
const MDM_STAT_LPENABLE     = 0x0080;   // low power stop mode enabled
const MDM_STAT_VLPM         = 0x0100;   // very low power mode active
const MDM_STAT_COREHALTED   = 0x010000;   
const MDM_STAT_SLEEPDEEP    = 0x020000;
const MDM_STAT_SLEEPING     = 0x040000;

const SIM_FCFG1         = 0x4004804C;       // flash configuration reg 1
const SIM_FCFG2         = 0x40048050;       // flash configuration reg 2

const SIM_SCGC6         = 0x4004803C;       // clock config reg 6
const SCGC6_FTF         = 0x1;              // flash clock gating bit

const FTFA_FSTAT        = 0x40020000;       // flash status register (8 bits!)
const FTFA_FCNFG        = 0x40020001;       // flash config register (8 bits!)
const FTFA_FSEC         = 0x40020002;
const FTFA_FOPT         = 0x40020003;
const FTFA_FCCOB_BASE   = 0x40020004;
const FTFA_PROTADDR     = 0x0408;          // flash protection bits

const FSTAT_CCIF        = 0x80;
const FSTAT_RDCOLERR    = 0x40;
const FSTAT_ACCERR      = 0x20;
const FSTAT_FPVIOL      = 0x10;
const FSTAT_MGSTAT0     = 0x01;

const MCM_PLACR         = 0xF000300C;       // Platform Control Register
const PLACR_ESFC        = 0x00010000;       // flash stall enable_n

// enable power to the internal debug system
// debugging/flashing won't work without this enabled!
function kinetis_enableDebugPower()
{
    writeStat(0x50000000);
        
    // wait for confirmation
    local timeout = 0;
    while((readStat() & 0xF0000000) != 0xF0000000)
    {
        timeout++;
        if (timeout > 100)
        {
            logmsg(LOG_ERROR, "ERROR: Debug system does not power up!\n");
            return 1;
        }
    }    
    return 0;
}

// halt the kinetis core
function kinetis_mdm_halt()
{
    // enable debug power, in case this hasn't been done
    if (kinetis_enableDebugPower() != 0)
    {
        return -1;
    }
    
    // setup debug mode through SCS
    local result = writeMemory(SCS_DHCSR, DHCSR_DBGKEY | C_DEBUGEN | C_HALT | C_MASKINTS);
    if (result != CMD_STATUS_OK)
    {
        logmsg(LOG_ERROR, "ERROR: kinetis_mdm_reset - write to SCS_DHCSR failed!\n");
        return -1;
    }
    
    // release the system reset and enter debug mode
    result = writeAP(MDM_AP_CTRL, MDM_CTRL_DEBUGREQ);
    if (result != CMD_STATUS_OK)
    {
        logmsg(LOG_ERROR, "ERROR: kinetis_mdm_reset - write to MDM_AP_CTRL failed!\n");
        return -1;
    }
    
    result = readAP(MDM_AP_STAT);
    if (result & MDM_STAT_COREHALTED)
    {
        logmsg(LOG_DEBUG, "Entered debug mode - core halted!\n");
        logmsg(LOG_DEBUG, format("PC  = %08X\n", readCoreRegister(15)));
        logmsg(LOG_DEBUG, format("MSP = %08X\n", readCoreRegister(DCRSR_REG_MSP)));   
    }
    else
    {
        logmsg(LOG_ERROR, "Entering debug mode failed - core not halted " + format("%08X",result) + "\n");
        return -1;        
    }
    return 0;
}

function kinetis_flash_longword(address,data)
{
    clearCmdQueue();
    // clear error flags
    queueWriteMemory(FTFA_FSTAT, 0xFFFE0000 | FSTAT_RDCOLERR | FSTAT_ACCERR | FSTAT_FPVIOL);
    
    // wait for the previous command to finish
    // if there was any        
    queuePollMemory(FTFA_FSTAT, FSTAT_CCIF);
    
    // set address and data
    queueWriteMemory(FTFA_FCCOB_BASE, 0x06000000 | (address & 0x00FFFFFF));
    queueWriteMemory(FTFA_FCCOB_BASE+4, data);

    // trigger flash write command
    queueWriteMemory(FTFA_FSTAT, 0xFFFE0000 | FSTAT_CCIF);
    // wait for the previous command to finish
    queuePollMemory(FTFA_FSTAT, FSTAT_CCIF);
    queueReadMemory(FTFA_FSTAT);
    
    // execute commands!
    if (executeCmdQueue() != 0)
    {
        logmsg(LOG_ERROR, "ERROR: command queue execution failed\n");
        return -1;
    }
        
    // get result of operations
    local result = popUInt8();
    if (result != CMD_STATUS_OK)
    {
        logmsg(LOG_ERROR, "ERROR: flash longword failed: " + result + "\n");
        return -1;
    }
    // report error from Flash controller, if any
    local retval = popUInt32();
    if (retval & FSTAT_MGSTAT0)
    {
        logmsg(LOG_ERROR, "Command executing error!\n");
        // Note: do not return because we need more information .. 
    }
    if (retval & FSTAT_RDCOLERR)
    {
        logmsg(LOG_ERROR, "Flash read/write collison!\n");
        return -1;
    }
    if (retval & FSTAT_ACCERR)
    {
        logmsg(LOG_ERROR, "Flash access error!\n");
        return -1;
    }
    if (retval & FSTAT_FPVIOL)
    {
        logmsg(LOG_ERROR, "Flash violation!\n");
        return -1;            
    }
    return 0;
}


function kinetis_flasherase()
{
    logmsg(LOG_DEBUG, "Attempting to erase the flash!\n");
    
    // reset the system
    setReset(0);
    
    if (kinetis_mdm_halt() != 0)
    {
        //return -1;
    }
        
    // make sure the flash has a clock
    if (writeMemory(SIM_SCGC6, SCGC6_FTF) != CMD_STATUS_OK)
    {
        logmsg(LOG_ERROR, "Could not set the FTF clock gating bit (SWD)\n");
        return -1;
    }
    
    // check for flash ready bit
    if (pollAP(MDM_AP_STAT, MDM_STAT_FLASHREADY) != CMD_STATUS_OK)
    {
        logmsg(LOG_ERROR, "Flash is not ready!\n");
        return -1;
    }
    else
    {
        logmsg(LOG_DEBUG, "Flash is ready!\n");
    }
    
    // read the flash protection bits
    //local retval = readMemory(FTF_PROTADDR);
    
    // check system security
    local retval = readAP(MDM_AP_STAT);
    if (retval & MDM_STAT_SYSSECURITY)
    {
        logmsg(LOG_ERROR, "Part is secured - cannot program (yet)\n");
        return -1;
    }
    else
    {
        logmsg(LOG_DEBUG, "Part is unsecured - full access available\n");
    }
                    
    // get the size of the flash
    retval = readMemory(SIM_FCFG1);
    
    // program size in KB
    local psize_tbl = array(16, -1);    // program size array
    psize_tbl[0] = 8;
    psize_tbl[1] = 16;
    psize_tbl[3] = 32;
    psize_tbl[5] = 64;
    psize_tbl[7] = 128;
    psize_tbl[9] = 256;
    psize_tbl[15] = 32;
    
    // sector size in bytes
    local secsize_tbl = array(16,-1);
    secsize_tbl[0] = 256;
    secsize_tbl[1] = 512;
    secsize_tbl[3] = 1024;
    secsize_tbl[5] = 2048;
    secsize_tbl[7] = 4096;
    secsize_tbl[9] = 4096;
    secsize_tbl[15] = 1024;
    
    local psize = psize_tbl[(retval >> 24) & 0x0F];
    local secsize = secsize_tbl[(retval >> 24) & 0x0F];
    
    logmsg(LOG_DEBUG, format("Flash = %d KB  Sector = %d Bytes\n", psize, secsize));
    
    if ((psize != 32) || (secsize != 1024))
    {
        logmsg(LOG_ERROR, "Wrong flash memory size - only the MKV10Z32 is supported!\n"); 
        return -1;
    }
    else
    {
        logmsg(LOG_DEBUG, "Flash memory size OK!\n"); 
    }
    
    // check if flash enabled!
    if ((retval & 1) == 1)
    {
        logmsg(LOG_ERROR, "Flash is disabled!\n"); 
        return -1;
    }
    
    // ************************************************************
    //   ERASING FLASH
    // ************************************************************
    
    if (writeAP(MDM_AP_CTRL, MDM_CTRL_SYSRESETREQ | MDM_CTRL_FLASHERASE) != CMD_STATUS_OK)
    {
        logmsg(LOG_ERROR, "Mass erase trigger failed (SWD)\n");
        return -1;
    }
            
    // read flash erase in-progress until it clears!
    if (pollAP_not(MDM_AP_CTRL, MDM_CTRL_FLASHERASE) == 0)
    {
        print("Flash mass erase successful!\n");
    }
    else
    {
        logmsg(LOG_ERROR, "Error: flash mass erase time-out!\n");
        return -1;
    }
    
    // use hardware reset to reset the target
    setReset(1);
    setReset(0);
    connect();
    
    kinetis_mdm_halt();
    
    return 0;   // ok
}


logmsg(LOG_DEBUG, "Loaded kinetis.nut\n");
