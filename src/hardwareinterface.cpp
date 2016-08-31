/*

  Swagger - A tool for programming ARM processors using the SWD protocol

  Niels A. Moseley (c) Moseley Instruments 2016

*/

#include <stdexcept>
#include "hardwareinterface.h"
#include "cobs.h"

HardwareInterface* HardwareInterface::open(const char *comport, uint32_t baudrate)
{    
    try
    {
        HardwareInterface *interface = new HardwareInterface(comport, baudrate);
        return interface;
    }
    catch(std::runtime_error &e)
    {
        //TODO: fix error handling
        printf("Error: %s\n", e.what());
        return NULL;
    }
}

HardwareInterface::~HardwareInterface()
{
    if (isOpen())
    {
        close();
    }
}

HardwareInterface::HardwareInterface(const char *comport, uint32_t baudrate) : m_timeout(1000)
{
    m_debug = false;
    switch(baudrate)
    {
    case 1200:
        m_port.setBaudRate(QSerialPort::Baud1200);
        break;
    case 2400:
        m_port.setBaudRate(QSerialPort::Baud2400);
        break;
    case 9600:
        m_port.setBaudRate(QSerialPort::Baud9600);
        break;
    case 19200:
        m_port.setBaudRate(QSerialPort::Baud19200);
        break;
    case 57600:
        m_port.setBaudRate(QSerialPort::Baud57600);
        break;
    case 115200:
        m_port.setBaudRate(QSerialPort::Baud115200);
    break;

    default:
        throw std::runtime_error("Unsupported baud rate");
    }

    m_port.setPortName(comport);
    m_port.setDataBits(QSerialPort::Data8);
    m_port.setParity(QSerialPort::NoParity);
    m_port.setStopBits(QSerialPort::OneStop);
    m_port.setFlowControl(QSerialPort::NoFlowControl);

    if (!m_port.open(QIODevice::ReadWrite))
    {
        throw std::runtime_error(m_port.errorString().toStdString());
    }
}

void HardwareInterface::close()
{
    if (m_port.isOpen())
    {
        m_port.close();
    }
}

bool HardwareInterface::writePacket(const std::vector<uint8_t> &data)
{
    if (!isOpen())
    {
        m_lastError = "COM port not open";
        return false;
    }

    if (m_debug)
    {
        printf("TX ");
        printPacket(data);
    }

    // do COBS encoding
    std::vector<uint8_t> cobsbuffer;
    if (COBS::encode(data, cobsbuffer))
    {
        if (m_debug)
        {
            printf("TX (COBS) ");
            printPacket(cobsbuffer);
        }
        if (m_port.write((const char*)&cobsbuffer[0], cobsbuffer.size()) != cobsbuffer.size())
        {
            // unexpected fragmentation .. :-0
            m_lastError = "Unexpected fragmentation";
            return false;
        }
        if (!m_port.waitForBytesWritten(m_timeout))
        {
            m_lastError = "COM port timed out on write";
            return false;
        }
    }
    else
    {
        m_lastError = "Error COBS encoding";
        return false;
    }
    return true;
}

bool HardwareInterface::readPacket(std::vector<uint8_t> &data)
{
    std::vector<uint8_t> rxbuffer;
    if (!isOpen())
    {
        m_lastError = "COM port not open";
        return false;
    }

    // read data from the serial port, until we get a 0 byte
    // as this signals the end of the packet
    uint8_t c;
    do
    {
        if (!m_port.bytesAvailable())
        {
            if (!m_port.waitForReadyRead(m_timeout))
            {
                m_lastError = "COM port timed out on read";
                return false;
            }
        }

        m_port.read((char*)&c,1);
        rxbuffer.push_back(c);

    } while(c != 0);

    if (m_debug)
    {
        printf("RX (COBS) ");
        printPacket(rxbuffer);
    }

    // remove trailing zero
    rxbuffer.pop_back();

    // do COBS decoding
    if (COBS::decode(rxbuffer, data))
    {
        if (data.size() > 0)
        {
            if (m_debug)
            {
                printf("RX DECODED: ");
                printPacket(data);
            }
            data.pop_back();    // remove trailing NULL byte
        }
        else
        {
            m_lastError = "Error removing zero byte at end of packet";
            return false;
        }
    }
    else
    {
        m_lastError = "Error COBS encoding";
        return false;
    }
    return true;
}

void HardwareInterface::printPacket(const std::vector<uint8_t> &data)
{
    size_t N = data.size();

    for(size_t i=0; i<N; i++)
    {
        printf("%02X ", data[i]);
    }
    printf("\n");
}
