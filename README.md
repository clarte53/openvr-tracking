# Description

This application provide a fast streaming of OpenVR tracking data to any
application. The data is provided through the standard output of the
application in a simple interroperable way.

# Getting started

This application should be built as a standard console application. The
application could then be launched from another application as a process, and
it's standard output should be redirected to get the data. An other way, is
to launch the application with a socket port as second parameter. In this
case the application open a TCP server listenning for connection then
streaming the data.

The update frequency is passed on the command line as the first parameter.
The frequency is expected to be an unsigned integer representing the timespan
between to captured frame, in milliseconds. The default value is 10
milliseconds.

To close the application, press the enter key.

## Build procedure

To build application start by building the sockpp submodule first.

```shell
cd sockpp
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=.. ..
```

open and build `sockpp.sln`.

Next build the final application

```shell
cd ..\..
mkdir build
cd build
cmake ..
```

open and build `OpenVR-Tracking.sln`.

# Data format

The tracking data is written as blocks of bytes in hexadecimal, with a long long
timestamp (usually 64 bits) followed by 12 floats (usually 32 bits each)
per frame. The bytes are written in big endian format (network order). Each
frame is written on a new line. A frame represents the HmdMatrix34_t data from
OpenVR API for the head pose of the HMD in the tracking system. The timestamp
is the time in nanoseconds since epoch (Unix time, i.e since 01/01/1970).
