// Target...: Kinetis KV10Zxx processors from Freescale/NXP
// Author...: Niels A. Moseley
// Version..: 0.1
// 
// This is experimental!
//

class TargetKV10Z extends TargetBase
{
    constructor()
    {
        name = "KV10Z"
        idcode = 0x11111111
    }
}

// create a target object
kv10ztarget <- TargetKV10Z();

// register the target
registerTarget(::kv10ztarget);
