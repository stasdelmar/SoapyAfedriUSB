#include <SoapySDR/Device.hpp>
#include <SoapySDR/Registry.hpp>
#include "SoapyAfedriUSB.hpp"   // имя класса — поправим под ваш заголовок

static std::vector<SoapySDR::Kwargs> findAfedriUSB(const SoapySDR::Kwargs &args)
{
    std::vector<SoapySDR::Kwargs> results;
    // перечисление HID-устройств через hid_enumerate(VID, PID)
    // для каждого найденного Afedri — заполняем Kwargs: driver=afedri_usb, serial, ...
    return results;
}

static SoapySDR::KwargsList afedriUSBArgs(void)
{
    SoapySDR::Kwargs args;
    args["driver"] = "afedri_usb";
    SoapySDR::KwargsList results;
    results.push_back(args);
    return results;
}

static SoapySDR::Device *makeAfedriUSB(const SoapySDR::Kwargs &args)
{
    return new SoapyAfedriUSB(args);
}

// регистрация модуля — макросом из SoapySDR/Registry.hpp вашей версии
