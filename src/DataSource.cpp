#include "DataSource.hpp"
#include <SoapySDR/Logger.hpp>

#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>

// ============================================================================
// DataSource (базовый класс)
// ============================================================================

DataSource::DataSource()
    : _ring(1024 * 1024)
{
}

DataSource::~DataSource()
{
    shutdown();
}

bool DataSource::init()
{
    if (!openRaw())
    {
        return false;
    }

    _running = true;
    _thread = std::thread(&DataSource::threadFunc, this);

    return true;
}

void DataSource::shutdown()
{
    if (_running.exchange(false))
    {
        _ring.shutdown();
        if (_thread.joinable())
        {
            _thread.join();
        }
        closeRaw();
    }
}

bool DataSource::readSamples(int16_t *data, size_t num_samples, int timeout_ms)
{
    return _ring.pop(data, num_samples * 2, timeout_ms);
}

void DataSource::threadFunc()
{
    SoapySDR::log(SOAPY_SDR_INFO, "AfedriUSB: DataSource background thread started");

    std::vector<int16_t> packet_buf;
    int consecutive_failures = 0;

    while (_running)
    {
        packet_buf.clear();

        if (readRawPacket(packet_buf))
        {
            if (!packet_buf.empty())
            {
                _ring.push(packet_buf.data(), packet_buf.size());
            }
            consecutive_failures = 0;
        }
        else
        {
            consecutive_failures++;
            if (consecutive_failures % 100 == 0)
            {
                SoapySDR::logf(SOAPY_SDR_WARNING,
                               "AfedriUSB: %d consecutive read failures from data source",
                               consecutive_failures);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }

    SoapySDR::log(SOAPY_SDR_INFO, "AfedriUSB: DataSource background thread stopped");
}

// ============================================================================
// MSDDataSource
// ============================================================================

MSDDataSource::MSDDataSource(const std::string &path)
    : _path(path)
{
}

MSDDataSource::~MSDDataSource()
{
    // shutdown() вызывается базовым деструктором DataSource,
    // который в свою очередь вызовет closeRaw().
}

bool MSDDataSource::openRaw()
{
    _file_handle = ::open(_path.c_str(), O_RDONLY);
    if (_file_handle < 0)
    {
        SoapySDR::logf(SOAPY_SDR_ERROR, "AfedriUSB: Cannot open MSD file: %s (errno=%d)",
                       _path.c_str(), errno);
        return false;
    }

    SoapySDR::logf(SOAPY_SDR_INFO, "AfedriUSB: MSD file opened: %s", _path.c_str());
    return true;
}

void MSDDataSource::closeRaw()
{
    if (_file_handle != -1)
    {
        ::close(_file_handle);
        _file_handle = -1;
    }
}

bool MSDDataSource::readRawPacket(std::vector<int16_t> &out)
{
    if (_file_handle < 0)
    {
        return false;
    }

    std::vector<unsigned char> raw(READ_PACKET_SIZE);

    if (::lseek(_file_handle, 0, SEEK_SET) < 0)
    {
        SoapySDR::logf(SOAPY_SDR_ERROR, "AfedriUSB: lseek failed (errno=%d)", errno);
        return false;
    }

    ssize_t bytes_read = ::read(_file_handle, raw.data(), raw.size());
    if (bytes_read < 0)
    {
        SoapySDR::logf(SOAPY_SDR_ERROR, "AfedriUSB: MSD read error (errno=%d)", errno);
        return false;
    }

    if (bytes_read == 0)
    {
        return false;
    }

    size_t num_int16 = (size_t)bytes_read / sizeof(int16_t);
    num_int16 -= (num_int16 % 2);

    out.resize(num_int16);
    std::memcpy(out.data(), raw.data(), num_int16 * sizeof(int16_t));

    return true;
}

// ============================================================================
// UDPDataSource
// ============================================================================

UDPDataSource::UDPDataSource(int port)
    : _port(port)
{
}

UDPDataSource::~UDPDataSource()
{
    // closeRaw() вызывается через shutdown() базового класса.
}

bool UDPDataSource::openRaw()
{
    _socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (_socket < 0)
    {
        SoapySDR::logf(SOAPY_SDR_ERROR, "AfedriUSB: Cannot create UDP socket (errno=%d)", errno);
        return false;
    }

    int optval = 1;
    if (setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
    {
        SoapySDR::log(SOAPY_SDR_WARNING, "AfedriUSB: setsockopt SO_REUSEADDR failed");
    }

    int recvsize = 1 << 20; // 1MB
    if (setsockopt(_socket, SOL_SOCKET, SO_RCVBUF, (char *)&recvsize, sizeof(recvsize)) < 0)
    {
        SoapySDR::log(SOAPY_SDR_WARNING, "AfedriUSB: setsockopt SO_RCVBUF failed");
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(_port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        SoapySDR::logf(SOAPY_SDR_ERROR, "AfedriUSB: Cannot bind UDP socket to port %d (errno=%d)",
                       _port, errno);
        ::close(_socket);
        _socket = -1;
        return false;
    }

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 200000; // 200ms - чтобы поток мог периодически проверять _running
    if (setsockopt(_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
    {
        SoapySDR::log(SOAPY_SDR_WARNING, "AfedriUSB: setsockopt SO_RCVTIMEO failed");
    }

    SoapySDR::logf(SOAPY_SDR_INFO, "AfedriUSB: UDP socket listening on port %d", _port);
    return true;
}

void UDPDataSource::closeRaw()
{
    if (_socket != -1)
    {
        ::close(_socket);
        _socket = -1;
    }
}

bool UDPDataSource::readRawPacket(std::vector<int16_t> &out)
{
    if (_socket < 0)
    {
        return false;
    }

    unsigned char buffer[PACKET_SIZE];
    ssize_t bytes_read = recvfrom(_socket, buffer, PACKET_SIZE, 0, nullptr, nullptr);

    if (bytes_read < 0)
    {
        if (errno != EAGAIN && errno != EWOULDBLOCK)
        {
            SoapySDR::logf(SOAPY_SDR_DEBUG, "AfedriUSB: UDP recvfrom error (errno=%d)", errno);
        }
        return false;
    }
if ((size_t)bytes_read <= HEADER_SIZE)
    {
        SoapySDR::logf(SOAPY_SDR_WARNING, "AfedriUSB: UDP packet too small: %ld bytes", bytes_read);
        return false;
    }

    // Пропускаем заголовок (первые 4 байта пакета)
    const unsigned char *iq_data = buffer + HEADER_SIZE;
    size_t iq_bytes = (size_t)bytes_read - HEADER_SIZE;
    size_t num_int16 = iq_bytes / sizeof(int16_t);
    num_int16 -= (num_int16 % 2); // гарантируем чётное количество (I,Q пары)

    out.resize(num_int16);
    std::memcpy(out.data(), iq_data, num_int16 * sizeof(int16_t));

    return true;
}
