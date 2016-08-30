/*

  Swagger - A tool for programming ARM processors using the SWD protocol

  Niels A. Moseley (c) Moseley Instruments 2016

*/


#ifndef HardwareInterface_h
#define HardwareInterface_h

#include <stdint.h>
#include <QtSerialPort>
#include <QtSerialPort/QSerialPortInfo>

#include "protocol.h"

typedef uint32_t HWResult;



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


    /** get the last error in human readable form */
    std::string getLastError() const
    {
        return m_lastError;
    }

    /** write packet to the hardware interface */
    bool writePacket(const std::vector<uint8_t> &data);

    /** read packet from the hardware interface */
    bool readPacket(std::vector<uint8_t> &data);

protected:
    void printPacket(const std::vector<uint8_t> &data);

    HardwareInterface(const char *comport, uint32_t baudrate);

    bool        m_debug;
    uint32_t    m_timeout;
    QSerialPort m_port;
    std::string m_lastError;
};

#endif
