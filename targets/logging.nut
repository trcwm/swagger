//
// Swagger utility functions
//
// Author...: Niels A. Moseley
// Version..: 0.1
// 
// This is experimental!
//

// global constants
const LOG_ERROR     = 3;
const LOG_WARNING   = 2;
const LOG_INFO      = 1;
const LOG_DEBUG     = 0;

// logging/debug function
function logmsg(level, str)
{
    if (level >= ::debug)
        print(str)
}
