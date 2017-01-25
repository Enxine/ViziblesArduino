// (c) Copyright 2016 Enxine DEV S.C.
// Released under Apache License, version 2.0
//
// This library is used to send data froma simple sensor 
// to the Vizibles IoT platform in an easy and secure way.

#ifndef _platform_h
#define _platform_h

#include <ViziblesArduino.h>

typedef enum {OPEN, WEP, WPA, WPA2} encryptionType_t;

long getMACSeed(void);
char *getLocalIp(char *buf /*16 bytes*/);
char *getGatewayIp(char *buf /*16 bytes*/);
char *getLocalMac(char *buf /*18 bytes*/);
char *getGatewayMac(char *buf /*18 bytes*/);
int connectToWiFiAP(char *SSID, char *passwd, encryptionType_t encType);
int disconnectFromWiFiAp(void);
int isWiFiConnected(void);
int enableWiFiAp(void);
int disableWiFiAp(void);
int isApActive(void);
void tryToConnectToLastAP(bool b);
bool isTryingToConnectToLastAP(void);

#endif //_platform_h