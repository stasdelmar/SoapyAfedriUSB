#include "AfedriControlUSB.hpp"
#include <SoapySDR/Logger.hpp>

#include <cstring>
#include <cstdlib>
#include <unistd.h>

AfedriControlUSB::AfedriControlUSB(const std::string &serial)
    : _serial(serial)
{
}

AfedriControlUSB::~AfedriControlUSB()
{
    close();
}

bool AfedriControlUSB::init()
{
    if (hid_init() < 0)
    {
        SoapySDR::log(SOAPY_SDR_ERROR, "AfedriUSB: hid_init failed");
        return false;
    }

    if (!openDevice())
    {
        SoapySDR::log(SOAPY_SDR_ERROR, "AfedriUSB: Failed to open HID device");
        hid_exit();
        return false;
    }

    SoapySDR::log(SOAPY_SDR_INFO, "AfedriUSB: USB HID device opened successfully");
    return true;
}

void AfedriControlUSB::close()
{
    if (_handle)
    {
        hid_close(_handle);
        _handle = nullptr;
        hid_exit();
    }
}

bool AfedriControlUSB::openDevice()
{
    if (!_serial.empty())
    {
        wchar_t serial_wide[256] = {0};
        std::mbstowcs(serial_wide, _serial.c_str(), 255);
        _handle = hid_open(USB_VID, USB_PID, serial_wide);
    }
    else
    {
        _handle = hid_open(USB_VID, USB_PID, nullptr);
    }

    if (!_handle)
    {
        SoapySDR::logf(SOAPY_SDR_ERROR,
                       "AfedriUSB: Cannot open device VID=%04X PID=%04X (serial=%s)",
                       USB_VID, USB_PID, _serial.empty() ? "any" : _serial.c_str());
        return false;
    }

    SoapySDR::logf(SOAPY_SDR_INFO, "AfedriUSB: Device opened (serial: %s)",
                   _serial.empty() ? "auto" : _serial.c_str());
    return true;
}

int AfedriControlUSB::sendHIDCommand(const unsigned char *cmd, int cmd_size,
                                     unsigned char *resp, int resp_size)
{
    if (!_handle)
    {
        SoapySDR::log(SOAPY_SDR_ERROR, "AfedriUSB: sendHIDCommand - device not connected");
        return -1;
    }

    int ret = hid_write(_handle, cmd, cmd_size);
    if (ret < 0)
    {
        SoapySDR::log(SOAPY_SDR_ERROR, "AfedriUSB: hid_write failed");
        return -1;
    }

    int bytes = hid_read_timeout(_handle, resp, resp_size, 1000);
    if (bytes < 0)
    {
        SoapySDR::log(SOAPY_SDR_ERROR, "AfedriUSB: hid_read_timeout failed");
        return -1;
    }
    if (bytes == 0)
    {
        SoapySDR::log(SOAPY_SDR_WARNING, "AfedriUSB: hid_read_timeout - no response (timeout)");
    }

    return bytes;
}

bool AfedriControlUSB::setFrequency(unsigned long freq, int channel)
{
    unsigned char cmd[8] = {0};
    cmd[0] = 0x00; // Report ID
    cmd[1] = HID_SET_R820T_FREQ_COMMAND;
    cmd[2] = (freq >> 0) & 0xFF;
    cmd[3] = (freq >> 8) & 0xFF;
    cmd[4] = (freq >> 16) & 0xFF;
    cmd[5] = (freq >> 24) & 0xFF;
    cmd[6] = (unsigned char)channel;
    cmd[7] = 0x00;

    unsigned char response[64] = {0};
    int bytes = sendHIDCommand(cmd, sizeof(cmd), response, sizeof(response));
    if (bytes < 0)
    {
        return false;
    }

    SoapySDR::logf(SOAPY_SDR_DEBUG, "AfedriUSB: setFrequency %lu Hz on channel %d - OK",
                   freq, channel);
    return true;
}

bool AfedriControlUSB::setSampleRate(unsigned long rate)
{
    unsigned char cmd[8] = {0};
    cmd[0] = 0x00;
    cmd[1] = HID_GENERIC_SET_SAMPLE_RATE_COMMAND;
    cmd[2] = (rate >> 0) & 0xFF;
    cmd[3] = (rate >> 8) & 0xFF;
    cmd[4] = (rate >> 16) & 0xFF;
    cmd[5] = (rate >> 24) & 0xFF;
    cmd[6] = 0x00;
    cmd[7] = 0x00;

    unsigned char response[64] = {0};
    int bytes = sendHIDCommand(cmd, sizeof(cmd), response, sizeof(response));
    if (bytes < 0)
    {
        return false;
    }

    SoapySDR::logf(SOAPY_SDR_DEBUG, "AfedriUSB: setSampleRate %lu Hz - OK", rate);
    return true;
}

bool AfedriControlUSB::setGain(int gain, int channel)
{
    unsigned char cmd[8] = {0};
    cmd[0] = 0x00;
    cmd[1] = HID_SET_R820T_LNA_GAIN_COMMAND;
    cmd[2] = (unsigned char)gain;
    cmd[3] = (unsigned char)channel;
    cmd[4] = 0x00;
    cmd[5] = 0x00;
    cmd[6] = 0x00;
    cmd[7] = 0x00;

    unsigned char response[64] = {0};
    int bytes = sendHIDCommand(cmd, sizeof(cmd), response, sizeof(response));
    if (bytes < 0)
    {
        return false;
    }

    SoapySDR::logf(SOAPY_SDR_DEBUG, "AfedriUSB: setGain %d on channel %d - OK", gain, channel);
    return true;
}

bool AfedriControlUSB::startCapture()
{
    unsigned char cmd[8] = {0};
    cmd[0] = 0x00;
    cmd[1] = HID_GENERIC_START_UDP_STREAM_COMMAND;
    cmd[2] = 0x01; // Start
    cmd[3] = 0x00;
    cmd[4] = 0x00;
    cmd[5] = 0x00;
    cmd[6] = 0x00;
    cmd[7] = 0x00;

    unsigned char response[64] = {0};
    int bytes = sendHIDCommand(cmd, sizeof(cmd), response, sizeof(response));
    if (bytes < 0)
    {
        return false;
    }

    usleep(100000); // 100ms для стабилизации потока

    SoapySDR::log(SOAPY_SDR_DEBUG, "AfedriUSB: startCapture - OK");
    return true;
}

bool AfedriControlUSB::stopCapture()
{
    unsigned char cmd[8] = {0};
    cmd[0] = 0x00;
    cmd[1] = HID_GENERIC_START_UDP_STREAM_COMMAND;
    cmd[2] = 0x00; // Stop
    cmd[3] = 0x00;
    cmd[4] = 0x00;
    cmd[5] = 0x00;
    cmd[6] = 0x00;
    cmd[7] = 0x00;

    unsigned char response[64] = {0};
    int bytes = sendHIDCommand(cmd, sizeof(cmd), response, sizeof(response));
    if (bytes < 0)
    {
        return false;
    }

    SoapySDR::log(SOAPY_SDR_DEBUG, "AfedriUSB: stopCapture - OK");
    return true;
}

std::string AfedriControlUSB::getSerialNumber() const
{
    if (!_handle) return "";

    wchar_t serial[256] = {0};
    if (hid_get_serial_number_string(_handle, serial, 255) < 0)
    {
        return "unknown";
    }

    char serial_str[256] = {0};
    std::wcstombs(serial_str, serial, 255);
    return std::string(serial_str);
}

std::string AfedriControlUSB::getVersionInfo() const
{
    if (!_handle) return "Afedri SDR USB";

    wchar_t product[256] = {0};
    if (hid_get_product_string(_handle, product, 255) < 0)
    {
        return "Afedri SDR USB";
    }

    char product_str[256] = {0};
    std::wcstombs(product_str, product, 255);
    return std::string(product_str);
}
