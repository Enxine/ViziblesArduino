// (c) Copyright 2017 Enxine DEV S.C.
// Released under Apache License, version 2.0
// @author: jamartinez@enxine.com
//
// This example shows how to connect the I/O control board developed by Enxine
// to Vizibles.
// It is a board with capacity to control up to two inputs and two outputs at
// 220V, as you can read here:
//   https://developers.vizibles.com/en/devices/conf2switch/
//
// It was developed over common Arduino libraries which need to be present in
// your system for this example to compile right:
//
// Web server:  https://github.com/lasselukkari/aWOT
// HttpClient:  https://github.com/pablorodiz/HttpClient
// Websockets:  https://github.com/pablorodiz/Arduino-Websocket
// JSON:        https://github.com/bblanchon/ArduinoJson
// AES-256:     https://github.com/qistoph/ArduinoAES256
// Time:        https://github.com/PaulStoffregen/Time

#include <ViziblesArduino.h>
#include <ESP8266WiFi.h>

/******************************************************************************/
/* These are the configuration parameters you will have to setup before       */
/* running this example, so that the device can connect to your wifi first,   */
/* and to Vizibles then.                                                      */
/******************************************************************************/
// Wifi settings
const char WIFI_SSID[] = "<YOUR-WIFI-SSID>";
const char WIFI_PASS[] = "<YOUR-WIFI-PASSWORD>";

// API Key settings
const char apikeyID[] = { "<YOUR-API-KEY-ID>" };
const char apikey[] = { "<YOUR-API-KEY-SECRET>" };

// Thing ID. You can use this default value or give it your own
const char thingID[] = { "io-control" };
/******************************************************************************/
/******************************************************************************/

// Correspondence between inputs/outputs and GPIOs
#define OUTPUT1 5
#define OUTPUT2 16
#define INPUT1 12
#define INPUT2 13

// Create WiFi clients and server over which the Vizibles library will work
WiFiClient wc;
WiFiClient wc1;
WiFiServer ws(DEFAULT_THING_HTTP_SERVER_PORT);
ViziblesArduino client(wc, wc1);

// Status vars
unsigned int input1 = -1;
unsigned int input2 = -1;
int output1 = 0;
int output2 = 0;
int exposed = 0;
int connected = 0;

void output1On(const char *parameters[]) {
	digitalWrite(OUTPUT1, HIGH);
	output1 = 1;
	keyValuePair values[] = {{ "output1", "on" }, {NULL, NULL}};
	client.update(values, errorCallback);
}

void output1Off(const char *parameters[]) {
	digitalWrite(OUTPUT1, LOW);
	output1 = 0;
	keyValuePair values[] = {{ "output1", "off" }, {NULL, NULL}};
	client.update(values, errorCallback);
}

void output1Switch(const char *parameters[]) {
	if (output1 == 0) output1On(NULL);
	else output1Off(NULL);
}

void output2On(const char *parameters[]) {
	digitalWrite(OUTPUT2, HIGH);
	output2 = 1;
	keyValuePair values[] = {{ "output2", "on" }, {NULL, NULL}};
	client.update(values, errorCallback);
}

void output2Off(const char *parameters[]) {
	digitalWrite(OUTPUT2, LOW);
	output2 = 0;
	keyValuePair values[] = {{ "output2", "off" }, {NULL, NULL}};
	client.update(values, errorCallback);
}

void output2Switch(const char *parameters[]) {
	if (output2 == 0) output2On(NULL);
	else output2Off(NULL);
}

//Callbacks for non synchronous events on Vizibles primitive functions
void onConnectToVizibles(void) {
	Serial.println(F("Connected to Vizibles"));
	connected = 1;
}

void onDisconnectFromVizibles(void) {
	Serial.println(F("Disconnected from Vizibles"));
	connected = 0;
}	

void errorCallback(void) {
	Serial.println(F("ERROR: send to cloud failed"));
}

void exposingErrorCallback(void) {
	exposed--;
}

void setup() {
#ifdef VZ_CLOUD_DEBUG
	// Enable serial communications
	Serial.begin(9600);
#endif	

	// initialize outputs
	pinMode(INPUT1, INPUT);
	pinMode(INPUT2, INPUT);
	pinMode(OUTPUT1, OUTPUT);
	pinMode(OUTPUT2, OUTPUT);
	digitalWrite(OUTPUT1, LOW);
	digitalWrite(OUTPUT2, LOW);

	// Connect to wifi
	WiFi.begin(WIFI_SSID, WIFI_PASS);
	int i = 0;
	while (WiFi.status() != WL_CONNECTED && i < 40) {   
		delay(500);
		LOG(F("."));
		i++;
	}
	LOGLN(F(""));
	delay(100);
	if (WiFi.status() != WL_CONNECTED) {
		Serial.println(F("Error: WiFi not connected!"));
		return;
	}
	
	// Start server
	ws.begin();

	keyValuePair options[] = {{"hostname", VZ_DEFAULT_HOSTNAME},
				  {"protocol", "ws"},
				  {"id", (char *)thingID},
				  {"keyID", (char *)apikeyID},
				  {"keySecret", (char *)apikey},
				  {(char *)NULL, (char *)NULL }};
	
	if (client.connect(options, onConnectToVizibles, onDisconnectFromVizibles) != 0) {
	       Serial.println(F("Error: can NOT connect with Vizibles!"));
	}
}

void loop() {
	// Check for external connections to the local HTTP server and periodic 
	// process of the status of the connection with Vizibles. This must be
	// included in any Vizibles connected thing.
	WiFiClient c = ws.available();
	if (c) client.process(&c);
	else client.process(NULL);
	
	if (connected && exposed < 6) {
		switch (exposed) {
		case 0: 
			if(! client.expose("output1Off", output1Off, exposingErrorCallback)) exposed++;
			break;
		case 1:
			if(! client.expose("output1On", output1On, exposingErrorCallback)) exposed++;
			break;
		case 2:
			if(! client.expose("output1Switch", output1Switch, exposingErrorCallback)) exposed++;
			break;
		case 3: 
			if(! client.expose("output2Off", output2Off, exposingErrorCallback)) exposed++;
			break;
		case 4:
			if(! client.expose("output2On", output2On, exposingErrorCallback)) exposed++;
			break;
		case 5:
			if(! client.expose("output2Switch", output2Switch, exposingErrorCallback)) exposed++;
			break;
		}	
	}

	// Take into account that the on state of the input with AC input signal
	// is pulsed, not a permanent state. So we need a little buffer of
	// previous states and a little delay to distinguish among on and off
	// states of the inputs.
	int tmpInput = digitalRead(INPUT1) ? 0 : 1;
	unsigned int input1Old = input1;
	input1 = (input1 << 1) | (tmpInput & 0x01); 
	if (input1 && !input1Old) {
		keyValuePair values[] = {{ "input1", "on" }, {NULL, NULL}};
		client.update(values, errorCallback);
	} else if (!input1 && input1Old) {
		keyValuePair values[] = {{ "input1", "off" }, {NULL, NULL}};
		client.update(values, errorCallback);
	}

	tmpInput = digitalRead(INPUT2) ? 0 : 1;
	unsigned int input2Old = input2;
	input2 = (input2 << 1) | (tmpInput & 0x01); 
	if (input2 && !input2Old) {
		keyValuePair values[] = {{ "input2", "on" }, {NULL, NULL}};
		client.update(values, errorCallback);
	} else if (!input2 && input2Old) {
		keyValuePair values[] = {{ "input2", "off" }, {NULL, NULL}};
		client.update(values, errorCallback);
	}

	delay(20);
}
