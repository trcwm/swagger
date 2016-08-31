//
// Swagger utility functions
//
// Author...: Niels A. Moseley
// Version..: 0.1
// 
// This is experimental!
//

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


logmsg(LOG_DEBUG, "Loaded utils.nut\n");
