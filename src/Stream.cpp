#include "SoapyAfedriUSB.hpp"
#include <SoapySDR/Logger.hpp>
#include <SoapySDR/Formats.hpp>
#include <cstring>

SoapySDR::Stream *SoapyAfedriUSB::setupStream(const int direction, const std::string &format,
                                              const std::vector<size_t> &channels,
                                              const SoapySDR::Kwargs &args)
{
    if (direction != SOAPY_SDR_RX)
    {
        throw std::runtime_error("AfedriUSB: RX only");
    }
    
    if (format != SOAPY_SDR_CS16 && format != SOAPY_SDR_CF32)
    {
        throw std::runtime_error("AfedriUSB: Only CS16 and CF32 formats supported");
    }
    
    std::lock_guard<std::mutex> lock(_streams_protect_mtx);
    
    int stream_id = _stream_sequence_provider++;
    _configured_streams[stream_id] = StreamContext(channels.empty() ? 
                                                    std::vector<size_t>{0} : channels,
                                                    format, false);
    
    SoapySDR::logf(SOAPY_SDR_DEBUG, "AfedriUSB: setupStream id=%d, format=%s", 
                   stream_id, format.c_str());
    
    return (SoapySDR::Stream *)(new int(stream_id));
}

void SoapyAfedriUSB::closeStream(SoapySDR::Stream *stream)
{
    int stream_id = *reinterpret_cast<int *>(stream);
    
    deactivateStream(stream, 0, 0);
    
    {
        std::lock_guard<std::mutex> lock(_streams_protect_mtx);
        _configured_streams.erase(stream_id);
    }
    
    delete reinterpret_cast<int *>(stream);
}

size_t SoapyAfedriUSB::getStreamMTU(SoapySDR::Stream *stream) const
{
    return 1024;
}

int SoapyAfedriUSB::activateStream(SoapySDR::Stream *stream, const int flags,
                                   const long long timeNs, const size_t numElems)
{
    int stream_id = *reinterpret_cast<int *>(stream);
    
    {
        std::lock_guard<std::mutex> lock(_streams_protect_mtx);
        auto it = _configured_streams.find(stream_id);
        if (it != _configured_streams.end())
        {
            it->second.active = true;
        }
    }
    
    _control.startCapture();
    SoapySDR::log(SOAPY_SDR_DEBUG, "AfedriUSB: Stream activated");
    
    return 0;
}

int SoapyAfedriUSB::deactivateStream(SoapySDR::Stream *stream, const int flags,
                                     const long long timeNs)
{
    int stream_id = *reinterpret_cast<int *>(stream);
    
    {
        std::lock_guard<std::mutex> lock(_streams_protect_mtx);
        auto it = _configured_streams.find(stream_id);
        if (it != _configured_streams.end())
        {
            it->second.active = false;
        }
    }
    
    _control.stopCapture();
    SoapySDR::log(SOAPY_SDR_DEBUG, "AfedriUSB: Stream deactivated");
    
    return 0;
}

int SoapyAfedriUSB::readStream(SoapySDR::Stream *stream, void *const *buffs, 
                               const size_t numElems, int &flags, long long &timeNs,
                               const long timeoutUs)
{
    int stream_id = *reinterpret_cast<int *>(stream);
    
    StreamContext *ctx = nullptr;
    {
        std::lock_guard<std::mutex> lock(_streams_protect_mtx);
        auto it = _configured_streams.find(stream_id);
        if (it != _configured_streams.end())
        {
            ctx = &it->second;
        }
    }
    
    if (!ctx || ctx->channels.empty()) return 0;
    
    std::vector<int16_t> data(numElems * 2);
    if (!_data_source->read(data, numElems, timeoutUs / 1000))
    {
        return SOAPY_SDR_TIMEOUT;
    }
    
    if (ctx->format == SOAPY_SDR_CS16)
    {
        std::memcpy(buffs[0], data.data(), data.size() * sizeof(int16_t));
    }
    else if (ctx->format == SOAPY_SDR_CF32)
    {
        float *out = (float *)buffs[0];
        for (size_t i = 0; i < data.size(); i++)
        {
            out[i] = (float)data[i] / 32768.0f;
        }
    }
    
    return numElems;
}
