// (c) Copyright 2016 Enxine DEV S.C.
// Released under Apache License, version 2.0
//
// This library is used to send data from a simple sensor 
// to the Vizibles IoT platform in an easy and secure way.

#ifndef _vz_strings_h
#define _vz_strings_h

#include "config.h"

const char viziblesUrl[] PROGMEM = { VZ_DEFAULT_HOSTNAME };
const char timeUrl[] PROGMEM = { "/time" };
const char Path[] PROGMEM = { "/things/me/" };
#ifdef VZ_HTTP_SERVER
const char alfanumericValues[] PROGMEM = {	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789" };
const char configWifiResponse[]	= { "{'configId': '" };
const char wifiAppliedStart[] = { "{ 'wifiApplied': " };
const char wifiAppliedEnd[] = { ", 'capture': true }" };
const char wifiApplied[] = { "wifiApplied" };
const char wifiCapture[] = { "capture" };
const char wifiTrue[] = { "true" };
const char doResponse_0[] PROGMEM = {"HTTP/1.0 200 OK\r\nAccess-Control-Allow-Origin: *\r\nContent-Type: application/json\r\nContent-Length: "};  //Length 97
const char doResponse_2[] PROGMEM = { "{ 'functionId': '" };
const char doResponse_3[] PROGMEM = { "', 'task': '" };
const char doResponse_4[] PROGMEM = { "', 'result': '" };
const char doResponse_5[] PROGMEM = { "'}" };
const char doResponse_6[] PROGMEM = { "ERROR" };
const char doResponse_7[] PROGMEM = { "OK" };
#endif /*VZ_HTTP_SERVER*/
const char ccThingId[] PROGMEM = { "thingId" };
#ifdef VZ_EXECUTE_FUNCTIONS
const char ccParams[] PROGMEM = { "params" };
const char ccTask[] PROGMEM = { "task" };
#endif /*VZ_EXECUTE_FUNCTIONS*/
#ifdef VZ_ITTT_RULES
const char newLocalResponseOk[] PROGMEM = { "{ 'newLocal': 'ok' }" };
const char itttsKey[] PROGMEM = { "ittts" };
const char itttsAddedKey[] PROGMEM = { "itttsAdded" };
const char newLocalAddresses[] PROGMEM = { "addresses" };
const char newLocalTickets[] PROGMEM = { "tickets" };
const char newLocalItttsRemoved[] PROGMEM = { "itttsRemoved" };
const char itttMetaKey[] PROGMEM = { "_Meta:" };
#endif /*VZ_ITTT_RULES*/
#ifdef VZ_WEBSOCKETS
const char WSProtocolThingDo[] PROGMEM = { "thingDo" };
const char WSProtocolFunctionId[] PROGMEM = { "functionId" };
const char WSProtocolUpdateAck[] PROGMEM = { "updateAck" };
const char WSProtocolLocalAck[] PROGMEM = { "localAck" };
const char WSProtocolPingAck[] PROGMEM = { "pingAck" };
#ifdef VZ_EXECUTE_FUNCTIONS
const char WSProtocolFunctionsAck[] PROGMEM = { "functionsAck" };
const char WSProtocolResultAck[] PROGMEM = { "resultAck" };
#endif /*VZ_EXECUTE_FUNCTIONS*/
#ifdef VZ_HTTP_SERVER
const char WSProtocolConfigAck[] PROGMEM = { "configAck" };
#endif /*VZ_HTTP_SERVER*/
#ifdef VZ_ITTT_RULES
const char WSProtocolThingNewLocal[] PROGMEM = { "thingNewLocal" };
#endif /*VZ_ITTT_RULES*/
const char WSProtocolMe[] PROGMEM = { "me" };
#endif /*VZ_WEBSOCKETS*/
const char optionsProtocol[] PROGMEM = { "protocol" };
const char optionsHostname[] PROGMEM = { "hostname" };
//const char optionsCredentials[] PROGMEM = { "credentials" };
const char optionsId[] PROGMEM = { "id" };
const char optionsKeyId[] PROGMEM = { "keyID" };
const char optionsKeySecret[] PROGMEM = { "keySecret" };
const char optionsType[] PROGMEM = { "type" };
const char optionsApiBase[] PROGMEM = { "apiBasePath" };
const char optionsProtocolHttp[] PROGMEM = { "http" };
const char optionsProtocolWss[] PROGMEM = { "wss" };
const char optionsProtocolWs[] PROGMEM = { "ws" };
const char optionsHostLocalhost[] PROGMEM = { "localhost" };
#ifdef VZ_ITTT_RULES
const char itttIf[] PROGMEM = { "if" };
const char itttThen[] PROGMEM = { "then" };
const char itttIs[] PROGMEM = { "is" };
const char itttIsNot[] PROGMEM = { "is not" };
const char itttLessThan[] PROGMEM = { "less than" };
const char itttMoreThan[] PROGMEM = { "more than" };
const char itttRunTask[] PROGMEM = { "runTask" };
const char itttKeyAddition[] PROGMEM = { "123456789012" };
const char itttPayload[] PROGMEM = { "{\"params\":[],\"task\":\"\"}" };
const char itttPath[] PROGMEM = { "/do/" };
const char itttMethodPost[] PROGMEM = { "POST" };
const char itttMetaStart[] PROGMEM = { "{ 'ITTTDone': [" };
const char itttMeta_1[] PROGMEM = { "'" };
const char itttMeta_2[] PROGMEM = { ",'" };
const char itttMetaEnd[] PROGMEM = { "]}" };
#endif /*VZ_ITTT_RULES*/
const char date[] PROGMEM = { "VZ-Date: " };
const char contentType[] PROGMEM = { "application/json" };
const char authorization[] PROGMEM = { "Authorization: VIZIBLES " };
const char weekdays[7][4] PROGMEM = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const char months[12][4] PROGMEM = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
const char UTCBase[] PROGMEM = "Mon, 01 Jan 1970 00:00:00 GMT";
const char contentLenghtHeaderName[] PROGMEM = "Content-Length" ;
const char contentTypeHeaderName[] = "Content-Type";
const char connectionAndHost[] PROGMEM = " HTTP/1.1\r\nConnection: close\r\nHost: ";
const char dotSpace[] PROGMEM = ": ";
const char newLine[] PROGMEM = {"\r\n"};

#endif //_vz_strings_h