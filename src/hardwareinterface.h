/*

  Swagger - A tool for programming ARM processors using the SWD protocol

  Niels A. Moseley

*/


#ifndef HardwareInterface_h
#define HardwareInterface_h

#include <stdint.h>
#include <QtSerialPort>
#include <QtSerialPort/QSerialPortInfo>

#include "protocol.h"

typedef uint32_t HWResult;

#define SWD_OK 1
#define SWD_WAIT 2
#define SWD_FAULT 4
#define INT_ERROR 99

class HardwareInterface
{
public:
    virtual ~HardwareInterface();

    /** open a COM port to the hardware interface */
    static HardwareInterface* open(const char *comport, uint32_t baudrate);

    /** print the available COM ports to the console */
    static void printInterfaces()
    {
        foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
        {
            std::string name = info.portName().toStdString();
            std::string description = info.description().toStdString();
            printf("Port name: %s\n", name.c_str());
            printf("Port name: %s\n", description.c_str());
        }
    }

    /** check to see if this interface is still open */
    bool isOpen() const
    {
        return m_port.isOpen();
    }

    /** close the serial port */
    void close();

    /** set the reset pin of the target */
    virtual bool setTargetReset(bool reset);

    /** connect to the target */
    virtual HWResult connect(uint32_t &idcode);

    /** write register */
    virtual HWResult writeRegister(bool APnDP, uint32_t address, uint32_t data);

    /** read register */
    virtual HWResult readRegister(bool APnDP, uint32_t address, uint32_t &data);

    /** read programmer ID string */
    virtual HWResult queryInterfaceName(std::string &name);

    /** get the last error in human readable form */
    std::string getLastError() const
    {
        return m_lastError;
    }

    /** for testing/development purposes */
    static void generateOKPacket();

    /** for testing/developement purposes */
    static void generateNamePacket(const std::string &deviceName);

protected:
    bool writePacket(const std::vector<uint8_t> &data);
    bool readPacket(std::vector<uint8_t> &data);

    void printPacket(const std::vector<uint8_t> &data);

    HardwareInterface(const char *comport, uint32_t baudrate);

    bool        m_debug;
    uint32_t    m_timeout;
    QSerialPort m_port;
    std::string m_lastError;
};

#endif
