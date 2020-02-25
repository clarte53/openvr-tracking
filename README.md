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
`[type#name#serial]` followed 2 unsigned long long (64bits) buttons status
pressed and touched then by 12 floats representing a HmdMatrix34_t. Type is
`T` for tracker `CR` for right controller, `CL` for left controller, and `C?`
for unknown hand assigned controller.

## Data sample
Here is a sample of data with one controller and one tracker connected:

706f11862e5df6157b3a2a3f52ce47bede8f38bf217a643f3f3329bb49f4763f1de586be100d473f77333f3fdd4d353e9f15243f218488bf[T#vive_tracker#LHR-19B5D14E]0000000000000000000000000000000040b996be90a374bf8d78393c186aa33f6fea043afaa044bc45fb7fbf00d63a3ff2a7743fb7b596bef8a4833b3b24adbf[CR#vive_controller#LHR-FFF75945]00000000000000000000000000000000324bdabe35716fbe0db25fbf658d833fa2a221bd4c4b783f4f0e76beec0b4e3f8b58673fe8318bbd7c72d8bebd018cbf

```
TimeStamp: 706f11862e5df615
Head position : 7b3a2a3f52ce47bede8f38bf217a643f3f3329bb49f4763f1de586be100d473f77333f3fdd4d353e9f15243f218488bf
First Tracker id : [T#vive_tracker#LHR-19B5D14E]
First Tracker buttons: pressed 0000000000000000 touched 0000000000000000
First Tracker position : 40b996be90a374bf8d78393c186aa33f6fea043afaa044bc45fb7fbf00d63a3ff2a7743fb7b596bef8a4833b3b24adbf
Right controller id : [CR#vive_controller#LHR-FFF75945]
Right controller buttons : pressed 0000000002000000 touched 0000000002000000
Right controller position : 324bdabe35716fbe0db25fbf658d833fa2a221bd4c4b783f4f0e76beec0b4e3f8b58673fe8318bbd7c72d8bebd018cbf
```
