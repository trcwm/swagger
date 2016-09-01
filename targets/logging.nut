//
// Swagger utility functions
//
// Author...: Niels A. Moseley
// Version..: 0.1
// 
// This is experimental!
//

// global constants
const LOG_ERROR     = 0;
const LOG_WARNING   = 1;
const LOG_DEBUG     = 2;
const LOG_INFO      = 3;

// logging/debug function
function logmsg(level, str)
{
    if (::debug >= level)
        print(str)
}
