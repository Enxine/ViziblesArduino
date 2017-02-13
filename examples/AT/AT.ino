// (c) Copyright 2016 Enxine DEV S.C.
// Released under Apache License, version 2.0
//
// This example shows how to use Vizibles and its Sensor
// Client library to send temperature, humidity and light
// updates periodically to the platform, measured with a
// DHT11 (temperature and humidity) and a TEMT6000 (ambient
// light) sensors. 
// It was developed over common Arduino libraries which need
// to be present in your system for this example to compile
// rigth:
// Crypto suite: https://github.com/spaniakos/Cryptosuite
// Time: https://github.com/PaulStoffregen/Time
// HttpClient: https://github.com/pablorodiz/HttpClient
// Arduino web server library: https://github.com/lasselukkari/aWOT.git
// JSON: https://github.com/bblanchon/ArduinoJson
// Websockets: https://github.com/pablorodiz/Arduino-Websocket.git

#include <HttpClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <aWOT.h>
#include <ViziblesArduino.h>
#include <ATCommandSet.h>

#define MAX_LINE_SIZE 254

// Create WiFi and HTTP clients over which the Vizibles library will work
#if defined ESP8266
WiFiClientSecure wc;
#else
WiFiClient wc;
#endif /*ESP8266*/
WiFiClient wc1;
WiFiServer ws(DEFAULT_THING_HTTP_SERVER_PORT);
ViziblesArduino client(wc, wc1);


void setup()
{
	// Enable serial communications
	Serial.begin(9600);

	// Start TCP server
	ws.begin();

	//Wait for a while for everything to stabilize
	delay(100);
}

char line[MAX_LINE_SIZE+1];
unsigned int lineCount = 0;
unsigned int started = 0;
void loop()
{
	if(!started) {
		Serial.println(F("+VZ-READY>\r\n"));	
		started = 1;
	}
	
	if (Serial.available()) {
		char c = '\0';
		while(Serial.available() && c!='\n' && lineCount<MAX_LINE_SIZE) {
			c = Serial.read();
			line[lineCount++] = c;
			Serial.print(c);
		}
		if(c=='\n' || lineCount==MAX_LINE_SIZE) {
			line[lineCount]='\0';
			parseATCommand(client, line);
			lineCount = 0;
		}	
	}


	if(ws.hasClient()) { //Check if any client connected to server
		WiFiClient c = ws.available();
		client.process(&c);
	} else client.process(NULL);

}
