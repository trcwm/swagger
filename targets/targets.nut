// 
// This file is used to load in all the target information.
//
// Each target must register itself via an auto-execute script
// that appends a target info table to the global targets table.
//
// The target info table must hold the follwing:
//   .name          - (string) the human readable name of the target
//   .idcode        - (integer) the IDCODE of the chip supported
//   .flasherase    - (function) a function that erases the entire flash
//   .upload        - (function) a function that uploads/fashes a binary file
//
// result_code flasherase()
//
// result_code upload(filepath, startaddress)
//
// Author...: Niels A. Moseley
// Version..: 0.1
// 
// This is experimental!
//

// *************************************
// global variables
// *************************************
targets <- [];      // create an empty targets array
targetIDx <- -1;    // current target in use, -1 if none selected

// *************************************
// Target baseclass
// override this to create targets
// *************************************
class TargetBase
{
    name = null
    idcode = null
    
    constructor()
    {
        // set name and idcode in the derived class
        name = "";
        idcode = 0;        
    }
     
    // override this in your derived class
    function flasherase() {}

    // override this in your derived class    
    function upload(filename) {}

    // check if the target is indeed
    // the one that is expected
    // return 0 if ok, else -1.
    // override this in your derived class
    function check()
    {
        return -1;
    }    
    
    // halt the target
    // return 0 if ok, else -1.
    function halt()
    {
        return -1;
    }
    
    // reset the target
    // state is either true(1)
    // or false(0).
    // return 0 if ok, else -1.
    function reset(state)
    {
        return -1;
    }
    
    function getName()
    {
        return name;
    }
    
    function getIDCode()
    {
        return idcode;
    }
}

// *************************************
// function to register a target
// *************************************
function registerTarget(target_info)
{
    // append the target to the targets table
    if (::debug > 0)
    {
        print("Target " + target_info.getName() + " added\n");
    }
    ::targets.append(target_info);
}

logmsg(LOG_DEBUG, "Loaded targets.nut\n");