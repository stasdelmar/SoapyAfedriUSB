# SoapyAfedriUSB - SoapySDR Plugin for Afedri SDR USB

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
