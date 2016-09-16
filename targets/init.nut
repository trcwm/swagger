//
// Swagger utility functions
//
// Author...: Niels A. Moseley
// Version..: 0.1
// 
// This is experimental!
//

// *************************************
// global constants
// *************************************

const LOG_ERROR     = 3;
const LOG_WARNING   = 2;
const LOG_INFO      = 1;
const LOG_DEBUG     = 0;

// *************************************
// global variables
// *************************************

debug <- 1;         // set the debug level


// *************************************
// initialize system 
// *************************************
function initSystem()
{
    try
    {    
        dofile(scriptDir + "logging.nut");
        
        if (verbose)
        {
            debug = 0;
        }
        
        logmsg(LOG_DEBUG, "Initializing...\n");
        
        dofile(scriptDir + "targetfuncs.nut");
        dofile(scriptDir + "targets.nut");
        dofile(scriptDir + "nxp\\kinetis.nut");
        dofile(scriptDir + "nxp\\mkv10z.nut");
        print("targets loaded!\n");
        
        sleep(200); // wait for programming interface to get online
        
        connect();
        
        if (!interactive)
        {
            if (targets[0].flash_erase() != 0)
            {
                return -1;
            }
            if (targets[0].flash_program() != 0)
            {
                return -1;
            }
            if (verifyFlash() != 0)
            {
                return -1;
            }
        }
        else
        {
            logmsg(LOG_INFO, "Interactive mode... \n");
        }
    }
    catch(error)
    {
        print("Error: " + error);
    }
}

initSystem();
