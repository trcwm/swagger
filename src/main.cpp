/*

  Swagger - A tool for programming ARM processors using the SWD protocol

  Niels A. Moseley

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <stdarg.h>

#include <QCoreApplication>

#include "cobs.h"
#include "hardwareinterface.h"

#include <squirrel.h>
#include <sqstdblob.h>
#include <sqstdsystem.h>
#include <sqstdio.h>
#include <sqstdmath.h>
#include <sqstdstring.h>
#include <sqstdaux.h>

// note: http://wiki.squirrel-lang.org/default.aspx/SquirrelWiki/Embedding%20Getting%20Started.html
// http://wiki.squirrel-lang.org/default.aspx/SquirrelWiki/SquirrelCallToCpp.html

#ifdef SQUNICODE
#define scfprintf fwprintf
#define scvprintf vfwprintf
#else
#define scfprintf fprintf
#define scvprintf vfprintf
#endif

#define VERSION "0.1"

// dirty hack ..
HardwareInterface* g_interface = NULL;


void printfunc(HSQUIRRELVM SQ_UNUSED_ARG(v),const SQChar *s,...)
{
    va_list vl;
    va_start(vl, s);
    scvprintf(stdout, s, vl);
    va_end(vl);
    (void)v; /* UNUSED */
}

void errorfunc(HSQUIRRELVM SQ_UNUSED_ARG(v),const SQChar *s,...)
{
    va_list vl;
    va_start(vl, s);
    scvprintf(stderr, s, vl);
    va_end(vl);
}

void register_global_func(HSQUIRRELVM v, SQFUNCTION f, const char *fname)
{
    sq_pushroottable(v);
    sq_pushstring(v,fname,-1);
    sq_newclosure(v,f,0);
    sq_newslot(v,-3,SQFalse);
    sq_pop(v,1); //pops the root table
}

void register_global_func(HSQUIRRELVM v, SQFUNCTION f, const char *fname, SQUserPointer *userPtr)
{
    sq_pushroottable(v);
    sq_pushstring(v,fname,-1);
    sq_pushuserpointer(v, userPtr);
    sq_newclosure(v,f,1);
    sq_setparamscheck(v,1,NULL);
    sq_newslot(v,-3,SQFalse);
    sq_pop(v,1); //pops the root table
}

void compile_error_handler(HSQUIRRELVM v, const SQChar* desc, const SQChar* source, SQInteger line, SQInteger column)
{
    printf("Error in %s:%d:%d %s\n", source, line, column, desc);
}

// *****************************************
// ** CUSTOM SQUIRREL FUNCTIONS
// *****************************************

SQInteger quit(HSQUIRRELVM v)
{
    int *done;
    sq_getuserpointer(v,-1,(SQUserPointer*)&done);
    *done=1;
    return 0;
}

SQInteger enumerateSerialPorts(HSQUIRRELVM v)
{
    HardwareInterface::printInterfaces();

    return 0;   // no arguments are returned.
}


/** Squirrel command: targetReset(int v)

    Sets the state of the target's reset pin.

   */
SQInteger targetReset(HSQUIRRELVM v)
{
    SQInteger nargs = sq_gettop(v);  // get number of arguments

    if (nargs != 2)
        return 0;   // no arguments returned;

    SQInteger vReset;
    if (SQ_SUCCEEDED(sq_getinteger(v, -1, &vReset)))
    {
        if (g_interface != 0)
        {            
            //TODO: error checking..
            g_interface->setTargetReset(vReset > 0);
        }
        else
        {
            printf("Error: no connection with interface\n");
        }
    }
    return 0;
}

/** Squirrel command: connect()

    connect to target
    returns a table:
      .swdcode - SWD return code
      .idcode  - the IDCODE of the connected part

   */
SQInteger targetConnect(HSQUIRRELVM v)
{
    SQInteger nargs = sq_gettop(v);  // get number of arguments

    if (g_interface == 0)
    {
        printf("Error: can't connect - open an interface first!");
        return 0;
    }

    uint32_t idcode;
    HWResult result = g_interface->connect(idcode);

    // create a table as a return argument
    sq_newtable(v);

    sq_pushstring(v, _SC("swdcode"), -1);
    sq_pushinteger(v, result);
    sq_newslot(v, -3, SQFalse);

    sq_pushstring(v, _SC("idcode"), -1);
    sq_pushinteger(v, idcode);
    sq_newslot(v, -3, SQFalse);

    printf("Connect called");

    return 1;   //return one item (table)
}

/** Squirrel command: read(APnDP, address)

    connect to target
    returns a table:
      .swdcode - SWD return code
      .data    - the data read

   */
SQInteger doReadTransaction(HSQUIRRELVM v)
{
    SQInteger nargs = sq_gettop(v);  // get number of arguments

    if (g_interface == 0)
    {
        printf("Error: can't connect - open an interface first!");
        return 0;
    }

    if (nargs != 3)
        return 0;   // no arguments returned;

    SQInteger APnDP, address, data;
    if (!SQ_SUCCEEDED(sq_getinteger(v, -1, &APnDP)))
    {
        printf("First argument should be an integer");
        return 0;
    }
    if (!SQ_SUCCEEDED(sq_getinteger(v, -2, &address)))
    {
        printf("Second argument should be an integer");
        return 0;
    }

    printf("APnDP  : %d\n", APnDP);
    printf("Address: %08X\n", address);

    uint32_t my_data;
    HWResult result = g_interface->readRegister((APnDP > 0), address, my_data);

    // create a table as a return argument
    sq_newtable(v);

    sq_pushstring(v, _SC("swdcode"), -1);
    sq_pushinteger(v, result);
    sq_newslot(v, -3, SQFalse);

    sq_pushstring(v, _SC("data"), -1);
    sq_pushinteger(v, my_data);
    sq_newslot(v, -3, SQFalse);

    printf("Data: %08X\n", my_data);

    return 1;   //return one item (table)
}



/** Squirrel command: write(<int>APnDP, <int>address, <int>data)

    perform a write transaction
    returns a swdcode as an integers

   */
SQInteger doWriteTransaction(HSQUIRRELVM v)
{
    SQInteger nargs = sq_gettop(v);  // get number of arguments

    if (g_interface == 0)
    {
        printf("Error: can't connect - open an interface first!");
        return 0;
    }

    if (nargs != 4)
        return 0;   // no arguments returned;

    SQInteger APnDP, address, data;
    if (!SQ_SUCCEEDED(sq_getinteger(v, -1, &APnDP)))
    {
        printf("First argument should be an integer");
        return 0;
    }
    if (!SQ_SUCCEEDED(sq_getinteger(v, -2, &address)))
    {
        printf("Second argument should be an integer");
        return 0;
    }
    if (!SQ_SUCCEEDED(sq_getinteger(v, -3, &data)))
    {
        printf("Third argument should be an integer");
        return 0;
    }

    printf("APnDP  : %d\n", APnDP);
    printf("Address: %08X\n", address);
    printf("data   : %08X\n", data);

    HWResult result = g_interface->writeRegister((APnDP > 0), address, data);

    sq_pushinteger(v, result);
    return 1;   //return one item (table)
}



/** Squirrel command: <string> getInterfaceName()

    Sets the state of the target's reset pin.

   */
SQInteger getInterfaceName(HSQUIRRELVM v)
{
    SQInteger nargs = sq_gettop(v);  // get number of arguments

    if (g_interface != 0)
    {
        std::string name;
        if (g_interface->queryInterfaceName(name))
        {
            sq_pushstring(v, name.c_str(),-1);
            return 1; // return one argument
        }
        else
        {
            //TODO: error reporting?
        }
    }
    else
    {
        printf("Error: no connection with interface\n");
    }
    return 0; // no return variables
}



/** Squirrel command: openInterface(string interfacename) */

SQInteger openInterface(HSQUIRRELVM v)
{
    SQInteger nargs = sq_gettop(v);  // get number of arguments

    if (nargs != 2)
        return 0;   // no arguments returned;

    // try to open an interface by integer (windows only)
    // FIXME: check for win32
    SQInteger vComport;
    const char *deviceStrPtr;
    if (SQ_SUCCEEDED(sq_getinteger(v, -1, &vComport)))
    {
        std::stringstream deviceName;
        deviceName << "\\\\.\\COM" << vComport;

        if (g_interface != 0)
            delete g_interface;

        g_interface = HardwareInterface::open(deviceName.str().c_str(), 19200);

        if (g_interface == 0)
            printf("Error: could not open interface %s\n", deviceName.str().c_str());
        else
            printf("Interface opened\n");
    }
    else if (SQ_SUCCEEDED(sq_getstring(v, -1, &deviceStrPtr)))
    {
        if (g_interface != 0)
            delete g_interface;

        std::string deviceName("/dev/tty.");
        deviceName.append(deviceStrPtr);
        g_interface = HardwareInterface::open(deviceName.c_str(), 19200);

        if (g_interface == 0)
            printf("Error: could not open interface %s\n", deviceName.c_str());
        else
            printf("Interface opened\n");
    }
    return 0;
}

/** Squirrel command: closeInterface() */

SQInteger closeInterface(HSQUIRRELVM v)
{
    SQInteger nargs = sq_gettop(v);  // get number of arguments

    if (g_interface != 0)
    {
        g_interface->close();
        g_interface = 0;
        printf("Interface closed\n");
    }
    return 0;
}

bool doScript(HSQUIRRELVM v, const char *fileName)
{
    //sq_pushroottable(v);

    if (SQ_SUCCEEDED(sqstd_dofile(v, fileName, SQFalse, SQTrue)))
    {
        return true;
    }
    return false;
}

void Interactive(HSQUIRRELVM v)
{
    #define MAXINPUT 1024
    SQChar buffer[MAXINPUT];
    SQInteger blocks =0;
    SQInteger string=0;
    SQInteger retval=0;
    SQInteger done=0;

    register_global_func(v, quit, _SC("quit"), (SQUserPointer *)&done);

    while (!done)
    {
        SQInteger i = 0;
        scprintf(_SC("\nsq>"));
        for(;;) {
            int c;
            if(done)return;
            c = getchar();
            if (c == _SC('\n')) {
                if (i>0 && buffer[i-1] == _SC('\\'))
                {
                    buffer[i-1] = _SC('\n');
                }
                else if(blocks==0)break;
                buffer[i++] = _SC('\n');
            }
            else if (c==_SC('}')) {blocks--; buffer[i++] = (SQChar)c;}
            else if(c==_SC('{') && !string){
                    blocks++;
                    buffer[i++] = (SQChar)c;
            }
            else if(c==_SC('"') || c==_SC('\'')){
                    string=!string;
                    buffer[i++] = (SQChar)c;
            }
            else if (i >= MAXINPUT-1) {
                scfprintf(stderr, _SC("sq : input line too long\n"));
                break;
            }
            else{
                buffer[i++] = (SQChar)c;
            }
        }
        buffer[i] = _SC('\0');

        if(buffer[0]==_SC('=')){
            scsprintf(sq_getscratchpad(v,MAXINPUT),(size_t)MAXINPUT,_SC("return (%s)"),&buffer[1]);
            memcpy(buffer,sq_getscratchpad(v,-1),(scstrlen(sq_getscratchpad(v,-1))+1)*sizeof(SQChar));
            retval=1;
        }
        i=scstrlen(buffer);
        if(i>0){
            SQInteger oldtop=sq_gettop(v);
            if(SQ_SUCCEEDED(sq_compilebuffer(v,buffer,i,_SC("interactive console"),SQTrue))){
                sq_pushroottable(v);
                if(SQ_SUCCEEDED(sq_call(v,1,retval,SQTrue)) &&  retval){
                    scprintf(_SC("\n"));
                    sq_pushroottable(v);
                    sq_pushstring(v,_SC("print"),-1);
                    sq_get(v,-2);
                    sq_pushroottable(v);
                    sq_push(v,-4);
                    sq_call(v,2,SQFalse,SQTrue);
                    retval=0;
                    scprintf(_SC("\n"));
                }
            }

            sq_settop(v,oldtop);
        }
    }
}


int main(int argc, char *argv[])
{
    QCoreApplication coreApplication(argc, argv);

    HSQUIRRELVM v;
    SQInteger retval = 0;

    COBS::test();

    printf("Swagger version " VERSION " "__DATE__"\n");
    printf("Using %s (%d bits)\n",SQUIRREL_VERSION,((int)(sizeof(SQInteger)*8)));
    printf("\nuse:\n");
    printf("  quit() to exit.\n");
    printf("  openInterface(<integer> | <string>)\n");
    printf("  closeInterface()\n");
    printf("  showSerial() to enumerate serial interfaces.\n");
    printf("  table{.swdcode,.idcode} = connect() to connect to the target.\n");
    printf("  swdcode = write(APnDP, address/4, data) to write to a register.\n");
    printf("  table{.swdcode,.data} = read(APnDP, address/4) to read a register.\n");
    printf("\n");

    v=sq_open(1024);
    sq_enabledebuginfo(v, SQTrue);
    sq_pushroottable(v);

    sq_setprintfunc(v, printfunc, errorfunc);

    sqstd_register_bloblib(v);
    sqstd_register_iolib(v);
    sqstd_register_systemlib(v);
    sqstd_register_mathlib(v);
    sqstd_register_stringlib(v);

    // register our own functions
    register_global_func(v, enumerateSerialPorts, _SC("showSerial"));

    register_global_func(v, openInterface, _SC("openInterface"));
    register_global_func(v, closeInterface, _SC("closeInterface"));
    register_global_func(v, getInterfaceName, _SC("getInterfaceName"));
    register_global_func(v, targetConnect, _SC("connect"));
    register_global_func(v, doWriteTransaction, _SC("write"));
    register_global_func(v, doReadTransaction, _SC("read"));
    register_global_func(v, targetReset, _SC("targetReset"));

    // load all the targets
    sq_setcompilererrorhandler(v, compile_error_handler);
    if (!doScript(v, "..\\targets\\targets.nut"))
    {
        printf("Cannot execute targets.nut script!\n");
    }

    Interactive(v);

    sq_close(v);

    if (g_interface != 0)
        g_interface->close();

    return 0;
}
