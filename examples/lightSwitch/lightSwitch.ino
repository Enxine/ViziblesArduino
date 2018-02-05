// (c) Copyright 2016 Enxine DEV S.C.
// Released under Apache License, version 2.0
//
// This example shows how to use Vizibles and its Sensor
// Client library for sending data to the platform
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
#ifdef ESP8266
#include <ESP8266WiFi.h>
#elif defined ESP32
#include <WiFi.h>
#include <WiFiClientSecure.h>
#endif
#include <ArduinoJson.h>
#include <aWOT.h>
#include <ViziblesArduino.h>
#include <platform.h>

// API key ID to identificate the key you want to use to authenticate the sensor in the platform
//IMPORTANT: Use your own API key here to avoid replicated IDs
const unsigned char apikeyID[] PROGMEM = { "YOUR_API_KEY_ID_HERE" };
// API key to authenticate the sensor in the platform
//IMPORTANT: Use your own API key here to avoid replicated IDs
const unsigned char apikey[] PROGMEM = { "YOUR_API_KEY_SECRET_HERE" };
// Thing ID to identify the sensor inside the platform
const unsigned char thingID[] PROGMEM = { "light-switch" };

#define INPUT_SW 13

// Create WiFi clients and server over which the Vizibles library will work
WiFiClient wc;
WiFiClient wc1;
WiFiServer ws(DEFAULT_THING_HTTP_SERVER_PORT);
ViziblesArduino client(wc, wc1);

int connected = 0;
int pendingUpdate = 1;
int status = 1;

void errorCallback(void) {
#ifdef VZ_CLOUD_DEBUG
	Serial.println("Send to cloud failed");
#endif /*VZ_CLOUD_DEBUG*/
}
void onConnectToVizibles(void) {
	connected = 1;
#ifdef VZ_CLOUD_DEBUG
	Serial.println("Connected to Vizibles");
#endif /*VZ_CLOUD_DEBUG*/
}

void onDisconnectFromVizibles(void) {
	connected = 0;
#ifdef VZ_CLOUD_DEBUG
	Serial.println("Disconnected from Vizibles");
#endif /*VZ_CLOUD_DEBUG*/	
}	

void setup()
{
#ifdef VZ_CLOUD_DEBUG
	// Enable serial communications
	Serial.begin(9600);
#endif	
	delay(100);
	WiFi.begin("YOUR_WIFI_SSID_HERE", "YOUR_WIFI_PASSWORD_HERE");
	int i = 0;
	while (WiFi.status() != WL_CONNECTED && i<5) {   
		delay(500);
#ifdef VZ_CLOUD_DEBUG
		Serial.print(".");
#endif
		i++;
	}
#ifdef VZ_CLOUD_DEBUG
	Serial.println();
	Serial.println(WiFi.localIP());
#endif	

	// Start TCP server
	ws.begin();
	
	// initialize input
	pinMode(INPUT_SW, INPUT);
#ifdef ESP32
	attachInterrupt(INPUT_SW, interrupt, FALLING); //ESP32 uses pin number directly as interrupt number
#else	
	attachInterrupt(digitalPinToInterrupt(INPUT_SW), interrupt, FALLING); //In ESP8266, instead, the interrupt number must be deducted from pin number
#endif	
	
	//Get API key
	convertFlashStringToMemString(apikey, key);
	//Get API key ID
	convertFlashStringToMemString(apikeyID, keyid);
	//Get thing ID
	convertFlashStringToMemString(thingID, thingid);

	keyValuePair options[] = {{"protocol", "ws"},
							  {"id", thingid},
							  {"keyID", keyid},
							  {"keySecret", key},
							  {(char *)NULL, (char *)NULL }};
	
	client.connect(options, onConnectToVizibles, onDisconnectFromVizibles);
}


void interrupt() {
	if(status == 0) status = 1;
	else status = 0;
	pendingUpdate = 1;
}

void loop()
{
	WiFiClient c = ws.available(); //Check if any client connected to server
	if (c) client.process(&c);
	else client.process(NULL);
	delay(20);	
	if(connected && pendingUpdate){
		keyValuePair values[] = {{ "status", status?(char *)"on":(char *)"off" }, {NULL, NULL}};
		client.update(values, &errorCallback); 
		pendingUpdate = 0;
	}
}
