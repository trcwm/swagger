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
    
    function halt()
    {
        // halt the target
        logmsg(LOG_ERROR, "Halt is not implemented");
        return -1;  // error
    }
    
    // check the state of target reset
    function isReset()
    {
        local retval = readAP(MDM_AP_STAT);
        if (retval.swdcode != SWD_OK)
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
            if (retval.swdcode != SWD_OK)
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
            if (retval.swdcode != SWD_OK)
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
        
        // check for flash ready bit
        local retval = readAP(MDM_AP_STAT);
        if (retval.swdcode != SWD_OK)
        {
            logmsg(LOG_ERROR, "Checking for flash failed (SWD not OK)\n");
            return -1;
        }
        if (retval.data & MDM_STAT_FLASHREADY)
        {
            logmsg(LOG_DEBUG, "Flash is ready.\n");
        }
        else
        {
            logmsg(LOG_ERROR, "Flash is not ready!\n");
            return -1;
        }
        
        // check system security
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
        
        // setup debug mode through SCS
        writeMemory(SCS_DHCSR, DHCSR_DBGKEY | C_DEBUGEN | C_HALT | C_MASKINTS);        
        
        retval = writeAP(MDM_AP_CTRL, MDM_CTRL_SYSRESETREQ | MDM_CTRL_DEBUGREQ | MDM_CTRL_COREHOLD);
        if (retval.swdcode != SWD_OK)
        {
            logmsg(LOG_ERROR, "Entering debug mode failed (SWD not OK)\n");
            return -1;
        }
        
        // release the system reset and enter debug mode
        retval = writeAP(MDM_AP_CTRL, MDM_CTRL_DEBUGREQ);
        if (retval.swdcode != SWD_OK)
        {
            logmsg(LOG_ERROR, "Reset release failed (SWD not OK)\n");
            return -1;
        }
        
        // check if we've entered debug mode
        retval = readAP(MDM_AP_STAT);
        if (retval.swdcode != SWD_OK)
        {
            logmsg(LOG_ERROR, "Checking for debug mode failed (SWD not OK)\n");
            return -1;
        }
        if (retval.data & MDM_STAT_COREHALTED)
        {
            logmsg(LOG_DEBUG, "Entered debug mode!\n");
        }
        else
        {
            logmsg(LOG_ERROR, "Entering debug mode failed - core not halted\n");
            return -1;
        }
        
        print(format("SCS_DHCSR = %08X\n", readMemory(SCS_DHCSR).data));
    }
    
}

// create a target object
kv10ztarget <- TargetKV10Z();

// register the target
registerTarget(::kv10ztarget);
