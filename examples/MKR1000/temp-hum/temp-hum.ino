// (c) Copyright 2017 Enxine DEV S.C.
// Released under Apache License, version 2.0
// @author: jamartinez@enxine.com
//
// This example shows how to build a connected thermometer and higrometer using
// an Arduino MKR-1000 with a KY-015 DHT11 sensor:
// https://tkkrlab.nl/wiki/Arduino_KY-015_Temperature_and_humidity_sensor_module
//
// We are using input 6 to connect the sensor, but you can configure it to use
// any input changing the value of 'DHTPIN'.
//
// The example also shows how to 'expose' a function, so that you can then
// create a control to invoke that function passing it parameters from your
// dashboard.
//
// It was developed over common Arduino libraries which need to be present in
// your system for this example to compile right:
//
// WiFi:            https://github.com/arduino-libraries/WiFi101
// Web server:      https://github.com/lasselukkari/aWOT
// HttpClient:      https://github.com/pablorodiz/HttpClient
// Websockets:      https://github.com/pablorodiz/Arduino-Websocket
// JSON:            https://github.com/bblanchon/ArduinoJson
// AES-256:         https://github.com/qistoph/ArduinoAES256
// Time:            https://github.com/PaulStoffregen/Time
// Adafruit Sensor: https://github.com/adafruit/Adafruit_Sensor
// DHT Sensor:      https://github.com/adafruit/DHT-sensor-library

#include <WiFi101.h>
#include <ViziblesArduino.h>
#include <DHT.h>

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
const char baseID[] = { "mkr1000-temp-hum" };
/******************************************************************************/
/******************************************************************************/

// Create WiFi clients and server over which the Vizibles library will work
WiFiClient wc;
WiFiClient wc1;
WiFiServer ws(DEFAULT_THING_HTTP_SERVER_PORT);
ViziblesArduino client(wc, wc1);

#define DHTPIN 6 
#define DHTTYPE DHT11 

const float DEFAULT_TARGET_TEMPERATURE = 22;

//Internal variables to store thing status
int connected = 0;
int exposed = 0;
float targetTemperature = DEFAULT_TARGET_TEMPERATURE;
float lastReportedTemperature = -1;
float lastReportedHumidity = -1;

// Exposed functions
void setTargetTemperature(const char *parameters[]) {
	keyValuePair keyValues[2];
	keyValues[0].name = "TargetTemperature";
	keyValues[0].value = (char *)parameters[1];

	keyValues[1].name = NULL;
	keyValues[1].value = NULL; 

	client.update(keyValues, errorCallback);
}

//Callbacks for non sinchronous events on Vizibles primitive functions
void onConnectToVizibles(void) {
	Serial.println(F("Connected to Vizibles"));
	connected = 1;
	targetTemperature = DEFAULT_TARGET_TEMPERATURE;
	lastReportedTemperature = -1;
	lastReportedHumidity = -1;
}

void onDisconnectFromVizibles(void) {
	Serial.println(F("Disconnected from Vizibles"));
	exposed = 1;
	connected = 0;
}	

void errorCallback(void) {
	Serial.println(F("ERROR: send to cloud failed"));
}

void exposingErrorCallback(void) {
	Serial.println(F("ERROR trying to expose function"));
	exposed = 0;
}

DHT dht(DHTPIN, DHTTYPE);

void setup() {
	// Open serial port
	Serial.begin(9600);

	pinMode(DHTPIN, INPUT);
	dht.begin();

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

	if (connected && ! exposed) {
		if (!client.expose("setTargetTemperature", setTargetTemperature, exposingErrorCallback)) exposed = 1;
	} else {
		int err;
		float hum = dht.readHumidity();
		float temp = dht.readTemperature();

		// Check if any reads failed and exit early (to try again).
		if (isnan(hum) || isnan(temp)) {
			Serial.println(F("Failed to read from DHT sensor!"));
			return;
		}
		
		if ((lastReportedTemperature != temp) || (lastReportedHumidity != hum)) {
			keyValuePair keyValues[3];
			char strTemp[5];
			keyValues[0].name = "Temperature";
			snprintf(strTemp, 5, "%f", temp);
			keyValues[0].value = strTemp;

			char strHum[5];
			keyValues[1].name = "Humidity";
			snprintf(strHum, 5, "%f", hum);
			keyValues[1].value = strHum;

			keyValues[2].name = NULL;
			keyValues[2].value = NULL; 

			lastReportedTemperature = temp;
			lastReportedHumidity = hum;

			client.update(keyValues, errorCallback);
		}
	}
	delay(1000);
}
