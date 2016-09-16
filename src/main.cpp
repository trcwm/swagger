/*

  Swagger - A tool for programming ARM processors using the SWD protocol

  Niels A. Moseley

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>


#include <QString>
#include <QCoreApplication>
#include <QCommandLineParser>

#include "cobs.h"
#include "hardwareinterface.h"
#include "squirrel_funcs.h"

#define VERSION "0.1"

// dirty hack ..
HardwareInterface* g_interface = NULL;


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

void dumpSerialPortNames()
{
    const auto infos = QSerialPortInfo::availablePorts();
    printf("Available ports:\n");
    for (const QSerialPortInfo &info : infos)
    {
        printf("  %s %s\n", info.portName().toStdString().c_str(), info.description().toStdString().c_str());
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication coreApplication(argc, argv);
    QCoreApplication::setApplicationName("Swagger");
    QCoreApplication::setApplicationVersion( VERSION );

    HSQUIRRELVM v;
    SQInteger retval = 0;

    QCommandLineParser parser;
    parser.setApplicationDescription("");
    parser.addHelpOption();

    // Add -f for binary source file
    QCommandLineOption binFile(QStringList() << "f" << "file", "file name of firmware (.bin).", "filename");
    parser.addOption(binFile);

    // Add -p for processor identification
    QCommandLineOption procType(QStringList() << "p" << "proc", "target processor name.", "processor");
    parser.addOption(procType);

    // Add -c for com port name
    QCommandLineOption comPort(QStringList() << "c" << "com", "COM port name.", "comport");
    parser.addOption(comPort);

    // Add -b for baud rate
    QCommandLineOption baudrate(QStringList() << "b" << "baud", "Override the baud rate.", "baudrate", "57600");
    parser.addOption(baudrate);

    // Add -D for disable auto-erase
    QCommandLineOption disableAutoErase(QStringList() << "A" << "disable-erase", "Disable auto-erase.");
    parser.addOption(disableAutoErase);

    // Add -V for disable auto-erase
    QCommandLineOption disableVerify(QStringList() << "V" << "disable-verify", "Disable auto-verify.");
    parser.addOption(disableVerify);

    // Add -v for verbose mode
    QCommandLineOption verboseMode(QStringList() << "v" << "verbose", "Set to verbose mode.");
    parser.addOption(verboseMode);

    // Add -I for verbose mode
    QCommandLineOption interactiveOption(QStringList() << "I" << "interactive", "Interactive mode.");
    parser.addOption(interactiveOption);

    parser.process(coreApplication);

    if (!parser.isSet(binFile))
    {
        fprintf(stderr, "Error: please specify the file to flash.\n\n");
        parser.showHelp(1);
    }

    if (!parser.isSet(comPort))
    {
        fprintf(stderr, "Error: please specify the COM port to use.\n\n");
        parser.showHelp(1);
    }

    /*
    if (!parser.isSet(procType))
    {
        fprintf(stderr, "Error: please specify the processor type.\n\n");
        parser.showHelp(1);
    }
    */

    // try to open the COM port interface
    // and produce an error if we're not able
    // including a list of possible ports

    std::stringstream deviceName;
#ifdef _WIN32
    deviceName << "\\\\.\\COM" << parser.value(comPort).toStdString().c_str();
#else
    deviceName << "/dev/tty." << parser.value(comPort).toStdString().c_str();
#endif

    bool ok = false;
    uint32_t baud = parser.value(baudrate).toInt(&ok);
    if (!ok)
    {
        printf("Baudrate %s is not an integer!", qPrintable(parser.value(baudrate)));
        return 1;
    }

    g_interface = HardwareInterface::open(deviceName.str().c_str(), baud);
    if (g_interface == 0)
    {
        fprintf(stderr, "Error: could not open communication port %s!\n\n", deviceName.str().c_str());
        dumpSerialPortNames();
        return 1;
    }

#if 0
    printf("Swagger version " VERSION " "__DATE__"\n");
    printf("Using %s (%d bits)\n",SQUIRREL_VERSION,((int)(sizeof(SQInteger)*8)));
    printf("\nuse:\n");
    printf("  quit() to exit.\n");
    printf("  openInterface(<integer> | <string>)\n");
    printf("  closeInterface()\n");
    printf("  showSerial() to enumerate serial interfaces.\n");
    printf("  table{.status,.idcode} = connect() to connect to the target.\n");
    printf("  table{.status,data} = readAP/readDP/readMemory (address)\n");
    printf("  table{.status} = writeAP/writeDP/writeMemory (address, data)\n");
    printf("\n");
#endif

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
    register_global_func(v, queueUInt8, _SC("queueUInt8"));
    register_global_func(v, queueUInt32, _SC("queueUInt32"));
    register_global_func(v, executeCmdQueue, _SC("executeCmdQueue"));
    register_global_func(v, popUInt8, _SC("popUInt8"));
    register_global_func(v, popUInt32, _SC("popUInt32"));
    register_global_func(v, dumpCmdQueue, _SC("dumpCmdQueue"));
    register_global_func(v, clearCmdQueue, _SC("clearCmdQueue"));
    register_global_func(v, dumpResultQueue, _SC("dumpResultQueue"));
    register_global_func(v, printLastPacketError, _SC("printLastPacketError"));
    register_global_func(v, sleep, _SC("sleep"));

    // pass on command line parameters to squirrel environment
    createStringVariable(v,"procType",qPrintable(parser.value(procType)));
    createStringVariable(v,"binFile",qPrintable(parser.value(binFile)));
    createBooleanVariable(v, "verbose", parser.isSet(verboseMode));
    createBooleanVariable(v, "interactive", parser.isSet(interactiveOption));

    QString scriptpath = QCoreApplication::applicationDirPath();
    scriptpath.append("\\..\\targets\\");
    createStringVariable(v,"scriptDir",qPrintable(scriptpath));

    // load all the targets
    sq_setcompilererrorhandler(v, compile_error_handler);

    scriptpath.append("init.nut");
    if (!doScript(v, scriptpath.toStdString().c_str()))
    {
        printf("Cannot execute %s!\n", qPrintable(scriptpath));
        return 1;
    }

    if (parser.isSet(interactiveOption))
            Interactive(v);

    sq_close(v);

    if (g_interface != 0)
        g_interface->close();

    return 0;
}
