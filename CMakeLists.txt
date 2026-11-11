########################################################################
# SoapyAfedriUSB - Afedri SDR USB Plugin for SoapySDR
########################################################################
cmake_minimum_required(VERSION 3.3)
project(SoapyAfedriUSB CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pthread")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

########################################################################
# Find SoapySDR
########################################################################
find_package(SoapySDR CONFIG)
if(NOT SoapySDR_FOUND)
    message(FATAL_ERROR "SoapySDR development files not found!")
endif()

########################################################################
# Find hidapi via pkg-config
# На Debian/Ubuntu пакет: libhidapi-dev
# Может называться hidapi-libusb или hidapi-hidraw в зависимости от backend
########################################################################
find_package(PkgConfig REQUIRED)

pkg_check_modules(HIDAPI hidapi-libusb)
if(NOT HIDAPI_FOUND)
    pkg_check_modules(HIDAPI hidapi-hidraw)
endif()
if(NOT HIDAPI_FOUND)
    pkg_check_modules(HIDAPI hidapi)
endif()
if(NOT HIDAPI_FOUND)
    message(FATAL_ERROR "HIDAPI not found! Install: sudo apt-get install libhidapi-dev")
endif()

message(STATUS "HIDAPI found: ${HIDAPI_LIBRARIES}")

########################################################################
# Threads
########################################################################
find_package(Threads REQUIRED)

########################################################################
# Include directories
########################################################################
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${HIDAPI_INCLUDE_DIRS}
)
link_directories(${HIDAPI_LIBRARY_DIRS})

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
# Build the module using official SoapySDR macro
########################################################################
SOAPY_SDR_MODULE_UTIL(
    TARGET afedri_usb_support
    SOURCES ${SOURCES}
    LIBRARIES ${HIDAPI_LIBRARIES} Threads::Threads
)
