//
// Swagger utility functions
//
// Author...: Niels A. Moseley
// Version..: 0.1
// 
// This is experimental!
//

DPACC <- 0;
APACC <- 1;


function APselect(reg_address)
{
    return dwrite(::DPACC, 2, reg_address);
}

function readstat()
{
    return dread(::DPACC, 1);
}

function writestat(data)
{
    return dwrite(::DPACC, 1, data);
}