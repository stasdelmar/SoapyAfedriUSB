#include "SoapyAfedriUSB.hpp"
#include <SoapySDR/Logger.hpp>

SoapyAfedriUSB::SoapyAfedriUSB(const std::string& usb_serial,
                               const std::string& mode,
                               const std::string& msd_path,
                               int udp_port)
    : _control(usb_serial),
      _num_channels(1),
      _frequency(1000000),
      _sample_rate(192000),
      _gain(0),
      _antenna("Vertical"),
      _stream_sequence_provider(1)
{
    SoapySDR::logf(SOAPY_SDR_INFO, "AfedriUSB: Initializing (mode=%s)", mode.c_str());
    
    if (!_control.init())
    {
        throw std::runtime_error("Failed to initialize USB HID control");
    }
    
    if (mode == "msd")
    {
        _data_source = std::make_unique<MSDDataSource>(msd_path);
    }
    else if (mode == "udp")
    {
        _data_source = std::make_unique<UDPDataSource>(udp_port);
    }
    else
    {
        throw std::runtime_error("Unknown data source mode: " + mode);
    }
    
    if (!_data_source->init())
    {
        throw std::runtime_error("Failed to initialize data source");
    }
    
    SoapySDR::log(SOAPY_SDR_INFO, "AfedriUSB: Device initialized successfully");
}

SoapyAfedriUSB::~SoapyAfedriUSB()
{
    _control.close();
}

std::string SoapyAfedriUSB::getDriverKey() const
{
    return "afedri_usb";
}

std::string SoapyAfedriUSB::getHardwareKey() const
{
    return "Afedri SDR USB";
}

SoapySDR::Kwargs SoapyAfedriUSB::getHardwareInfo() const
{
    SoapySDR::Kwargs info;
    info["origin"] = "https://afedri-sdr.com";
    info["version"] = "0.1.0";
    info["serial"] = _control.getSerialNumber();
    return info;
}

size_t SoapyAfedriUSB::getNumChannels(const int dir) const
{
    return (dir == SOAPY_SDR_RX) ? _num_channels : 0;
}

bool SoapyAfedriUSB::getFullDuplex(const int dir, const size_t channel) const
{
    return false;
}

// Frequency
void SoapyAfedriUSB::setFrequency(const int dir, const size_t channel, 
                                  const std::string &name, const double frequency,
                                  const SoapySDR::Kwargs &args)
{
    if (dir == SOAPY_SDR_RX && name == "RF")
    {
        _frequency = frequency;
        _control.setFrequency((unsigned long)frequency, channel);
    }
}

double SoapyAfedriUSB::getFrequency(const int dir, const size_t channel, 
                                    const std::string &name) const
{
    if (dir == SOAPY_SDR_RX && name == "RF") return _frequency;
    return 0;
}

std::vector<std::string> SoapyAfedriUSB::listFrequencies(const int dir, 
                                                         const size_t channel) const
{
    return {"RF"};
}

SoapySDR::RangeList SoapyAfedriUSB::getFrequencyRange(const int dir, const size_t channel,
                                                      const std::string &name) const
{
    return SoapySDR::RangeList{SoapySDR::Range(0, 2000e6)};
}

// Gain
std::vector<std::string> SoapyAfedriUSB::listGains(const int dir, 
                                                   const size_t channel) const
{
    return {"RF"};
}

void SoapyAfedriUSB::setGain(const int dir, const size_t channel, const double value)
{
    if (dir == SOAPY_SDR_RX)
    {
        _gain = value;
        _control.setGain((int)value, channel);
    }
}

double SoapyAfedriUSB::getGain(const int dir, const size_t channel) const
{
    return (dir == SOAPY_SDR_RX) ? _gain : 0;
}

SoapySDR::Range SoapyAfedriUSB::getGainRange(const int dir, const size_t channel) const
{
    return SoapySDR::Range(0, 30);
}

// Sample rate
void SoapyAfedriUSB::setSampleRate(const int dir, const size_t channel, const double rate)
{
    if (dir == SOAPY_SDR_RX)
    {
        _sample_rate = rate;
        _control.setSampleRate((unsigned long)rate);
    }
}

double SoapyAfedriUSB::getSampleRate(const int dir, const size_t channel) const
{
    return (dir == SOAPY_SDR_RX) ? _sample_rate : 0;
}

std::vector<double> SoapyAfedriUSB::listSampleRates(const int dir, const size_t channel) const
{
    return {192000, 768000, 1536000};
}

SoapySDR::RangeList SoapyAfedriUSB::getSampleRateRange(const int dir, const size_t channel) const
{
    return SoapySDR::RangeList{SoapySDR::Range(48000, 1536000)};
}

// Antenna
std::vector<std::string> SoapyAfedriUSB::listAntennas(const int dir, const size_t channel) const
{
    return {"Vertical"};
}

void SoapyAfedriUSB::setAntenna(const int dir, const size_t channel, const std::string &name)
{
    _antenna = name;
}

std::string SoapyAfedriUSB::getAntenna(const int dir, const size_t channel) const
{
    return _antenna;
}
