/*

  Swagger - A tool for programming ARM processors using the SWD protocol

  Niels A. Moseley

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
    m_debug = true;
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

HWResult HardwareInterface::connect(uint32_t &idcode)
{
    HardwareTXCommand txcmd;
    txcmd.cmdType = TXCMD_TYPE_CONNECT;
    txcmd.data    = 0x01010101;
    txcmd.APnDP   = 0x01;     // Not used but more efficient after COBS encoding if != 0
    txcmd.address = 0x01;     // Not used but more efficient after COBS encoding if != 0

    idcode = 0;

    if (!isOpen())
    {
        m_lastError = "COM port not open";
        return INT_ERROR;
    }

    // send the packet
    std::vector<uint8_t> buffer;
    buffer.insert(buffer.begin(), (const uint8_t*)&txcmd, ((const uint8_t*)&txcmd) + sizeof(txcmd));
    if (!writePacket(buffer))
    {
        printf("%s\n", getLastError().c_str());
        return INT_ERROR;
    }

    // receive status packet
    if (!readPacket(buffer))
    {
        printf("%s\n", getLastError().c_str());
        return INT_ERROR;
    }
    else
    {
        if (buffer.size() != sizeof(HardwareRXCommand))
        {
            m_lastError = "Received packet is larger than expected";
            return INT_ERROR;
        }
        HardwareRXCommand *rxcmd = (HardwareRXCommand *)&buffer[0];
        if (rxcmd->status != RXCMD_STATUS_OK)
        {
            m_lastError = "Protocol error";
            return INT_ERROR;
        }

        idcode = rxcmd->data;
        return rxcmd->swdcode;
    }
    return INT_ERROR;
}

bool HardwareInterface::setTargetReset(bool reset)
{
    HardwareTXCommand txcmd;
    txcmd.cmdType = TXCMD_TYPE_RESETPIN;
    txcmd.data    = reset ? 1:0;
    txcmd.APnDP   = 0x01;     // Not used but more efficient after COBS encoding if != 0
    txcmd.address = 0x01;     // Not used but more efficient after COBS encoding if != 0

    if (!isOpen())
    {
        m_lastError = "COM port not open";
        return false;
    }

    // send the packet
    std::vector<uint8_t> buffer;
    buffer.insert(buffer.begin(), (const uint8_t*)&txcmd, ((const uint8_t*)&txcmd) + sizeof(txcmd));
    if (!writePacket(buffer))
    {
        printf("%s\n", getLastError().c_str());
        return false;
    }

    // receive status packet
    if (!readPacket(buffer))
    {
        printf("%s\n", getLastError().c_str());
        return false;
    }
    else
    {
        if (buffer.size() != sizeof(HardwareRXCommand))
        {
            m_lastError = "Received packet is larger than expected";
            return false;
        }
        HardwareRXCommand *rxcmd = (HardwareRXCommand *)&buffer[0];
        if (rxcmd->status != RXCMD_STATUS_OK)
        {
            m_lastError = "Protocol error";
            return false;
        }

        printf("%d IDCODE = %08X\n", rxcmd->swdcode, rxcmd->data);
    }
    return true;
}

bool HardwareInterface::writePacket(const std::vector<uint8_t> &data)
{
    if (!isOpen())
    {
        m_lastError = "COM port not open";
        return false;
    }

    // do COBS encoding
    std::vector<uint8_t> cobsbuffer;
    if (COBS::encode(data, cobsbuffer))
    {
        if (m_debug)
        {
            printf("TX ");
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
        printf("RX ");
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

HWResult HardwareInterface::writeRegister(bool APnDP, uint32_t address, uint32_t data)
{
    HardwareTXCommand txcmd;
    txcmd.cmdType = TXCMD_TYPE_WRITEREG;
    txcmd.data    = data;
    txcmd.APnDP   = APnDP ? 1:0;
    txcmd.address = (uint8_t)address;

    // send the packet
    std::vector<uint8_t> buffer;
    buffer.insert(buffer.begin(), (const uint8_t*)&txcmd, ((const uint8_t*)&txcmd) + sizeof(txcmd));

    if (!writePacket(buffer))
    {
        return INT_ERROR;
    }

    // receive status packet
    if (!readPacket(buffer))
    {
        return INT_ERROR;
    }
    else
    {
        if (buffer.size() != sizeof(HardwareRXCommand))
        {
            m_lastError = "Received packet is larger than expected";
            return INT_ERROR;
        }
        HardwareRXCommand *rxcmd = (HardwareRXCommand *)&buffer[0];
        if (rxcmd->status != RXCMD_STATUS_OK)
        {
            m_lastError = "Protocol error";
            return INT_ERROR;
        }
        else
        {
            switch(rxcmd->swdcode)
            {
            case 1:
                return SWD_OK;
            case 2:
                return SWD_WAIT;
            case 4:
                return SWD_FAULT;
            default:
                m_lastError = "Unknown SWD ACK";
                return INT_ERROR;
            }
        }
    }
    // we should never get here..
    return INT_ERROR;
}

HWResult HardwareInterface::readRegister(bool APnDP, uint32_t address, uint32_t &data)
{
    HardwareTXCommand txcmd;
    txcmd.cmdType = TXCMD_TYPE_READREG;
    txcmd.data    = data;
    txcmd.APnDP   = APnDP ? 1:0;
    txcmd.address = (uint8_t)address;

    // send the packet
    std::vector<uint8_t> buffer;
    buffer.insert(buffer.begin(), (const uint8_t*)&txcmd, ((const uint8_t*)&txcmd) + sizeof(txcmd));
    if (!writePacket(buffer))
    {
        printf("%s\n", getLastError().c_str());
        return INT_ERROR;
    }

    // receive status packet
    if (!readPacket(buffer))
    {
        printf("%s\n", getLastError().c_str());
        return INT_ERROR;
    }
    else
    {
        if (buffer.size() != sizeof(HardwareRXCommand))
        {
            m_lastError = "Received packet is larger than expected";
            printf("%s\n", getLastError().c_str());
            return INT_ERROR;
        }
        HardwareRXCommand *rxcmd = (HardwareRXCommand *)&buffer[0];
        if (rxcmd->status != RXCMD_STATUS_OK)
        {
            m_lastError = "Protocol error";
            printf("%s\n", getLastError().c_str());
            return INT_ERROR;
        }
        else
        {
            data = rxcmd->data;            
            switch(rxcmd->swdcode)
            {
            case 1:
                return SWD_OK;
            case 2:
                return SWD_WAIT;
            case 4:
                return SWD_FAULT;
            default:
                m_lastError = "Unknown SWD ACK";
                printf("%s\n", getLastError().c_str());
                return INT_ERROR;
            }
        }
    }
    // we should never get here..
    return INT_ERROR;
}

HWResult HardwareInterface::queryInterfaceName(std::string &name)
{
    HardwareTXCommand txcmd;
    txcmd.cmdType = TXCMD_TYPE_GETPROGID;
    txcmd.data    = 0x01010101;
    txcmd.APnDP   = 1;
    txcmd.address = 1;

    // send the packet
    std::vector<uint8_t> buffer;
    buffer.insert(buffer.begin(), (const uint8_t*)&txcmd, ((const uint8_t*)&txcmd) + sizeof(txcmd));

    if (!writePacket(buffer))
    {
        printf("%s\n",getLastError().c_str());
        return INT_ERROR;
    }

    // receive status packet
    if (!readPacket(buffer))
    {
        printf("%s\n",getLastError().c_str());
        return INT_ERROR;
    }
    else
    {
        // skip the RXCMD structure
        name.clear();
        buffer.erase(buffer.begin(), buffer.begin()+sizeof(HardwareRXCommand));
        name.insert(name.begin(), buffer.begin(), buffer.end());
        return SWD_OK;
    }
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

void HardwareInterface::generateOKPacket()
{
    HardwareRXCommand rxcmd;
    rxcmd.data      = 0x01010101;
    rxcmd.status    = RXCMD_STATUS_OK;    // ok
    rxcmd.swdcode   = 1;    // SWDOK

    std::vector<uint8_t> buffer, cobs;
    buffer.insert(buffer.begin(), (const uint8_t*)&rxcmd, ((const uint8_t*)&rxcmd) + sizeof(rxcmd));
    if (COBS::encode(buffer, cobs))
    {
        printf("const uint8_t OKPacket[] = {\n");
        for(size_t i=0; i<cobs.size(); i++)
        {
            printf("  %02X,\n", cobs[i]);
        }
    }
}

void HardwareInterface::generateNamePacket(const std::string &deviceName)
{
    HardwareRXCommand rxcmd;
    rxcmd.data      = 0x01010101;
    rxcmd.status    = RXCMD_STATUS_OK;    // ok
    rxcmd.swdcode   = 1;    // SWDOK

    std::vector<uint8_t> buffer, cobs;
    buffer.insert(buffer.begin(), (const uint8_t*)&rxcmd, ((const uint8_t*)&rxcmd) + sizeof(rxcmd));
    buffer.insert(buffer.end(), deviceName.begin(), deviceName.end());
    buffer.push_back(0); // add NULL terminator to string!

    if (COBS::encode(buffer, cobs))
    {
        printf("const uint8_t deviceNamePacket[] = {\n");
        for(size_t i=0; i<cobs.size(); i++)
        {
            printf(""
                   "0x%02X,", cobs[i]);
        }
    }
}
