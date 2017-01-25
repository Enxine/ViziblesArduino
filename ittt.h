// (c) Copyright 2016 Enxine DEV S.C.
// Released under Apache License, version 2.0
//
// This library is used to send data froma simple sensor 
// to the Vizibles IoT platform in an easy and secure way.

#ifndef _ittt_h
#define _ittt_h

#include <ViziblesArduino.h>
#include <ArduinoJson.h>
#include <HttpClient.h>

//#define MAX_ITTT_RULES 					5
#define MAX_ITTT_VARIABLE_NAME_LENGTH 	65
#define MAX_ITTT_THING_ID_LENGTH 		65
#define MAX_ITTT_VALUE_LENGTH 			65
#define MAX_ITTT_FUNCTION_NAME_LENGTH	65
#define MAX_ITTT_ADDRESS_LENGTH			17

#define ITTT_TICKET_LENGTH				20
#define ITTT_ID_LENGTH					12

#define MAX_ITTT_BUFFER_LENGTH			1024

typedef enum {IS, IS_NOT, LESS_THAN, MORE_THAN} itttCondition_t;

typedef struct {
	char *id;
	char *variable;
	char *value;
	char *thingId;
	char *function;
	itttCondition_t condition;
} itttRule_t;

typedef struct {
	char *thingId;
	char *address;
	char *ticket;
} itttThing_t;	

void initITTT(HttpClient *http, char *thingId);
void initITTTRules(void);
int testITTTRules(keyValuePair values[], char *meta, ViziblesArduino *client);
int addITTTRule(JsonObject& rule);
int removeITTTRule(char *id);
int addITTTAddress(JsonObject& addresses);
int addITTTTickets(JsonObject& tickets, char *key);
int itttGetNumberOfActiveRules();

#endif //_ittt_h