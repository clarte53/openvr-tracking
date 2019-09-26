# Description

This application provide a fast streaming of OpenVR tracking data to any
application. The data is provided through the standard output of the
application in a simple interroperable way.

# Getting started

This application should be built as a standard console application. The
application should then be launched from another application as a process, and
it's standard output should be redirected to get the data.

The update frequency can be passed on the command line as the first parameter.
The frequency is expected to be an unsigned integer representing the timespan
between to captured frame, in milliseconds. The default value is 10
milliseconds.

To close the application, press the enter key.

# Data format

The tracking data is written as blocks of bytes, with 12 floats per frame,
representing the HmdMatrix34_t data from OpenVR API. This matrix represent
the head pose of the HMD in the tracking system.
