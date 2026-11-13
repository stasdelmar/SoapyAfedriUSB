// Registration.cpp — регистрация драйвера Afedri SDR-Net (USB) в SoapySDR.
// Управление — через HID-интерфейс (hidapi), I/Q-поток — USB Audio.

#include <SoapySDR/Device.hpp>
#include <SoapySDR/Registry.hpp>
#include <SoapySDR/Logger.hpp>

#include "Device.hpp"   // <-- класс устройства из Device.cpp (см. примечание ниже)

#include <hidapi/hidapi.h>

#include <string>
#include <vector>

static const unsigned short AFEDRI_VID = 0x255d;
static const unsigned short AFEDRI_PID = 0x0002;

// Имя драйвера. Если в системе останется модуль оригинального SoapyAfedri
// с TCP-транспортом под тем же именем — поменяйте на "afedri_usb".
static const char *DRIVER_NAME = "afedri";

static std::string wcharToString(const wchar_t *wstr)
{
    if (wstr == nullptr) return "";
    std::string out;
    for (; *wstr != L'\0'; ++wstr)
    {
        // серийники Afedri — ASCII, поэтому простое приведение безопасно
        if (*wstr > 0 && *wstr < 128) out.push_back(static_cast<char>(*wstr));
    }
    return out;
}

static std::vector<SoapySDR::Kwargs> findAfedriUSB(const SoapySDR::Kwargs &args)
{
    std::vector<SoapySDR::Kwargs> results;

    if (hid_init() != 0)
    {
        SoapySDR::logf(SOAPY_SDR_ERROR, "Afedri: hid_init() failed");
        return results;
    }

    auto *infos = hid_enumerate(AFEDRI_VID, AFEDRI_PID);

    std::vector<std::string> seenSerials;
    for (auto *info = infos; info != nullptr; info = info->next)
    {
        const std::string serial = wcharToString(info->serial_number);

        // композитное устройство может отдать несколько HID-интерфейсов — дедуп по серийнику
        bool seen = false;
        for (const auto &s : seenSerials)
        {
            if (s == serial and not serial.empty()) { seen = true; break; }
        }
        if (seen) continue;

        // фильтр по запросу пользователя: --find="driver=afedri,serial=..."
        if (args.count("serial") != 0 and args.at("serial") != serial) continue;

        seenSerials.push_back(serial);

        SoapySDR::Kwargs dev;
        dev["driver"] = DRIVER_NAME;
        if (not serial.empty()) dev["serial"] = serial;
        dev["hid_path"] = info->path; // make() откроет HID по этому пути

        char lbl[256];
        if (not serial.empty())
            std::snprintf(lbl, sizeof(lbl), "Afedri SDR-Net (USB) :: %s", serial.c_str());
        else
            std::snprintf(lbl, sizeof(lbl), "Afedri SDR-Net (USB) :: %s", info->path);
        dev["label"] = lbl;

        results.push_back(dev);
    }

    hid_free_enumeration(infos);
    return results;
}

static SoapySDR::KwargsList afedriUSBArgs(void)
{
    SoapySDR::KwargsList results;
    SoapySDR::Kwargs dev;
    dev["driver"] = DRIVER_NAME;
    results.push_back(dev);
    return results;
}

static SoapySDR::Device *makeAfedriUSB(const SoapySDR::Kwargs &args)
{
    return new SoapyAfedriUSB(args); // <-- имя класса (см. примечание ниже)
}

static SoapySDR::Registry registerAfedriUSB(
    DRIVER_NAME,
    &findAfedriUSB,
    &makeAfedriUSB,
    SOAPY_SDR_ABI_VERSION);
