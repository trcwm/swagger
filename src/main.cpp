/*

  Swagger - A tool for programming ARM processors using the SWD protocol

  Niels A. Moseley

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "cobs.h"

#include <QtSerialPort>
#include <QtSerialPort/QSerialPortInfo>

#include <squirrel.h>
#include <sqstdblob.h>
#include <sqstdsystem.h>
#include <sqstdio.h>
#include <sqstdmath.h>
#include <sqstdstring.h>
#include <sqstdaux.h>

// note: https://docs.openttd.org/squirrel_8cpp_source.html

#ifdef SQUNICODE
#define scfprintf fwprintf
#define scvprintf vfwprintf
#else
#define scfprintf fprintf
#define scvprintf vfprintf
#endif

#define VERSION "0.1"

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

SQInteger quit(HSQUIRRELVM v)
{
    int *done;
    sq_getuserpointer(v,-1,(SQUserPointer*)&done);
    *done=1;
    return 0;
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

SQInteger enumerateSerialPorts(HSQUIRRELVM v)
{
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        std::string name = info.portName().toStdString();
        std::string description = info.description().toStdString();
        printf("Port name: %s\n", name.c_str());
        printf("Port name: %s\n", description.c_str());
    }
    return 0;   // no arguments are returned.
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
    HSQUIRRELVM v;
    SQInteger retval = 0;

    COBS::test();

    printf("Swagger version " VERSION " "__DATE__"\n");
    printf("Using %s (%d bits)\n",SQUIRREL_VERSION,((int)(sizeof(SQInteger)*8)));
    printf("\nuse quit() to exit.\n");

    v=sq_open(1024);
    sq_pushroottable(v);

    sq_setprintfunc(v, printfunc, errorfunc);

    sqstd_register_bloblib(v);
    sqstd_register_iolib(v);
    sqstd_register_systemlib(v);
    sqstd_register_mathlib(v);
    sqstd_register_stringlib(v);

    // register our own functions
    register_global_func(v, enumerateSerialPorts, _SC("showSerial"));

    // load all the targets
    sq_setcompilererrorhandler(v, compile_error_handler);
    if (!doScript(v, "targets\\targets.nut"))
    {
        printf("Cannot execute targets.nut script!\n");
    }

    Interactive(v);

    sq_close(v);

    return 0;
}
