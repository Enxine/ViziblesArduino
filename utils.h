// (c) Copyright 2016 Enxine DEV S.C.
// Released under Apache License, version 2.0
//
// This library is used to send data froma simple sensor 
// to the Vizibles IoT platform in an easy and secure way.

#ifndef _utils_h
#define _utils_h

#include <ViziblesArduino.h>
#include <HttpClient.h>

#define NETWORK_TIMEOUT 30000
#define NETWORK_DELAY   20

//#define DEBUG_PROCESSING_TIME

#ifdef DEBUG_PROCESSING_TIME
#define printTimeCreateVariables()		\
	double t = millis(), t1 = t;

#define printTimeSpent(message)			\
	Serial.print(message);			\
	t1 = millis();				\
	Serial.println(t1-t);			\
	t=t1;
#else 
#define printTimeCreateVariables()
#define	printTimeSpent(message)
#endif

void printChar(char c);
void hexToAscii(char *hex, char *ascii, unsigned int len);
void asciiToHex(char *ascii, char *hex, unsigned int len);
int isNumber(char *str);
int isInteger(char *str);
int HTTPRequest(HttpClient *http, char *hostname, unsigned int port, char *path, char *method, char *contentType, char *payload, char *headers[], char *response, unsigned int resLen);
int HTTPFastRequest(HttpClient *http, char *hostname, unsigned int port, char *path, char *method, char *contentType, char *payload, char *headers[], char *response, unsigned int resLen);
//void createHashSignature(char *buffer, char *key, char *httpMethod, char *protocol, char *hostName, unsigned int port, char *path, char *contentType, char *dateString, char *payload);
void createHashSignature(char *buffer, char *key, char *httpMethod,	char *protocol,	char *hostName,	unsigned int port, char *path, char *contentType, char *id,	char *dateString, char *payload);
void getDateString(char *UTCTime);
long parseDateString(char *UTCTime);
unsigned long elapsedMillis(unsigned long m);


#endif //_utils_h
