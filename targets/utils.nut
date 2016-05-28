//
// Swagger utility functions
//
// Author...: Niels A. Moseley
// Version..: 0.1
// 
// This is experimental!
//

// write Access port
function writeAP(address, data)
{
    return dwrite(1, address, data);
}

// write Debug port
function writeDP(address, data)
{
    return dwrite(0, address, data);
}

// read Access port
// returns a table:
//   .swdcode - the SWD code
//   .data    - the 32-bit data
function readAP(address)
{
    return dread(1, address);
}

// read Debug port
// returns a table:
//   .swdcode - the SWD code
//   .data    - the 32-bit data
function readDP(address)
{
    return dread(0, address);
}



// *********************************************************
//   Specific functions
// *********************************************************

function APselect(reg_address)
{
    return writeDP(2, reg_address);
}

function readstat()
{
    return readDP(1);           // read the CTRL/STAT register
}

function writestat(data)
{
    return writeDP(1,data);     // write the CTRL/STAT register
}

// *********************************************************
//   Very Specific functions
// *********************************************************

function enableDebug()
{
    writestat(0x50000000);  // enable power to DEBUG and system
}

function clearStickyErrors()
{
    APselect(0);
    writeDP(0,1+2+4+8+16);  // write to ABORT register
}

function readMemory(address)
{
    APselect(0);            // select AHB-AP
    writeAP(0,0x22000012);  // set configuration register
    writeAP(1,address);     // write the address
    readAP(3);              // dummy read
    return readAP(3);
}

function writeMemory(address, data)
{
    APselect(0);            // select AHB-AP
    writeAP(0,0x22000012);  // set configuration register
    writeAP(1,address);     // write the address
    return writeAP(3, data);
}

