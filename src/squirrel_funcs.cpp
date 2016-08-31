/*

  Swagger - A tool for programming ARM processors using the SWD protocol

  Niels A. Moseley (c) Moseley Instruments 2016

*/


#include "squirrel_funcs.h"
#include "hardwareinterface.h"

extern HardwareInterface* g_interface;

std::vector<uint8_t> g_cmdQueue;        // global command queue to programmer
std::vector<uint8_t> g_resultQueue;     // global result queue from programmer

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


/** Squirrel command: queue uint8_t in the command queue*/
SQInteger queueUInt8(HSQUIRRELVM v)
{
    SQInteger nargs = sq_gettop(v);  // get number of arguments

    if (nargs != 2)
    {
        printf("Error: queueUint8 does not have enough parameters\n");
        return 0;   // error, not enough
    }

    SQInteger byte;
    if (SQ_SUCCEEDED(sq_getinteger(v, -1, &byte)))
    {
        g_cmdQueue.push_back(byte);
    }
    else
    {
        printf("Error: queueUint8 parameter is not an integer\n");
    }
    return 0;   // no parameters returned
}


/** Squirrel command: queue uint32_t int the command queue*/
SQInteger queueUInt32(HSQUIRRELVM v)
{
    SQInteger nargs = sq_gettop(v);  // get number of arguments

    if (nargs != 2)
    {
        printf("Error: queueUint32 does not have enough parameters\n");
        return 0;   // error, not enough
    }

    SQInteger word;
    if (SQ_SUCCEEDED(sq_getinteger(v, -1, &word)))
    {
        g_cmdQueue.push_back(word & 0xFF); // LSB first
        g_cmdQueue.push_back((word>>8) & 0xFF);
        g_cmdQueue.push_back((word>>16) & 0xFF);
        g_cmdQueue.push_back((word>>24) & 0xFF);
    }
    else
    {
        printf("Error: queueUint32 parameter is not an integer\n");
    }
    return 0;   // no parameters returned
}


/** Squirrel command: execute command queue */
SQInteger executeCmdQueue(HSQUIRRELVM v)
{
    // no arguments required
    if (g_interface->writePacket(g_cmdQueue)==false)
    {
        printf("Error: writePacket %s\n", g_interface->getLastError().c_str());
        sq_pushinteger(v, 1);
        // error transmitting
    }
    g_resultQueue.clear();
    if (g_interface->readPacket(g_resultQueue)==false)
    {
        printf("Error: readPacket %s\n", g_interface->getLastError().c_str());
        sq_pushinteger(v, 2);
        // error receiving
    }
    sq_pushinteger(v, 0);
    return 1;   // result code
}


/** Squirrel command: pop uint8_t from result queue */
SQInteger popUInt8(HSQUIRRELVM v)
{
    if (g_resultQueue.size() > 0)
    {
        uint8_t byte = g_resultQueue.front();
        g_resultQueue.erase(g_resultQueue.begin());
        sq_pushinteger(v, byte);
    }
    else
    {
        sq_pushinteger(v, 0);
    }
    return 1;
}


/** Squirrel command: pop uint32_t from result queue */
SQInteger popUInt32(HSQUIRRELVM v)
{
    if (g_resultQueue.size() > 3)
    {
        // FIXME: ugly code, would be better with an operation
        // that does not require freeing memory
        uint32_t word = (uint32_t)g_resultQueue.front();   // LSB first
        g_resultQueue.erase(g_resultQueue.begin());
        word |= ((uint32_t)g_resultQueue.front()) << 8;
        g_resultQueue.erase(g_resultQueue.begin());
        word |= ((uint32_t)g_resultQueue.front()) << 16;
        g_resultQueue.erase(g_resultQueue.begin());
        word |= ((uint32_t)g_resultQueue.front()) << 24;
        g_resultQueue.erase(g_resultQueue.begin());
        sq_pushinteger(v, word);
    }
    else
    {
        sq_pushinteger(v, 0);
    }
    return 1;
}

SQInteger dumpCmdQueue(HSQUIRRELVM v)
{
    uint32_t N=g_cmdQueue.size();
    printf("Command queue size = %d bytes\n", N);
    for(uint32_t i=0; i<N; i++)
    {
        printf(" %02X", g_cmdQueue[i]);
    }
    printf("\n");
    return 0;
}

SQInteger printLastPacketError(HSQUIRRELVM v)
{
    if (g_interface != 0)
        printf("%s\n", g_interface->getLastError().c_str());
    return 0;
}


SQInteger dumpResultQueue(HSQUIRRELVM v)
{
    uint32_t N=g_resultQueue.size();
    printf("Result queue size = %d bytes\n", N);
    for(uint32_t i=0; i<N; i++)
    {
        printf(" %02X", g_resultQueue[i]);
    }
    printf("\n");
    return 0;
}

SQInteger clearCmdQueue(HSQUIRRELVM v)
{
    g_cmdQueue.clear();
    return 0;
}


#if 0
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

    printf("Connected to target with ID: %08X\n", idcode);

    return 1;   //return one item (table)
}


/** Squirrel command: readDP(address)
    returns a table:
      .status - 1 for OK
      .data   - the data read
*/
SQInteger readDP(HSQUIRRELVM v)
{
    SQInteger nargs = sq_gettop(v);  // get number of arguments

    if (g_interface == 0)
    {
        printf("Error: can't connect - open an interface first!");
        return 0;
    }

    if (nargs != 2)
    {
        printf("Incorrect number of arguments\n");
        return 0;   // no arguments returned;
    }

    SQInteger address;
    if (!SQ_SUCCEEDED(sq_getinteger(v, -1, &address)))
    {
        printf("First argument should be an integer");
        return 0;
    }

    uint32_t my_data;
    HWResult result = g_interface->readDP(address, my_data);

    // create a table as a return argument
    sq_newtable(v);

    sq_pushstring(v, _SC("status"), -1);
    sq_pushinteger(v, result);
    sq_newslot(v, -3, SQFalse);

    sq_pushstring(v, _SC("data"), -1);
    sq_pushinteger(v, my_data);
    sq_newslot(v, -3, SQFalse);

    return 1;   //return one item (table)
}

/** Squirrel command: writeDP(address)
    returns a table:
      .status - 1 for OK
*/
SQInteger writeDP(HSQUIRRELVM v)
{
    SQInteger nargs = sq_gettop(v);  // get number of arguments

    if (g_interface == 0)
    {
        printf("Error: can't connect - open an interface first!");
        return 0;
    }

    if (nargs != 3)
    {
        printf("Incorrect number of arguments\n");
        return 0;   // no arguments returned;
    }

    SQInteger address, data;
    if (!SQ_SUCCEEDED(sq_getinteger(v, -1, &data)))
    {
        printf("Second argument should be an integer");
        return 0;
    }
    if (!SQ_SUCCEEDED(sq_getinteger(v, -2, &address)))
    {
        printf("First argument should be an integer");
        return 0;
    }

    //printf("%d %d\n", address, data);

    HWResult result = g_interface->writeDP(address, data);

    // create a table as a return argument
    sq_newtable(v);

    sq_pushstring(v, _SC("status"), -1);
    sq_pushinteger(v, result);
    sq_newslot(v, -3, SQFalse);

    return 1;   //return one item (table)
}


/** Squirrel command: readAP(address)
    returns a table:
      .status - 1 for OK
      .data   - the data read
*/
SQInteger readAP(HSQUIRRELVM v)
{
    SQInteger nargs = sq_gettop(v);  // get number of arguments

    if (g_interface == 0)
    {
        printf("Error: can't connect - open an interface first!");
        return 0;
    }

    if (nargs != 2)
    {
        printf("Incorrect number of arguments\n");
        return 0;   // no arguments returned;
    }

    SQInteger address;
    if (!SQ_SUCCEEDED(sq_getinteger(v, -1, &address)))
    {
        printf("Second argument should be an integer");
        return 0;
    }

    uint32_t my_data;
    HWResult result = g_interface->readAP(address, my_data);

    // create a table as a return argument
    sq_newtable(v);

    sq_pushstring(v, _SC("status"), -1);
    sq_pushinteger(v, result);
    sq_newslot(v, -3, SQFalse);

    sq_pushstring(v, _SC("data"), -1);
    sq_pushinteger(v, my_data);
    sq_newslot(v, -3, SQFalse);

    return 1;   //return one item (table)
}

/** Squirrel command: writeAP(address)
    returns a table:
      .status - 1 for OK
*/
SQInteger writeAP(HSQUIRRELVM v)
{
    SQInteger nargs = sq_gettop(v);  // get number of arguments

    if (g_interface == 0)
    {
        printf("Error: can't connect - open an interface first!");
        return 0;
    }

    if (nargs != 3)
    {
        printf("Incorrect number of arguments\n");
        return 0;   // no arguments returned;
    }

    SQInteger address, data;
    if (!SQ_SUCCEEDED(sq_getinteger(v, -1, &data)))
    {
        printf("Second argument should be an integer");
        return 0;
    }
    if (!SQ_SUCCEEDED(sq_getinteger(v, -2, &address)))
    {
        printf("Second argument should be an integer");
        return 0;
    }

    HWResult result = g_interface->writeAP(address, data);

    // create a table as a return argument
    sq_newtable(v);

    sq_pushstring(v, _SC("status"), -1);
    sq_pushinteger(v, result);
    sq_newslot(v, -3, SQFalse);

    return 1;   //return one item (table)
}



/** Squirrel command: readMemory(address)
    returns a table:
      .status - 1 for OK
      .data   - the data read
*/
SQInteger readMemory(HSQUIRRELVM v)
{
    SQInteger nargs = sq_gettop(v);  // get number of arguments

    if (g_interface == 0)
    {
        printf("Error: can't connect - open an interface first!");
        return 0;
    }

    if (nargs != 2)
        return 0;   // no arguments returned;

    SQInteger address;
    if (!SQ_SUCCEEDED(sq_getinteger(v, -1, &address)))
    {
        printf("Second argument should be an integer");
        return 0;
    }

    uint32_t my_data;
    HWResult result = g_interface->readMemory(address, my_data);

    // create a table as a return argument
    sq_newtable(v);

    sq_pushstring(v, _SC("status"), -1);
    sq_pushinteger(v, result);
    sq_newslot(v, -3, SQFalse);

    sq_pushstring(v, _SC("data"), -1);
    sq_pushinteger(v, my_data);
    sq_newslot(v, -3, SQFalse);

    return 1;   //return one item (table)
}

/** Squirrel command: writeMemory(address)
    returns a table:
      .status - 1 for OK
*/
SQInteger writeMemory(HSQUIRRELVM v)
{
    SQInteger nargs = sq_gettop(v);  // get number of arguments

    if (g_interface == 0)
    {
        printf("Error: can't connect - open an interface first!");
        return 0;
    }

    if (nargs != 3)
        return 0;   // no arguments returned;

    SQInteger address, data;
    if (!SQ_SUCCEEDED(sq_getinteger(v, -1, &data)))
    {
        printf("Second argument should be an integer");
        return 0;
    }
    if (!SQ_SUCCEEDED(sq_getinteger(v, -2, &address)))
    {
        printf("First argument should be an integer");
        return 0;
    }

    HWResult result = g_interface->writeMemory(address, data);

    // create a table as a return argument
    sq_newtable(v);

    sq_pushstring(v, _SC("status"), -1);
    sq_pushinteger(v, result);
    sq_newslot(v, -3, SQFalse);

    return 1;   //return one item (table)
}

#endif




bool doScript(HSQUIRRELVM v, const char *fileName)
{
    //sq_pushroottable(v);

    if (SQ_SUCCEEDED(sqstd_dofile(v, fileName, SQFalse, SQTrue)))
    {
        return true;
    }
    return false;
}
