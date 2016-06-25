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

HWResult HardwareInterface::connect(uint32_t &idcode)
{
    HardwareTXCommand txcmd;
    txcmd.cmdType = TXCMD_TYPE_CONNECT;
    txcmd.data    = 0x01010101;
    txcmd.address = 0x01;     // Not used but more efficient after COBS encoding if != 0

    idcode = 0;

    if (!isOpen())
    {
        m_lastError = "COM port not open";
        return RXCMD_PROTOCOL_ERROR;
    }

    // send the packet
    if (!writePacket(&txcmd, sizeof(txcmd)))
    {
        printf("%s\n", getLastError().c_str());
        return RXCMD_PROTOCOL_ERROR;
    }

    // receive status packet
    std::vector<uint8_t> buffer;
    if (!readPacket(buffer))
    {
        printf("%s\n", getLastError().c_str());
        return RXCMD_PROTOCOL_ERROR;
    }
    else
    {
        if (buffer.size() != sizeof(HardwareRXCommand))
        {
            m_lastError = "Received packet is larger than expected";
            return RXCMD_PROTOCOL_ERROR;
        }
        HardwareRXCommand *rxcmd = (HardwareRXCommand *)&buffer[0];

        idcode = rxcmd->data;
        return rxcmd->status;
    }
    return RXCMD_PROTOCOL_ERROR;
}

bool HardwareInterface::setTargetReset(bool reset)
{
    HardwareTXCommand txcmd;
    txcmd.cmdType = TXCMD_TYPE_RESETPIN;
    txcmd.data    = reset ? 1:0;
    txcmd.address = 0x01;     // Not used but more efficient after COBS encoding if != 0

    if (!isOpen())
    {
        m_lastError = "COM port not open";
        return false;
    }

    // send the packet
    if (!writePacket(&txcmd, sizeof(txcmd)))
    {
        printf("%s\n", getLastError().c_str());
        return false;
    }

    // receive status packet
    std::vector<uint8_t> buffer;
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

        printf("%d IDCODE = %08X\n", rxcmd->status, rxcmd->data);
    }
    return true;
}

bool HardwareInterface::writePacket(const void* data, size_t bytes)
{
    if (!isOpen())
    {
        m_lastError = "COM port not open";
        return false;
    }

    // make std::vector buffer
    std::vector<uint8_t> databuf;
    databuf.insert(databuf.begin(), (const uint8_t*)data, ((const uint8_t*)data) + bytes);

    // do COBS encoding
    std::vector<uint8_t> cobsbuffer;
    if (COBS::encode(databuf, cobsbuffer))
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

HWResult HardwareInterface::writeDP(uint32_t address, uint32_t data)
{
    HardwareTXCommand txcmd;
    txcmd.cmdType = TXCMD_TYPE_WRITEDP;
    txcmd.data    = data;
    txcmd.address = address;

    // send the packet
    if (!writePacket(&txcmd, sizeof(txcmd)))
    {
        printf("writeDP: %s\n", getLastError().c_str());
        return RXCMD_PROTOCOL_ERROR;
    }

    // receive status packet
    std::vector<uint8_t> buffer;
    if (!readPacket(buffer))
    {
        printf("%s\n", getLastError().c_str());
        return RXCMD_PROTOCOL_ERROR;
    }
    else
    {
        if (buffer.size() != sizeof(HardwareRXCommand))
        {
            m_lastError = "writeDP: Received packet is larger than expected";
            return RXCMD_PROTOCOL_ERROR;
        }
        HardwareRXCommand *rxcmd = (HardwareRXCommand *)&buffer[0];
        if (rxcmd->status != RXCMD_STATUS_OK)
        {
            m_lastError = "writeDP: Protocol error";
            return RXCMD_PROTOCOL_ERROR;
        }
        return rxcmd->status;
    }

    // we should never get here..
    return RXCMD_PROTOCOL_ERROR;
}

HWResult HardwareInterface::readDP(uint32_t address, uint32_t &data)
{
    HardwareTXCommand txcmd;
    txcmd.cmdType = TXCMD_TYPE_READDP;
    txcmd.data    = data;
    txcmd.address = address;

    // send the packet
    if (!writePacket(&txcmd, sizeof(txcmd)))
    {
        printf("%s\n", getLastError().c_str());
        return RXCMD_PROTOCOL_ERROR;
    }

    // receive status packet
    std::vector<uint8_t> buffer;
    if (!readPacket(buffer))
    {
        printf("%s\n", getLastError().c_str());
        return RXCMD_PROTOCOL_ERROR;
    }
    else
    {
        if (buffer.size() != sizeof(HardwareRXCommand))
        {
            m_lastError = "readDP: Received packet is larger than expected";
            printf("%s\n", getLastError().c_str());
            return RXCMD_PROTOCOL_ERROR;
        }
        HardwareRXCommand *rxcmd = (HardwareRXCommand *)&buffer[0];
        if (rxcmd->status != RXCMD_STATUS_OK)
        {
            m_lastError = "readDP: Protocol error";
            printf("%s %d\n", getLastError().c_str(), rxcmd->status);
            return RXCMD_PROTOCOL_ERROR;
        }
        else
        {
            data = rxcmd->data;
            return rxcmd->status;
        }
    }
    // we should never get here..
    return RXCMD_PROTOCOL_ERROR;
}

HWResult HardwareInterface::writeAP(uint32_t address, uint32_t data)
{
    HardwareTXCommand txcmd;
    txcmd.cmdType = TXCMD_TYPE_WRITEAP;
    txcmd.data    = data;
    txcmd.address = address;

    // send the packet
    if (!writePacket(&txcmd, sizeof(txcmd)))
    {
        printf("%s\n", getLastError().c_str());
        return RXCMD_PROTOCOL_ERROR;
    }

    // receive status packet
    std::vector<uint8_t> buffer;
    if (!readPacket(buffer))
    {
        printf("%s\n", getLastError().c_str());
        return RXCMD_PROTOCOL_ERROR;
    }
    else
    {
        if (buffer.size() != sizeof(HardwareRXCommand))
        {
            m_lastError = "writeAP: Received packet is larger than expected";
            return RXCMD_PROTOCOL_ERROR;
        }
        HardwareRXCommand *rxcmd = (HardwareRXCommand *)&buffer[0];
        if (rxcmd->status != RXCMD_STATUS_OK)
        {
            m_lastError = "writeAP: Protocol error";
            return RXCMD_PROTOCOL_ERROR;
        }
        return rxcmd->status;
    }

    // we should never get here..
    return RXCMD_PROTOCOL_ERROR;
}

HWResult HardwareInterface::readAP(uint32_t address, uint32_t &data)
{
    HardwareTXCommand txcmd;
    txcmd.cmdType = TXCMD_TYPE_READAP;
    txcmd.data    = data;
    txcmd.address = address;

    // send the packet
    if (!writePacket(&txcmd, sizeof(txcmd)))
    {
        printf("%s\n", getLastError().c_str());
        return RXCMD_PROTOCOL_ERROR;
    }

    // receive status packet
    std::vector<uint8_t> buffer;
    if (!readPacket(buffer))
    {
        printf("%s\n", getLastError().c_str());
        return RXCMD_PROTOCOL_ERROR;
    }
    else
    {
        if (buffer.size() != sizeof(HardwareRXCommand))
        {
            m_lastError = "readAP: Received packet is larger than expected";
            printf("%s\n", getLastError().c_str());
            return RXCMD_PROTOCOL_ERROR;
        }
        HardwareRXCommand *rxcmd = (HardwareRXCommand *)&buffer[0];
        if (rxcmd->status != RXCMD_STATUS_OK)
        {
            m_lastError = "readAP: Protocol error";
            printf("%s\n", getLastError().c_str());
            return RXCMD_PROTOCOL_ERROR;
        }
        else
        {
            data = rxcmd->data;
            return rxcmd->status;
        }
    }
    // we should never get here..
    return RXCMD_PROTOCOL_ERROR;
}

HWResult HardwareInterface::writeMemory(uint32_t address, uint32_t data)
{
    HardwareTXCommand txcmd;
    txcmd.cmdType = TXCMD_TYPE_WRITEMEM;
    txcmd.data    = data;
    txcmd.address = address;

    // send the packet
    if (!writePacket(&txcmd, sizeof(txcmd)))
    {
        printf("writeMemory: %08X %s\n", address, getLastError().c_str());
        return RXCMD_PROTOCOL_ERROR;
    }

    // receive status packet
    std::vector<uint8_t> buffer;
    if (!readPacket(buffer))
    {
        printf("writeMemory: %08X %s\n", address, getLastError().c_str());
        return RXCMD_PROTOCOL_ERROR;
    }
    else
    {
        if (buffer.size() != sizeof(HardwareRXCommand))
        {
            m_lastError = "Received packet is larger than expected";
            printf("writeMemory: %08X %s\n", address, getLastError().c_str());
            return RXCMD_PROTOCOL_ERROR;
        }
        HardwareRXCommand *rxcmd = (HardwareRXCommand *)&buffer[0];
        if (rxcmd->status != RXCMD_STATUS_OK)
        {
            m_lastError = "Protocol error";
            printf("writeMemory: %08X %s\n", address, getLastError().c_str());
            return RXCMD_PROTOCOL_ERROR;
        }
        return rxcmd->status;
    }

    // we should never get here..
    return RXCMD_PROTOCOL_ERROR;
}

HWResult HardwareInterface::readMemory(uint32_t address, uint32_t &data)
{
    HardwareTXCommand txcmd;
    txcmd.cmdType = TXCMD_TYPE_READMEM;
    txcmd.data    = data;
    txcmd.address = address;

    // send the packet
    if (!writePacket(&txcmd, sizeof(txcmd)))
    {
        printf("%s\n", getLastError().c_str());
        return RXCMD_PROTOCOL_ERROR;
    }

    // receive status packet
    std::vector<uint8_t> buffer;
    if (!readPacket(buffer))
    {
        printf("%s\n", getLastError().c_str());
        return RXCMD_PROTOCOL_ERROR;
    }
    else
    {
        if (buffer.size() != sizeof(HardwareRXCommand))
        {
            m_lastError = "Received packet is larger than expected";
            printf("readMemory: %08X %s\n", address, getLastError().c_str());
            return RXCMD_PROTOCOL_ERROR;
        }
        HardwareRXCommand *rxcmd = (HardwareRXCommand *)&buffer[0];
        if (rxcmd->status != RXCMD_STATUS_OK)
        {
            m_lastError = "Protocol error";
            printf("readMemory: %08X %s\n", address, getLastError().c_str());
            return RXCMD_PROTOCOL_ERROR;
        }
        else
        {
            data = rxcmd->data;
            return rxcmd->status;
        }
    }
    // we should never get here..
    return RXCMD_PROTOCOL_ERROR;
}

HWResult HardwareInterface::queryInterfaceName(std::string &name)
{
    HardwareTXCommand txcmd;
    txcmd.cmdType = TXCMD_TYPE_GETPROGID;
    txcmd.data    = 0x01010101;
    txcmd.address = 1;

    // send the packet
    if (!writePacket(&txcmd, sizeof(txcmd)))
    {
        printf("%s\n",getLastError().c_str());
        return RXCMD_PROTOCOL_ERROR;
    }

    // receive status packet
    std::vector<uint8_t> buffer;
    if (!readPacket(buffer))
    {
        printf("%s\n",getLastError().c_str());
        return RXCMD_PROTOCOL_ERROR;
    }
    else
    {
        // skip the RXCMD structure
        name.clear();
        buffer.erase(buffer.begin(), buffer.begin()+sizeof(HardwareRXCommand));
        name.insert(name.begin(), buffer.begin(), buffer.end());
        return RXCMD_STATUS_OK;
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

#if 0
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
#endif
