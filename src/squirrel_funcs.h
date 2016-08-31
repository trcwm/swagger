/*

  Swagger - A tool for programming ARM processors using the SWD protocol

  Niels A. Moseley (c) Moseley Instruments 2016

  Squirrel extensions

  note: http://wiki.squirrel-lang.org/default.aspx/SquirrelWiki/Embedding%20Getting%20Started.html
        http://wiki.squirrel-lang.org/default.aspx/SquirrelWiki/SquirrelCallToCpp.html

*/

#ifndef squirrel_funcs_h
#define squirrel_funcs_h

#include <squirrel.h>
#include <sqstdblob.h>
#include <sqstdsystem.h>
#include <sqstdio.h>
#include <sqstdmath.h>
#include <sqstdstring.h>
#include <sqstdaux.h>

#ifdef SQUNICODE
#define scfprintf fwprintf
#define scvprintf vfwprintf
#else
#define scfprintf fprintf
#define scvprintf vfprintf
#endif

#include <stdint.h>
#include <iostream>
#include <vector>
#include <stdarg.h>

void printfunc(HSQUIRRELVM SQ_UNUSED_ARG(v),const SQChar *s,...);

void errorfunc(HSQUIRRELVM SQ_UNUSED_ARG(v),const SQChar *s,...);

void register_global_func(HSQUIRRELVM v, SQFUNCTION f, const char *fname);

void register_global_func(HSQUIRRELVM v, SQFUNCTION f, const char *fname, SQUserPointer *userPtr);

void compile_error_handler(HSQUIRRELVM v, const SQChar* desc, const SQChar* source, SQInteger line, SQInteger column);

// *****************************************
// ** CUSTOM SQUIRREL FUNCTIONS
// *****************************************

/** quit the program */
SQInteger quit(HSQUIRRELVM v);

/** Squirrel command: queue uint8_t in the command queue*/
SQInteger queueUInt8(HSQUIRRELVM v);

/** Squirrel command: queue uint32_t int the command queue*/
SQInteger queueUInt32(HSQUIRRELVM v);

/** Squirrel command: execute command queue */
SQInteger executeCmdQueue(HSQUIRRELVM v);

/** Squirrel command: pop uint8_t from result queue */
SQInteger popUInt8(HSQUIRRELVM v);

/** Squirrel command: pop uint32_t from result queue */
SQInteger popUInt32(HSQUIRRELVM v);

/** Squirrel command: dump the current command queue */
SQInteger dumpCmdQueue(HSQUIRRELVM v);

/** Squirrel command: clear the command queue */
SQInteger clearCmdQueue(HSQUIRRELVM v);

/** Squirrel command: dump the current result queue */
SQInteger dumpResultQueue(HSQUIRRELVM v);

/** Squirrel command: print the last logged error in packet system */
SQInteger printLastPacketError(HSQUIRRELVM v);

/** Execute a squirrel script */
bool doScript(HSQUIRRELVM v, const char *fileName);

#endif
