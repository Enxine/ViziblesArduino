// (c) Copyright 2017 Enxine DEV S.C.
// Released under Apache License, version 2.0
// @author: jamartinez@enxine.com
//
// This is the basic example that shows how to connect an Arduino MKR-1000 to
// Vizibles and send data to the platform.
//
// It was developed over common Arduino libraries which need to be present in
// your system for this example to compile right:
//
// WiFi:        https://github.com/arduino-libraries/WiFi101
// Web server:  https://github.com/sontono/aWOT
// HttpClient:  https://github.com/pablorodiz/HttpClient
// Websockets:  https://github.com/pablorodiz/Arduino-Websocket
// JSON:        https://github.com/bblanchon/ArduinoJson
// AES-256:     https://github.com/qistoph/ArduinoAES256
// Time:        https://github.com/PaulStoffregen/Time

#include <WiFi101.h>
#include <ViziblesArduino.h>

/******************************************************************************/
/* These are the configuration parameters you will have to setup before       */
/* running this example, so that the device can connect to your wifi first,   */
/* and to Vizibles then.                                                      */
/* For more details and info see:                                             */
/* https://developers.vizibles.com/devices/arduino-mkr1000                    */
/******************************************************************************/
// Wifi settings
const char WIFI_SSID[] = "<YOUR-WIFI-SSID>";
const char WIFI_PASS[] = "<YOUR-WIFI-PASSWORD>";

// API Key settings
const char apikeyID[] = { "<YOUR-API-KEY-ID>" };
const char apikey[] = { "<YOUR-API-KEY-SECRET>" };

// Thing ID. You can use this default value or give it your own
const char baseID[] = { "mkr1000" };
/******************************************************************************/
/******************************************************************************/

// Create WiFi clients and server over which the Vizibles library will work
WiFiClient wc;
WiFiClient wc1;
WiFiServer ws(DEFAULT_THING_HTTP_SERVER_PORT);
ViziblesArduino client(wc, wc1);

//Callbacks for non synchronous events on Vizibles primitive functions
void onConnectToVizibles(void) {
	Serial.println(F("Connected to Vizibles"));
}

void onDisconnectFromVizibles(void) {
	Serial.println(F("Disconnected from Vizibles"));
}	

void errorCallback(void) {
	Serial.println(F("ERROR: send to cloud failed"));
}

void setup() {
	// Open serial port
	Serial.begin(9600);

	randomSeed(analogRead(0));

	// Connect to wifi
	WiFi.begin(WIFI_SSID, WIFI_PASS);
	int i = 0;
	while (WiFi.status() != WL_CONNECTED && i < 5) {   
		delay(500);
		Serial.print(F("."));
		i++;
	}
	delay(100);
	if (WiFi.status() != WL_CONNECTED) {
		Serial.println(F("Error: WiFi not connected!"));
		return;
	}
	
	// Start server
	ws.begin();

	keyValuePair options[] = {{"hostname", VZ_DEFAULT_HOSTNAME},
				  {"protocol", "ws"},
				  {"id", (char *)baseID},
				  {"keyID", (char *)apikeyID},
				  {"keySecret", (char *)apikey},
				  {(char *)NULL, (char *)NULL }};
	
	client.connect(options, onConnectToVizibles, onDisconnectFromVizibles);
}

void loop() {
	// Check for external connections to the local HTTP server and periodic 
	// process of the status of the connection with Vizibles. This must be
	// included in any Vizibles connected thing.
	WiFiClient c = ws.available();
	if (c) client.process(&c);
	else client.process(NULL);

	keyValuePair keyValues[2];
	char strTemp[6];
	keyValues[0].name = "Temperature";
	float randTemperature = ((float) random(0, 100) / 100) + random(15, 25);
	snprintf(strTemp, 6, "%f", randTemperature);
	keyValues[0].value = strTemp;
	keyValues[1].name = NULL;
	keyValues[1].value = NULL; 
	client.update(keyValues, errorCallback);

	delay(5000);
}
