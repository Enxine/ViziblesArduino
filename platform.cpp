// (c) Copyright 2016 Enxine DEV S.C.
// Released under Apache License, version 2.0
//
// This library is used to send data from a simple sensor
// to the Vizibles IoT platform in an easy and secure way.

#include <ViziblesArduino.h>
#include "platform.h"
#include "utils.h"

#if defined ESP8266
#include <ESP8266WiFi.h>
#include <IPAddress.h>
#include "lwip/netif.h"
#include "netif/etharp.h"
#elif defined ARDUINO_SAMD_MKR1000
#include <WiFi101.h>
#include <IPAddress.h>
#else
#error Vizibles platform specific functions not implemented for your architecture on platform.c
#endif

static int apActive = 0;
static int wifiConnected = 0;
static int tryingToConnectToLastAP = 1;

void macToStr(const uint8_t* mac, char *buffer) {
	LOGLN(F("[platform] macToStr()"));
	char inv[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
	int k = 0;
	for (int i = 0; i < 6; ++i) {
		buffer[k++] = inv[(mac[i] & 0xF0) >> 4];
		buffer[k++] = inv[(mac[i] & 0x0F)];
		if (i < 5) buffer[k++] = ':';
	}
	buffer[k] = '\0';
}

long getMACSeed(void) {
	LOGLN(F("[platform] getMACSeed()"));
	long m = 0;
#if defined ESP8266 || defined ARDUINO_SAMD_MKR1000
	unsigned char mac[6];
	WiFi.macAddress(mac);
	for(int i=0; i<6; i++) {
		m += mac[i]<<(8*i);
	}
#endif
	LOG(F("[platform] getMACSeed(): "));
	LOGLN(m);
	return m;
}

String IpAddress2String(const IPAddress &ipAddress) {
	LOGLN(F("[platform] IpAddress2String()"));
	return String(ipAddress[0]) + String(".") +	\
		String(ipAddress[1]) + String(".") +	\
		String(ipAddress[2]) + String(".") +	\
		String(ipAddress[3])  ;
}

char *getLocalIp(char *buf /*16 bytes*/) {
	LOGLN(F("[platform] getLocalIp()"));
#if defined ESP8266
	String clientIp = WiFi.localIP().toString();
	strcpy(buf, &clientIp[0]);
#elif defined ARDUINO_SAMD_MKR1000
	String clientIp = IpAddress2String(WiFi.localIP());
	strcpy(buf, &clientIp[0]);
#endif
	LOG(F("[platform] getLocalIp(): "));
	LOGLN(buf);
	return buf;
}

char *getGatewayIp(char *buf /*16 bytes*/) {
	LOGLN(F("[platform] getGatewayIp()"));
#if defined ESP8266
	String gatewayIp = WiFi.gatewayIP().toString();
	strcpy(buf, &gatewayIp[0]);
#elif defined ARDUINO_SAMD_MKR1000
	String gatewayIp = IpAddress2String(WiFi.gatewayIP());
	strcpy(buf, &gatewayIp[0]);
#endif
	LOG(F("[platform] getGatewayIp(): "));
	LOGLN(buf);
	return buf;
}

char *getLocalMac(char *buf /*18 bytes*/) {
	LOGLN(F("[platform] getLocalMac()"));
#if defined ESP8266 || defined ARDUINO_SAMD_MKR1000
	unsigned char mac[6] = {0,0,0,0,0,0};
	WiFi.macAddress(mac);
	macToStr(mac, buf);
#endif
	LOG(F("[platform] getLocalMac(): "));
	LOGLN(buf);
	return buf;
}

char *getGatewayMac(char *buf /*18 bytes*/) {
	LOGLN(F("[platform] getGatewayMac()"));
	uint32_t gwip = WiFi.gatewayIP();
	struct eth_addr *eth_ret;
	int entry = -1;

#if defined ESP8266
	// TBD: error 'ip_route' was not declared in this scope
	//
	return NULL;
	//
	// netif* interface = ip_route((ip_addr_t *)&gwip);
	// ip_addr_t *ip_ret;
	// etharp_request(interface, (ip_addr_t *)&gwip);
	// delay (500); //TODO: This is a magic number. Less time induces erroneous results on arp response
	// int t = 0;
	// while ((entry == -1) && t < 20) {
	// 	delay(100);
	// 	entry = etharp_find_addr(interface, (ip_addr_t *)&gwip, &eth_ret, &ip_ret);
	// 	t++;
	// }
	// if (entry == -1 || t == 20) {
	// 	LOGLN(F("[platform] getGatewayMac() error: ARP error, gateway not found"));
	// 	return NULL;
	// } else {
	// 	macToStr((unsigned char *)eth_ret, buf);
	// 	LOG(F("[platform] getGatewayMac(): "));
	// 	LOGLN(buf);
	// 	return buf;
	// }
#elif defined ARDUINO_SAMD_MKR1000
	// TODO
	buf[0] = '1';
	buf[1] = '8';
	buf[2] = ':';
	buf[3] = '5';
	buf[4] = '5';
	buf[5] = ':';
	buf[6] = '0';
	buf[7] = 'F';
	buf[8] = ':';
	buf[9] = 'B';
	buf[10] = 'E';
	buf[11] = ':';
	buf[12] = 'F';
	buf[13] = '7';
	buf[14] = ':';
	buf[15] = '8';
	buf[16] = '5';
	buf[17] = '\0';


	LOG(F("[platform] getGatewayMac(): "));
	LOGLN(buf);
	return buf;
#endif
}

int connectToWiFiAP(char *SSID, char *passwd, encryptionType_t encType) {
	LOGLN(F("[platform] connectToWiFiAP()"));
	LOG(F("connectToWiFiAP(\""));
	LOG(SSID);
	LOG(F("\", \""));
	LOG(passwd);
	LOG(F("\", "));
#if defined ESP8266 || defined ARDUINO_SAMD_MKR1000
	//Enable WiFi
	switch (encType) {
	case OPEN:
		LOG(F("OPEN"));
		WiFi.begin(SSID);
		break;
	case WEP:
		LOG(F("WEP"));
	case WPA:
#ifdef VZ_CLOUD_DEBUG
		if(encType!=WEP) LOG(F("WPA"));
#endif /*VZ_CLOUD_DEBUG*/
	case WPA2:
#ifdef VZ_CLOUD_DEBUG
		if(encType!=WEP && encType!=WPA) LOG(F("WPA2"));
#endif /*VZ_CLOUD_DEBUG*/
		WiFi.begin(SSID, passwd);
		break;
	}
	LOGLN(F(")"));
	double t = millis();
	while (WiFi.status() != WL_CONNECTED && elapsedMillis(t)<20000) {
		delay(500);
		LOG(".");
	}
	if(WiFi.status() != WL_CONNECTED) {
		LOGLN(F("connectToWiFiAP() error: Failed to connect"));
		return -1;
	}
#if defined ESP8266 // TODO: what about MKR1000?
	WiFi.setAutoConnect(true);
#endif
	wifiConnected = 1;
	delay(100);
	char gwMac[18];
	getGatewayMac(gwMac);
	LOGLN();
	LOGLN(F("[platform] connectToWiFiAP(): Connection OK"));
	LOG(F("Local IP address: "));
	LOGLN(WiFi.localIP());
#endif
	return 0;
}

int disconnectFromWiFiAp(void) {
	LOGLN(F("[platform] disconnectFromWiFiAp()"));
#if defined ESP8266
	if(WiFi.disconnect()) {
		LOGLN(F("[platform] disconnectFromWiFiAp(): disconnected OK"));
		wifiConnected = 0;
		return 0;
	} else {
		LOGLN(F("[platform] disconnectFromWiFiAp() error: Error disconnecting"));
		return -1;
	}
#elif defined ARDUINO_SAMD_MKR1000
	WiFi.disconnect();
	wifiConnected = 0;
	return 0;
#endif
}

int isWiFiConnected(void) {
	LOGLN(F("[platform] isWiFiConnected()"));
	bool wCon = false;
#if defined ESP8266
	if(WiFi.isConnected()) wCon = true;
#elif defined ARDUINO_SAMD_MKR1000
	if(WiFi.status() == WL_CONNECTED) wCon = true;
#endif
	if(wCon) {
		if(!wifiConnected) {
			wifiConnected = 1;
			delay(100);
			char gwMac[18];
			getGatewayMac(gwMac);
		}
		LOG(F("[platform] isWiFiConnected(): "));
		LOGLN(F("yes"));
		return 1;
	} else {
		wifiConnected = 0;
		LOG(F("[platform] isWiFiConnected(): "));
		LOGLN(F("no"));
		return 0;
	}
}

int enableWiFiAp(void) {
	LOGLN(F("[platform] enableWiFiAp()"));
#if defined ESP8266
	WiFi.setAutoConnect(false);
	char ssid[] = VZ_DEFAULT_AP_SSID;
	char mac[18];
	unsigned char m[6];
	WiFi.softAPmacAddress((uint8_t *) m);
	macToStr(m, mac);
	int ssidLen = strlen(ssid);
	ssid[ssidLen-4] = mac[12];
	ssid[ssidLen-3] = mac[13];
	ssid[ssidLen-2] = mac[15];
	ssid[ssidLen-1] = mac[16];
	if(!WiFi.softAP(ssid, VZ_DEFAULT_AP_PASSWD)) return -1;
	apActive = 1;
	IPAddress localIp(192, 168, 240, 1);
	IPAddress subnet(255, 255, 255, 0);
	if(!WiFi.softAPConfig(localIp, localIp, subnet)) {
		LOGLN(F("[platform] enableWiFiAp() error: Impossible to activate WiFi access point"));
		disableWiFiAp();
		return -1;
	}
	LOGLN(F("[platform] enableWiFiAp(): WiFi access point enabled OK"));
	return 0;
#elif defined ARDUINO_SAMD_MKR1000
	// TODO
	return 0;
#endif
}

int disableWiFiAp(void) {
	LOGLN(F("[platform] disableWiFiAp()"));
#if defined ESP8266
	//if(WiFi.isConnected()) {
	if(WiFi.status() == WL_CONNECTED) {
		String key = WiFi.psk();
		String ssid = WiFi.SSID();
		WiFi.disconnect();
		WiFi.softAPdisconnect(true);
		WiFi.begin(&ssid[0], &key[0]);
	} else {
		WiFi.softAPdisconnect(true);
	}
	apActive = 0;
	return 0;
#elif defined ARDUINO_SAMD_MKR1000
	// TODO
	return 0;
#endif
}

int isApActive(void) {
	//LOG(F("[platform] isApActive(): "));
	//LOGLN(apActive);
	return apActive;
}

void tryToConnectToLastAP(bool b) {
	LOGLN(F("[platform] tryToConnectToLastAP()"));
	if(b){
		tryingToConnectToLastAP = 1;
		LOGLN(F("[platform] tryToConnectToLastAP(true)"));
	} else {
		tryingToConnectToLastAP = 0;
		LOGLN(F("[platform] tryToConnectToLastAP(false)"));
	}
#if defined ESP8266 // TODO: what about MKR1000?
	WiFi.setAutoConnect(b);
#endif
}

bool isTryingToConnectToLastAP(void) {
	LOGLN(F("[platform] isTryingToConnectToLastAP()"));
	if(tryingToConnectToLastAP) return true;
	else return false;
}
