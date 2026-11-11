#pragma once

#include <string>
#include <cstdint>
#include <hidapi/hidapi.h>

class AfedriControlUSB
{
public:
    explicit AfedriControlUSB(const std::string &serial = "");
    ~AfedriControlUSB();

    bool init();
    void close();
    bool isConnected() const { return _handle != nullptr; }

    // Commands
    bool setFrequency(unsigned long freq, int channel = 0);
    bool setSampleRate(unsigned long rate);
    bool setGain(int gain, int channel = 0);
    bool startCapture();
    bool stopCapture();

    // Info
    std::string getSerialNumber() const;
    std::string getVersionInfo() const;

private:
    static constexpr uint16_t USB_VID = 0x0483;
    static constexpr uint16_t USB_PID = 0x5932;

    // HID Command codes (из fe_interface.h)
    static constexpr unsigned char HID_SET_R820T_FREQ_COMMAND = 78;
    static constexpr unsigned char HID_GENERIC_SET_SAMPLE_RATE_COMMAND = 30;
    static constexpr unsigned char HID_SET_R820T_LNA_GAIN_COMMAND = 79;
    static constexpr unsigned char HID_GENERIC_START_UDP_STREAM_COMMAND = 26;

    hid_device *_handle = nullptr;
    std::string _serial;

    bool openDevice();
    int sendHIDCommand(const unsigned char *cmd, int cmd_size,
                       unsigned char *resp, int resp_size);
};
