// (c) Copyright 2016 Enxine DEV S.C.
// Released under Apache License, version 2.0
//
// This library is used to send data from a simple sensor 
// to the Vizibles IoT platform in an easy and secure way.

#ifndef _ATCommandSet_h
#define _ATCommandSet_h

#define VZ_SECURE_AT_CLIENT

void parseATCommand(ViziblesArduino& cloud, char *line);

#endif //_ATCommandSet_h