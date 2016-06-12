// Target...: Kinetis KV10Zxx processors from Freescale/NXP
// Author...: Niels A. Moseley
// Version..: 0.1
// 
// This is experimental!
//
// Please see: AN4835 - Production Flash Programming
//                      Best Practices for Kinetis K- and L-series MCUs
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

class TargetKV10Z extends TargetBase
{

    constructor()
    {
        name = "KV10Z"
        idcode = 0x11111111
    }
    
    // processor identification check
    function check()
    {
        // check the processor IDCODE
        if (readDP(DP_IDCODE).data != 0x0BC11477)
        {
            logmsg(LOG_ERROR, "Processor ID code mismatch!");
            return -1;
        }
        else
        {
            logmsg(LOG_DEBUG, "Processor ID OK\n");
        }
        
        // check if the MDM-AP has the correct ID
        if (readAP(MDM_AP_IDR).data != 0x001C0020)
        {
            logmsg(LOG_ERROR, "MDM-AP ID mismatch!");
            return -1;
        }
        else
        {
            logmsg(LOG_DEBUG, "MDM-AP ID OK\n");
        }
        
        return 0;
    }

    function readCoreRegister(regID)
    {
        // NOTE: system must be in debug mode!
        // first request a register read
        // then read it!
        writeMemory(SCS_DCRSR, regID);        
        return readMemory(SCS_DCRDR);
    }
    
    function writeCoreRegister(regID, value)
    {
        // NOTE: system must be in debug mode!
        // first write the register value
        // then request a register write
        writeMemory(SCS_DCRDR, value);        
        return writeMemory(SCS_DCRSR, (regID & 0x1F) | DCRSR_WRITE);
    }    
    
    function showRegisters()
    {
        // check for halt
        local retval = readAP(MDM_AP_STAT);
        if (retval.data & MDM_STAT_COREHALTED)
        {
            // yes, halted
            for(local i=0; i<16; i+=1)
            {
                print(format("r%d = %08X\n", i, readCoreRegister(i).data));
            }
        }
        else
        {
            logmsg(LOG_ERROR, "Error: core not halted\n");
        }
    }
    
    function rresume()
    {
        // check for halt
        local retval = readAP(MDM_AP_STAT);
        if (retval.data & MDM_STAT_COREHALTED)
        {
            writeAP(MDM_AP_CTRL, 0);
            writeMemory(SCS_DHCSR, DHCSR_DBGKEY | C_DEBUGEN | C_MASKINTS)    
            logmsg(LOG_DEBUG, "Resuming..");
        }
        else
        {
            logmsg(LOG_WARNING, "Can't resume: core not halted");
        }
    }

    function halt()
    {
        // enable power to the debug system / core
        enableDebugPower();
        
        // setup debug mode through SCS
        local retval = writeMemory(SCS_DHCSR, DHCSR_DBGKEY | C_DEBUGEN | C_HALT | C_MASKINTS);
        if (retval.status != STAT_OK)
        {
            logmsg(LOG_ERROR, "Write memory failed\n");
            return -1;            
        }
                         
        // release the system reset and enter debug mode
        retval = writeAP(MDM_AP_CTRL, MDM_CTRL_DEBUGREQ);
        if (retval.status != STAT_OK)
        {
            logmsg(LOG_ERROR, "Reset release failed\n");
            return -1;
        }
        
        // check if we've entered debug mode
        retval = readAP(MDM_AP_STAT);
        if (retval.status != STAT_OK)
        {
            logmsg(LOG_ERROR, "Checking for debug mode failed\n");
            return -1;
        }
        if (retval.data & MDM_STAT_COREHALTED)
        {
            logmsg(LOG_DEBUG, "Entered debug mode - core halted!\n");
            logmsg(LOG_DEBUG, format("PC  = %08X\n", readCoreRegister(15).data));
            logmsg(LOG_DEBUG, format("MSP = %08X\n", readCoreRegister(DCRSR_REG_MSP).data));
            
        }
        else
        {
            logmsg(LOG_ERROR, "Entering debug mode failed - core not halted\n");
            return -1;
        }        
        
        return 0;  // OK!
    }

    // check the state of target reset
    function isReset()
    {
        local retval = readAP(MDM_AP_STAT);
        if (retval.status != STAT_OK)
        {
            logmsg(LOG_ERROR, "Target reset query command returned SWD FAULT\n");
            return -1;
        }
            
        if (!(retval.data & MDM_STAT_SYSRESET_N))
            return 0;
            
        return -1;
    }
    
    function reset(state)
    {
        // release or reset the target
        local retval;
        if (state == 1)
        {
            retval = writeAP(MDM_AP_CTRL, MDM_CTRL_SYSRESETREQ);            
            if (retval.status != STAT_OK)
            {
                logmsg(LOG_ERROR, "Reset request failed!\n");
                return -1;
            }
            else
            {
                logmsg(LOG_DEBUG, "Reset requested.\n");
                return 0;
            }   
        }
        else
        {
            // release reset
            retval = writeAP(MDM_AP_CTRL, 0);
            if (retval.status != STAT_OK)
            {
                logmsg(LOG_ERROR, "Reset de-assert failed!\n");
                return -1;
            }                
            else
            {
                logmsg(LOG_DEBUG, "Reset de-asserted.\n");    
                return 0;
            }                  
        }
        return -1;
    }
        
    function pollMemory(address, mask)
    {
        local retries = 0;
        while(retries < MAX_RETRIES)
        {
            local retval = readMemory(address);
            if ((retval.data & mask) == mask)
            {
                return 0;   // OK
            }
            retries++;
        }
        return -1;
    }
    
    function pollAP(address, mask)
    {   
        local retries = 0;
        while(retries < MAX_RETRIES)
        {
            local retval = readAP(address);
            if ((retval.data & mask) == mask)
            {
                return 0;   // OK
            }
            retries++;
        }
        return -1;
    }
    
    function pollAP_not(address, mask)
    {   
        local retries = 0;
        while(retries < MAX_RETRIES)
        {
            local retval = readAP(address);
            if ((retval.data & mask) != mask)
            {
                return 0;   // OK
            }
            retries++;
        }
        return -1;
    }    
    
    function securedErase()
    {
        if (writeAP(MDM_AP_CTRL, MDM_CTRL_SYSRESETREQ | MDM_CTRL_FLASHERASE).status != STAT_OK)
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
    }
    
    function flasherase()
    {
        logmsg(LOG_DEBUG, "Attempting to erase the flash!\n");
        
        // reset the system
        reset(1);
        if (isReset() == 0)
        {
            logmsg(LOG_DEBUG, "System reset succeeded!\n");
        }
        else
        {
            logmsg(LOG_ERROR, "System reset failed!\n");
            return -1;
        }
        
        if (halt() != 0)
        {
            //return -1;
        }
        
        // make sure the flash has a clock
        if (writeMemory(SIM_SCGC6, SCGC6_FTF).status != STAT_OK)
        {
            logmsg(LOG_ERROR, "Could not set the FTF clock gating bit (SWD)\n");
            return -1;
        }
                
        // check for flash ready bit
        if (pollAP(MDM_AP_STAT, MDM_STAT_FLASHREADY) != 0)
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
        if (retval.data & MDM_STAT_SYSSECURITY)
        {
            //logmsg(LOG_DEBUG, "Part is secured - using mass erase\n");
            logmsg(LOG_ERROR, "Part is secured - cannot program (yet)\n");
            return -1;
        }
        else
        {
            logmsg(LOG_DEBUG, "Part is unsecured - full access available\n");
        }
                    
        //print(format("SCS_DHCSR = %08X\n", readMemory(SCS_DHCSR).data));
        
        // get the size of the flash
        retval = readMemory(SIM_FCFG1);
        if (retval.status != STAT_OK)
        {
            logmsg(LOG_ERROR, "Checking reading SIM_FCFG1 (SWD not OK)\n");
            return -1;
        }
        
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
        
        local psize = psize_tbl[(retval.data >> 24) & 0x0F];
        local secsize = secsize_tbl[(retval.data >> 24) & 0x0F];
        
        logmsg(LOG_DEBUG, format("FLash = %d KB  Sector = %d Bytes\n", psize, secsize));
        
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
        if ((retval.data & 1) == 1)
        {
            logmsg(LOG_ERROR, "Flash is disabled!\n"); 
            return -1;
        }
        
        // ************************************************************
        //   ERASING FLASH
        // ************************************************************
        
        if (writeAP(MDM_AP_CTRL, MDM_CTRL_SYSRESETREQ | MDM_CTRL_FLASHERASE).status != STAT_OK)
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
        
        // reset part
        reset(1);
        reset(0);
        
        return 0;   // ok
    }
    
    
    function flash_program_longword(address, data)
    {
        // assumptions: we're in debug mode
        // and flash is ready and erased!
        
        // clear error flags
        writeMemory(FTFA_FSTAT, 0xFFFE0000 | FSTAT_RDCOLERR | FSTAT_ACCERR | FSTAT_FPVIOL);
        
        // wait for the previous command to finish
        // if there was any
        pollMemory(FTFA_FSTAT, FSTAT_CCIF);
        
        writeMemory(FTFA_FCCOB_BASE, 0x06000000 | (address & 0x00FFFFFF));
        writeMemory(FTFA_FCCOB_BASE+4, data);
        
        // trigger command! 
        writeMemory(FTFA_FSTAT, 0xFFFE0000 | FSTAT_CCIF);    // trigger command
        
        if (pollMemory(FTFA_FSTAT, FSTAT_CCIF) == 0)
        {
            logmsg(LOG_DEBUG, format("%08X <- %08X ok\n", address, data));
        }
        
        // report errors
        local retval = readMemory(FTFA_FSTAT);
        if (retval.data & FSTAT_MGSTAT0)
        {
            logmsg(LOG_ERROR, "Command executing error!\n");
            // Note: do not return.
        }
        if (retval.data & FSTAT_RDCOLERR)
        {
            logmsg(LOG_ERROR, "Flash read/write collison!\n");
            return -1;
        }
        if (retval.data & FSTAT_ACCERR)
        {
            logmsg(LOG_ERROR, "Flash access error!\n");
            return -1;
        }
        if (retval.data & FSTAT_FPVIOL)
        {
            logmsg(LOG_ERROR, "Flash violation!\n");
            return -1;            
        }
        
        return 0;
    }
    
    function flash_program()
    {
        local myfile = file("board_init.bin","rb");   
        local myblob = myfile.readblob(myfile.len());        
        print(format("Binary data is %d bytes\n", myblob.len()));
                
        local idx = 0;
        while(idx < myblob.len())
        {
            local word = myblob.readn('i');
            print(format("(%08X) <- %08X\n", idx, word));
            
            // program the flash but skip
            // the all-set word, as the erased flash
            // has these bits set already -> faster programming
            if (word != 0xFFFFFFFF)
            {
                if (flash_program_longword(idx, word) != 0)
                {
                    logmsg(LOG_ERROR,"Flashing failed :-@\n");
                    return -1;
                }
            }
            idx += 4;
        }
        
        myfile.close();
    }
    
    function ftest()
    {
        local myfile = file("stubby.bin","rb");   
        local myblob = myfile.readblob(myfile.len());        
        print(format("Binary data is %d bytes\n", myblob.len()));
                
        local idx = 0;
        local addr = 0x20000000
        while(idx < myblob.len())
        {
            local word = myblob.readn('i');            
            writeMemory(addr, word);
            print(format("(%08X) <- %08X\n", addr, word));
            idx += 4;
            addr+= 4;
        }
        myfile.close();
        writeCoreRegister(DCRSR_REG_DBGRET, 0x20000000);    // set PC to RAM
        print(format("PC = %08X\n", readCoreRegister(DCRSR_REG_DBGRET).data))
        writeAP(MDM_AP_CTRL, 0);    // release halt
        writeMemory(SCS_DHCSR, DHCSR_DBGKEY | C_DEBUGEN | C_MASKINTS)
        
        local retval = readAP(MDM_AP_STAT);

        if (retval.data & MDM_STAT_COREHALTED)
        {
            print("Core still halted.. \n");
        }
        //  retval = writeAP(MDM_AP_CTRL, MDM_CTRL_DEBUGREQ);
        //   writeMemory(SCS_DHCSR, DHCSR_DBGKEY | C_HALT | C_DEBUGEN | C_MASKINTS)        
    }
}

// create a target object
kv10ztarget <- TargetKV10Z();

// register the target
registerTarget(::kv10ztarget);

