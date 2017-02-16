#include <ViziblesArduino.h>
#include <ATCommandSet.h>
#include "platform.h"
//AT commands available
//AT+CONNECT[="<option name>","<option value>"[,...]]\r\n
//AT+UPDATE="<variable name>","<variable value>"[,...]\r\n
//AT+EXPOSE="<function name>"\r\n
//AT+DISCONNECT\r\n
//AT+SETOPTIONS="<option name>","<option value>"[,...]\r\n
//AT+GETMAC\r\n
//AT+WIFICONNECT="<SSID>","<password>"\r\n

/*Example command sequence (Light-switch)
AT+SETOPTIONS="id","light-switch","keyID","<YOUR_KEY_ID_HERE","keySecret","<YOUR_KEY_SECRET_HERE>"
AT+CONNECT
AT+UPDATE="status","on"
...
AT+DISCONNECT
*/

/*Example command sequence (Light-bulb)
AT+SETOPTIONS="id","light-bulb","keyID","<YOUR_KEY_ID_HERE","keySecret","<YOUR_KEY_SECRET_HERE>"
AT+CONNECT
AT+EXPOSE="lightOff"
AT+EXPOSE="lightOn"
...
AT+DISCONNECT
*/

const char ATConnect[] PROGMEM = "AT+CONNECT";
const char ATUpdate[] PROGMEM = "AT+UPDATE";
const char ATExpose[] PROGMEM = "AT+EXPOSE";
const char ATDisconnect[] PROGMEM = "AT+DISCONNECT";
const char ATOption[] PROGMEM = "AT+SETOPTIONS";
const char ATWifiConnect[] PROGMEM = "AT+WIFICONNECT";
const char ATGetMAC[] PROGMEM = "AT+GETMAC";
const char ATOK[] PROGMEM = "OK";
const char ATError[] PROGMEM = "ERROR";
const char ATEndLine[] PROGMEM = "\r\n";
const char optionProtocol[] PROGMEM = "protocol";
#if defined ESP8266
const char optionProtocolDefault[] PROGMEM = "wss";
#else	
const char optionProtocolDefault[] PROGMEM = "ws";
#endif /*ESP8266*/

void SerialPrint_P(PGM_P str) {
  for (uint8_t c; (c = pgm_read_byte(str)); str++) Serial.write(c);
}

void SerialPrintln_P(PGM_P str) {
  SerialPrint_P(str);
  SerialPrint_P(ATEndLine);
}

void errorCB(void) {
	Serial.println(F("+ERROR"));
}

void onConnectCB(void) {
	Serial.println(F("+CONNECTED"));
}

void onDisconnectCB(void) {
	Serial.println(F("+DISCONNECTED"));
}

void cbFunction(const char *parameters[]) {
	if(parameters[0]==NULL) return;
	Serial.print(F("+THINGDO="));
	int i = 0;
	while(parameters[i]!=NULL) {
		if(i==0) Serial.print(F("\""));
		else Serial.print(F(",\""));
		Serial.print(parameters[i]);
		Serial.print(F("\""));
		i++;
	}
	Serial.println();
}

int countOptions(
		char *buffer 		/*!< [in] Buffer with the string to analize (NULL terminated).*/) {
	int comas = 0, quotes = 0, i = 0;
	while(buffer[i]!='\0') {
		if(buffer[i]=='"') quotes++;
		if(buffer[i]==',') comas++;
		i++;
	}
	if((quotes==2 && comas==0) || (comas>0 && quotes==2*(comas+1))) return comas+1;
	else return -1;
}

int parseOptions(
		char *buffer, 		/*!< [in] Buffer with the string to analize (NULL terminated).*/
		char *values[],		/*!< [in] Array of char pointers to store results.*/
		int  valuesSize		/*!< [in] Maximum size of the pointers array.*/) {
	int n = 0, i = 0;
	char startNEnd = 1;
	while(buffer[i]!='\0') {
		if(buffer[i]=='"') {
			if(startNEnd) {
				startNEnd = 0;
				if(n<valuesSize) {
					values[n] = &buffer[i+1];
					n++;
				}
			} else {
				buffer[i]='\0';
				startNEnd = 1;
			}
		}	
		i++;
	}
	return n;
}

void parseATCommand(ViziblesArduino& cloud, char *line) {
	//Serial.println(line);
	if(!strncmp_P(line, ATConnect, strlen_P(ATConnect))) { //AT+CONNECT\r\n
		SerialPrintln_P(ATConnect);
		//You can include all options in the connect command instead of using SETOPTIONS
		int n = 0;
		n = countOptions(line);
		if(n>0 && n%2!=0){
			SerialPrintln_P(ATError);
			return;
		}	
		keyValuePair values[(n/2)+2];
		convertFlashStringToMemString(optionProtocol, _protocol);
		convertFlashStringToMemString(optionProtocolDefault, _prot);
		if(n>0) {
			char *p[n];
			if(parseOptions(line, p, n)!=n) {
				SerialPrintln_P(ATError);
				return;
			} else {
				int i = 0;
				int protocol=0;
				while(i<n) {
					if(!strcmp_P(p[i],optionProtocol))	protocol=1;
					values[i/2].name = p[i];
					values[i/2].value = p[i+1];
					i+=2;
				}
				if(!protocol){
					Serial.println(_prot);
					values[i/2].name = _protocol;
					values[i/2].value = _prot;	
					i+=2;
				}	
				values[i/2].name = NULL;
				values[i/2].value = NULL;
			}
		} else {
			if(line[strlen_P(ATConnect)]!='\r' || line[strlen_P(ATConnect)+1]!='\n') {
				SerialPrintln_P(ATError);
				return;
			}
		}	
		if(cloud.connect(values, onConnectCB, onDisconnectCB)==VIZIBLES_ERROR_INCORRECT_OPTIONS) SerialPrintln_P(ATError);
		else SerialPrintln_P(ATOK);
	} else if(!strncmp_P(line, ATWifiConnect, strlen_P(ATWifiConnect))) { //AT+WIFICONNECT="<SSID>","<password>"\r\n
			SerialPrintln_P(ATWifiConnect);
		int n = 0;
		n = countOptions(line);
		char *p[n];
		if(parseOptions(line, p, n)!=n) SerialPrintln_P(ATError);
		else{
			if (n==2){
				if (connectToWiFiAP(p[0], p[1], WPA2)==0) SerialPrintln_P(ATOK);
				else SerialPrintln_P(ATError);
			} else if (n==1){
				if (connectToWiFiAP(p[0], NULL, OPEN)==0) SerialPrintln_P(ATOK);
				else SerialPrintln_P(ATError);
			}
		}
	} else if(!strncmp_P(line, ATGetMAC, strlen_P(ATGetMAC))) { //AT+GETMAC\r\n%
		char mac[18];
		getLocalMac(mac);
		Serial.print(F("+MAC="));
		Serial.println(mac);
		///SerialPrintln_P(ATOK);
	} else if(!strncmp_P(line, ATUpdate, strlen_P(ATUpdate))) { //AT+UPDATE="<variable name>","<variable value>"[,...]\r\n
		SerialPrintln_P(ATUpdate);
		int n = 0;
		n = countOptions(line);
		if(n%2!=0) SerialPrintln_P(ATError);
		else {
			char *p[n];
			if(parseOptions(line, p, n)!=n) SerialPrintln_P(ATError);
			else {
				keyValuePair payLoadValues[n/2];
				int i = 0, j = 0;
				while(i<n) {
					payLoadValues[j].name = p[i];
					payLoadValues[j].value = p[i+1];
					i+=2;
					j++;
				}	
				payLoadValues[j].name = NULL;
				payLoadValues[j].value = NULL;
				if(cloud.update(payLoadValues, errorCB)!=0) SerialPrintln_P(ATError);
				else SerialPrintln_P(ATOK);
			}
		}
	} else if(!strncmp_P(line, ATExpose, strlen_P(ATExpose))) { //AT+EXPOSE="<function name>"\r\n
		SerialPrintln_P(ATExpose);
		int n = 0;
		n = countOptions(line);
		//Serial.println(n);
		if(n!=1) SerialPrintln_P(ATError);
		else {
			char *p[n];
			//Serial.println(parseOptions(line, p, n));
			if(parseOptions(line, p, n)!=n) SerialPrintln_P(ATError);
			else {
				int err = cloud.expose(p[0], cbFunction, errorCB);
				if(err!=0) SerialPrintln_P(ATError);
				else SerialPrintln_P(ATOK);
			}
		}
	} else if(!strncmp_P(line, ATDisconnect, strlen_P(ATDisconnect))) { //AT+DISCONNECT\r\n
		SerialPrintln_P(ATDisconnect);
		if(line[strlen_P(ATDisconnect)]=='\r' && line[strlen_P(ATDisconnect)+1]=='\n') {
			cloud.disconnect();
			SerialPrintln_P(ATOK);
		} else SerialPrintln_P(ATError);	
	} else if(!strncmp_P(line, ATOption, strlen_P(ATOption))) { //AT+SETOPTIONS="<option name>","<option value>"[,...]\r\n
		SerialPrintln_P(ATOption);
		int n = 0;
		n = countOptions(line);
		if(n%2!=0) {
			SerialPrintln_P(ATError);
			Serial.println("1");
		}	
		else {
			char *p[n];
			if(parseOptions(line, p, n)!=n) {
				SerialPrintln_P(ATError);
				Serial.println("2");
			}
			else {
				int protocol = 0;
				int i = 0;
				while(i<n){
					if(!strcmp_P(p[i],optionProtocol))	protocol=1;
					cloud.option(p[i], p[i+1]);
					i+=2;
				}
				if(!protocol){
					convertFlashStringToMemString(optionProtocol, _protocol);
					convertFlashStringToMemString(optionProtocolDefault, _prot);
					cloud.option(_protocol, _prot);
				}
				SerialPrintln_P(ATOK);
			}
		}
	} else {
		SerialPrintln_P(ATError);
	}	
}


