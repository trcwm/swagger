// Target...: LPC13xx processors from NXP
// Author...: Niels A. Moseley
// Version..: 0.1
// 
// This is experimental!
//

class TargetLPC13 extends TargetBase
{
    constructor()
    {
        name = "LPC13xx"
        idcode = 0x12345678
    }
}

// create a target object
lpc13target <- TargetLPC13();

// register the target
registerTarget(::lpc13target);
