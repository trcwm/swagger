/*

  Swagger - A tool for programming ARM processors using the SWD protocol

  Niels A. Moseley

*/


#ifndef HardwareInterface_h
#define HardwareInterface_h

#include <stdint.h>
#include <QtSerialPort>
#include <QtSerialPort/QSerialPortInfo>

enum SWDAckResult
{
    SWD_OK = 1,
    SWD_WAIT = 2,
    SWD_FAULT = 3,
    INT_ERROR = 4   	// internal error
};

#pragma pack(push,1)

/** format of packets sent to the hardware interface
    Note that this data will be encapsulated in a
    COBS packet. */
struct HardwareTXCommand
{
    uint8_t     cmdType;    // 0 = reset condition, 1 = read register, 2 = write register
    uint8_t     address;   // bits [2:3] of register address
    uint8_t     APnDP;      // Address or data register access (1 = address)
    uint32_t    data;       // reset line state or write register data
};

/** format of packets received from the hardware interface
    Note that this data will be encapsulated in a
    COBS packet. */
struct HardwareRXCommand
{
    uint8_t     status;     // status of communication 0 = OK
    uint8_t     swdcode;    // swd return code
    uint32_t    data;       // read register data
};

#pragma pack(pop)

class HardwareInterface
{
    const uint8_t cmdReset  = 0;
    const uint8_t cmdRead   = 1;
    const uint8_t cmdWrite  = 2;

public:
    virtual ~HardwareInterface();

    /** open a COM port to the hardware interface */
    static HardwareInterface* open(const char *comport, uint32_t baudrate);

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

    /** write register */
    virtual SWDAckResult writeRegister(bool APnDP, uint32_t address, uint32_t data);

    /** read register */
    virtual SWDAckResult readRegister(bool APnDP, uint32_t address, uint32_t &data);

    /** get the last error in human readable form */
    std::string getLastError() const
    {
        return m_lastError;
    }

    /** for testing purposes */
    static void generateOKPacket();


protected:
    bool writePacket(const std::vector<uint8_t> &data);
    bool readPacket(std::vector<uint8_t> &data);

    HardwareInterface(const char *comport, uint32_t baudrate);

    uint32_t    m_timeout;
    QSerialPort m_port;
    std::string m_lastError;
};

#endif
