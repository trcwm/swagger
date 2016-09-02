// Target...: Kinetis KV10Zxx processors from Freescale/NXP
// Author...: Niels A. Moseley
// Version..: 0.1
// 
// This is experimental!
//
// Please see: AN4835 - Production Flash Programming
//                      Best Practices for Kinetis K- and L-series MCUs
//

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
     

    function rresume()
    {
        // check for halt
        local retval = readAP(MDM_AP_STAT);
        if (retval & MDM_STAT_COREHALTED)
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
        return kinetis_mdm_halt()
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
            if (retval != CMD_STATUS_OK)
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
            if (retval != CMD_STATUS_OK)
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
    
    function flash_erase()
    {
        return kinetis_flasherase();
    }
        
    function flash_program_longword(address, data)
    {
        return kinetis_flash_longword(address, data);
    }
    
    function flash_program()
    {
        logmsg(LOG_INFO, "Flashing " + binFile + "\n");
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
                
        local idx = 0;
        while(idx < myblob.len())
        {
            local word = myblob.readn('i');
            logmsg(LOG_INFO, format("(%08X) <- %08X\r", idx, word));
            
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
        
        logmsg(LOG_INFO, "\n");
        
        setReset(1);
        setReset(0);
        
        return 0;
    }
}

// create a target object
kv10ztarget <- TargetKV10Z();

// register the target
registerTarget(::kv10ztarget);

