cmake_minimum_required(VERSION 3.3)
project(SoapyAfedriUSB CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

########################################################################
# Find SoapySDR
########################################################################
find_package(SoapySDR CONFIG)
if(NOT SoapySDR_FOUND)
    message(WARNING "SoapySDR development files not found - skipping support")
    return()
endif()

########################################################################
# Find hidapi (используем pkg-config)
########################################################################
find_package(PkgConfig REQUIRED)
pkg_check_modules(HIDAPI REQUIRED hidapi-libusb)
# Примечание: на некоторых системах пакет называется hidapi-hidraw
# Если hidapi-libusb не найден, попробуйте:
# pkg_check_modules(HIDAPI REQUIRED hidapi)

########################################################################
# Include directories
########################################################################
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${HIDAPI_INCLUDE_DIRS}
)

########################################################################
# Source files
########################################################################
set(SOURCES
    src/Registration.cpp
    src/Device.cpp
    src/Stream.cpp
    src/AfedriControlUSB.cpp
    src/DataSource.cpp
)

########################################################################
# Build the module - используем официальный SoapySDR макрос!
########################################################################
SOAPY_SDR_MODULE_UTIL(
    TARGET afedri_usb_support
    SOURCES ${SOURCES}
    LIBRARIES ${HIDAPI_LIBRARIES}
)
