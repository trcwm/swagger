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

const LOG_ERROR     = 0;
const LOG_WARNING   = 1;
const LOG_DEBUG     = 2;

// *************************************
// global variables
// *************************************

debug <- 3;         // set the debug level


// *************************************
// initialize system 
// *************************************
function initSystem()
{
    try
    {    
        dofile("..\\targets\\logging.nut");
        
        logmsg(LOG_DEBUG, "Initializing...\n");
        
        dofile("..\\targets\\utils.nut");
        dofile("..\\targets\\targetfuncs.nut");
        dofile("..\\targets\\targets.nut");
        dofile("..\\targets\\nxp\\kinetis.nut");
        dofile("..\\targets\\nxp\\mkv10z.nut");
        dofile("..\\targets\\nxp\\lpc13.nut");
        print("targets loaded!\n");
    }
    catch(error)
    {
        print("Error: " + error);
    }
}

initSystem();
