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

The update frequency could be passed on the command line as the first parameter.
The frequency is expected to be an unsigned integer representing the timespan
between to captured frame, in milliseconds. The default value is 10
milliseconds. If you want to use the socket mode, frequency parameter is mandatory.

One option are also available, `-all` to log also all tracked devices positions.
If this optionis set frequency parameter become mandatory.

To close the application, press the enter key.

```shell
# Log head position on standard output at frequency = 10ms
$> openvr-tracking.exe

# Log head position on standard output at frequency = 50ms
$> openvr-tracking.exe 50

# Start a TCP server on port 12000 and send head position to clients at frequency = 50ms
$> openvr-tracking.exe 50 12000

# Start a TCP server on port 12000 and send head and other devices position to clients at frequency = 50ms
$> openvr-tracking.exe 50 12000 -all

# Log head and other devices position on standard output at frequency = 10ms
$> openvr-tracking.exe 10 -all
```

## Build procedure

To build application start by building the sockpp submodule first.

```shell
cd sockpp
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=.. ..
```

open and build `sockpp.sln` in Release x64 and generate INSTALL project.

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

If option `-all` is set each tracked device is printed with the format: 
`[name#serial]` followed by n series of 12 floats representing a HmdMatrix34_t.

## Data sample
Here is a sample of data with one controller and one tracker connected:

7429c90e2f73f5157b3a2a3f52ce47bede8f38bf217a643f3f3329bb49f4763f1de586be100d473f77333f3fdd4d353e9f15243f218488bf[vive_controller#LHR-FFCD3B46]e092323f8bfb29be267132bf32df003f772939bde7ed753f2f4c8cbe1a5d503f2211373f1aff633eb9a0293faded14c0[vive_tracker#LHR-19B5D14E]82f2513f4f7c12bf2102a33adaf4783fd73ccabb528834bcc5fa7fbfac4c3d3f377a123fb9ed513f08eb4dbcfeb6b3bf

```
TimeStamp: 7429c90e2f73f515
Head position: 7b3a2a3f52ce47bede8f38bf217a643f3f3329bb49f4763f1de586be100d473f77333f3fdd4d353e9f15243f218488bf
First controller: [vive_controller#LHR-FFCD3B46]e092323f8bfb29be267132bf32df003f772939bde7ed753f2f4c8cbe1a5d503f2211373f1aff633eb9a0293faded14c0
First tracker: [vive_tracker#LHR-19B5D14E]82f2513f4f7c12bf2102a33adaf4783fd73ccabb528834bcc5fa7fbfac4c3d3f377a123fb9ed513f08eb4dbcfeb6b3bf
```
