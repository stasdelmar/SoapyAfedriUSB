# SoapyAfedriUSB - SoapySDR Plugin for Afedri SDR USB
# AI GENERATED !!! NOT TESTED!!! DRAFT!!!
USB support for Afedri SDR receivers via SoapySDR framework.

## Features

- USB HID control via HIDAPI
- Support for MSD (Mass Storage Device) mode file reading
- Support for UDP I/Q data streaming
- Multi-channel support (1-4 channels)
- Sample rates up to 1536000 Hz
- Frequency range 0-2 GHz
- Compatible with GQRX, CubicSDR, OpenWebRXPlus

## Requirements

bash
sudo apt-get install libsoapysdr-dev libhidapi-dev libusb-1.0-0-dev cmake

## Building

mkdir build
cd build
cmake ..
make
sudo make install
sudo ldconfig

## Usage

### Python

import soapysdr

# Discover device
devices = soapysdr.Device.enumerate({"driver": "afedri_usb"})
sdr = soapysdr.Device.make(devices[0])

# Configure
sdr.setFrequency(soapysdr.SOAPY_SDR_RX, 0, 14200000)
sdr.setGain(soapysdr.SOAPY_SDR_RX, 0, 20)
sdr.setSampleRate(soapysdr.SOAPY_SDR_RX, 0, 1536000)

# Stream I/Q data
stream = sdr.setupStream(soapysdr.SOAPY_SDR_RX, "cf32", [0])
sdr.activateStream(stream)

samples = sdr.readStream(stream, [1024], timeout=1.0)

### OpenWebRXPlus

Edit config.json:

{
  "source": {
    "type": "soapysdr",
    "device": "afedri_usb"
  }
}

## License

GPL-3.0+

## Authors

Based on SoapyAfedri by Alexander Sholokhov (RA9YER)
Sources from [https://afedri-sdr.com/](https://afedri-sdr.com/index.php/downloads) Written by 4Z5LV

# AI GENERATED !!! NOT TESTED!!! DRAFT!!!
