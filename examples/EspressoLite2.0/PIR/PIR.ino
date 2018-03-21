// (c) Copyright 2017 Enxine DEV S.C.
// Released under Apache License, version 2.0
//
// This example shows how to use Vizibles and its Arduino
// Client library to send movement detection the platform,
// using a low cost PIR breakout board and ESpresso Lite 
// v2.0 board. 
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
#include <platform.h>


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

const char baseID[] PROGMEM = { "PIR-" };
/******************************************************************************/
/******************************************************************************/

#define PIR_INPUT 		12 
#define LED_OUTPUT		2


// Create WiFi clients and server over which the Vizibles library will work
WiFiClientSecure wc;
WiFiClient wc1;
WiFiServer ws(DEFAULT_THING_HTTP_SERVER_PORT);
ViziblesArduino client(wc, wc1);

//Internal variables for storing thing state

unsigned int pirState = 0;
unsigned int ledActive = 1;
int connected = 0;
int pendingUpdate = 1;
int exposed = 2; 


//Callbacks for non sinchronous events on Vizibles primitive functions

void onConnectToVizibles(void) {
#ifdef VZ_CLOUD_DEBUG
    Serial.println("Connected to Vizibles");
#endif
    connected = 1;
    exposed = 0;
}

void onDisconnectFromVizibles(void) {
#ifdef VZ_CLOUD_DEBUG
    Serial.println("Disconnected from Vizibles");
#endif
    connected = 0;
    exposed = 2;
}	

void errorCallback(void) {
#ifdef VZ_CLOUD_DEBUG
    Serial.println("Send to cloud failed");
#endif
}

void exposingErrorCallback(void) {
    exposed--;
}

//Exposed functions

void activateLED(const char *parameters[]) {
#ifdef VZ_CLOUD_DEBUG
    Serial.println("LED activated");
#endif
    ledActive = 1;
    pendingUpdate = 1;
}

void deactivateLED(const char *parameters[]) {
#ifdef VZ_CLOUD_DEBUG
    Serial.println("LED deactivated");
#endif
    ledActive = 0;
    pendingUpdate = 1;
}

//Thing setup
void setup() {
#ifdef VZ_CLOUD_DEBUG
    // Enable serial communications
    Serial.begin(9600);
#endif	
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    int i = 0;
    while (WiFi.status() != WL_CONNECTED && i<5) {   
	delay(500);
#ifdef VZ_CLOUD_DEBUG
	Serial.print(".");
#endif
	i++;
    }
    delay(100);
#ifdef VZ_CLOUD_DEBUG
    Serial.println();
    Serial.println(WiFi.localIP());
#endif	

    // Start TCP server
    ws.begin();
	
    // initialize inputs
    pinMode(PIR_INPUT, INPUT);
    pinMode(LED_OUTPUT, OUTPUT);
    digitalWrite(LED_OUTPUT, HIGH);
    //Get API key
    convertFlashStringToMemString(apikey, key);
    //Get API key ID
    convertFlashStringToMemString(apikeyID, keyid);
	
    //Create id from constant string and last mac numbers
    char mac[18];
    getLocalMac(mac);
    int baseIdLen = strlen_P(baseID);
    char id[baseIdLen+7];
    strcpy_P(id, baseID);
    id[baseIdLen] = mac[9];
    id[baseIdLen+1] = mac[10];
    id[baseIdLen+2] = mac[12];
    id[baseIdLen+3] = mac[13];
    id[baseIdLen+4] = mac[15];
    id[baseIdLen+5] = mac[16];
    id[baseIdLen+6] = '\0';
	
    keyValuePair options[] = {{"protocol", "wss"},
			      {"id", id},
			      {"keyID", keyid},
			      {"keySecret", key},
			      {(char *)NULL, (char *)NULL }};
#ifdef VZ_CLOUD_DEBUG
    Serial.println(id);
#endif	
    client.connect(options, onConnectToVizibles, onDisconnectFromVizibles);
    attachInterrupt(digitalPinToInterrupt(PIR_INPUT), interrupt, CHANGE);
}


void interrupt() {
    pendingUpdate = 1;
}

void loop() {
    // Check for external connections to the local HTTP server and periodic 
    // process of the status of the connection with Vizibles. This must be
    // included in any Vizibles connected thing.
    if (ws.hasClient()) { //Check if any client connected to server
	WiFiClient c = ws.available();
	client.process(&c);
    } else {
	client.process(NULL);
    }

    //Test PIR INPUT
    pirState = digitalRead(PIR_INPUT);
    if (ledActive && pirState) digitalWrite(LED_OUTPUT, LOW);
    else digitalWrite(LED_OUTPUT, HIGH);

    //Expose functions as soon as connected
    if (connected) {
	if (exposed < 2) {
	    switch(exposed) {
	    case 0: 
		if (!client.expose("activateLED", activateLED, exposingErrorCallback)) exposed++;
		break;
	    case 1:
		if (!client.expose("deactivateLED", deactivateLED, exposingErrorCallback)) exposed++;
		break;
	    }
	    //Or update variables when they change			
	} else if (pendingUpdate) {
	    keyValuePair values[3];
	    values[0].name = "movement";
	    values[0].value = pirState ? (char*)"on" : (char*)"off";
	    values[1].name = "led";
	    values[1].value = ledActive ? (char*)"active" : (char*)"unactive";
	    values[2].name = NULL;
	    values[2].value = NULL; 
	    if (!client.update(values, errorCallback)) pendingUpdate = 0;;
			
	}
    }	
    delay(20);
}
