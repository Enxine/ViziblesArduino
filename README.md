# Introduction

This library is intended for connecting Arduino based sensors and actuators to the [Vizibles IoT plaform](https://vizibles.com). Most communications complexity is hidden inside the library, so it offers an easy to use interface for the programmer to focus on its application and forget about things like security, pairing, etc.
The library is ready to use with some simple running examples. Just follow the steps below to get everything working. By the way, do not forget to register into [Vizibles](https://vizibles.com).
 
Read this in other languages: [English](https://github.com/Enxine/ViziblesArduino/blob/master/README.md), [Español](https://github.com/Enxine/ViziblesArduino/blob/master/README.es.md) 

# Install the Arduino IDE

If you do not have it yet or your Arduino IDE verion is not up to date please follow the links and instructions [here](https://www.arduino.cc/en/main/software) for installing the latest version.
Once you have the IDE installed you will need to install also the support libraries for the board you will use, if it is not an original Arduino board, as for example, if you are using ESP8266 based boards, like I am doing, you will need to install the libraries for supporting ESP8266 based boards. In the main IDE window go to File, Preferences, and in the Additional Boards Manager URLs text box write ```http://arduino.esp8266.com/stable/package_esp8266com_index.json```.
There is not yet an official boards package for ESP32, so please follow the instructions [here](https://github.com/espressif/arduino-esp32/blob/master/README.md#installation-instructions). 

# Installing the libraries

To make this run you will need to clone or download a copy of the ViziblesArduino library to your Arduino IDE libraries' directory, but you will need to do the same also with these other libraries:

- Crypto suite: [https://github.com/spaniakos/Cryptosuite](https://github.com/spaniakos/Cryptosuite)
- Time: [https://github.com/PaulStoffregen/Time](https://github.com/PaulStoffregen/Time)
- HttpClient: [https://github.com/pablorodiz/HttpClient](https://github.com/pablorodiz/HttpClient)
- Arduino web server library: [https://github.com/lasselukkari/aWOT.git](https://github.com/lasselukkari/aWOT.git)
- JSON: [https://github.com/bblanchon/ArduinoJson](https://github.com/bblanchon/ArduinoJson)
- Websockets: [https://github.com/pablorodiz/Arduino-Websocket.git](https://github.com/pablorodiz/Arduino-Websocket.git)

# Supported architectures

This driver is a work in progress, so we are currently working on adapting it to new architectures. Vizibles, even it was designed with constrained resources in mind is a demmanding platform with strong security requirements. So the libray is also quite demanding. Currently the Arduino (C++) version of the library is supported on the following architectures:

- ESP32 based boards
- ESP8266 based boards
- Arduino MKR1000

SSL connections with the platform are only supported on ESP8266 based boards by now, but we are working hard to make it work on Atmel based boards.

# Exploring the examples

In the examples folder there are some ready to run. Some are specific for the MKR1000 board, but most where writen for the [Espresso Lite 2.0](http://www.espressolite.com/) which is the platform we can recomend by now.
Main examples are light-bulb and light-switch, that emulate a light and its corresponding switch. 
You can run both examples on two different boards or run a script to emulate one of the parts in your computer. Just compile and have fun.
But if you want a little more fun, you will want to play with the code, so let's explain it a little.
The first part of the magic is the creation of the client itself. It is not simple, since it requires two socket clients and one server to work with. These clients and server must be created externally to keep the library architecture-independent, even you will not use them directly.
```
WiFiClient wc;
WiFiClient wc1;
WiFiServer ws(DEFAULT_THING_HTTP_SERVER_PORT);
ViziblesArduino client(wc, wc1);
```
If you want to use secure (SSL) connections with the platform change the first line on the socket clients definition section to 
```
WiFiClientSecure wc;
```
If you will not use network calls to local services on other things on your network you can go ahead and use only one WiFiClient object, just use the same identifier in both parameters. In that case you must also consider to avoid the creation of a socket server, since you will probably not use it.
Next thing we do is defining some callback functions we will use later on. So the library has a way to report us when the device connects to the platform, when it disconnects, or when there is an error.
```
void errorCallback(void) {
	Serial.println("Send to cloud failed");
}
void onConnectToVizibles(void) {
#ifdef VZ_CLOUD_DEBUG
	Serial.println("Connected to Vizibles");
#endif /*VZ_CLOUD_DEBUG*/
}

void onDisconnectFromVizibles(void) {
#ifdef VZ_CLOUD_DEBUG
	Serial.println("Disconnected from Vizibles");
#endif /*VZ_CLOUD_DEBUG*/	
}	
```
Then we enter the setup function, where the main thing we do is connecting to the WiFi network of our choice
```
	WiFi.begin("YOUR_WIFI_SSID_HERE", "YOUR_WIFI_PASSWORD_HERE");
	int i = 0;
	while (WiFi.status() != WL_CONNECTED && i<5) {   
		delay(500);
		Serial.print(".");
		i++;
	}
```
Remember to write here the right credentials for your network. This is a work in progress. We expect to be able to pair things to any WiFi network by using a default configuration with an AP running on the thing and a mobile application, but this is not yet ready, so in the mean time just connect using this method.
Once you have network start the server if you will use it later on
```
	ws.begin();
```
And last thing to do on the setup is connecting to the [Vizibles plaform](https://vizibles.com). This is ok for a simple example, but for reliable products you should move the connection primitive to the main loop and manage reconnections when a disconnect is detected. As you can see in the code below we pass the callbacks for those events here.
```
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
```
Most options have default values, so you only need to set the those with relevant values for your application.

If you want to use secure (SSL) connections with the platform change the ```"protocol"``` value to ```"wss"```.
 
You might be thinking what ```convertFlashStringToMemString``` means. It is a good practice in Arduino to store strings in program memory for saving scarce RAM space, but the code to retrieve those strings start showhing everywhere in your code. So I've writen this macro to do it, which you can find on the file ```ViziblesArduino.h```.
Once the thing is connected to the [Vizibles plaform](https://vizibles.com) it only needs to detect and report button switchs (light-switch) 
```
	int tmp = digitalRead(13);
	if (tmp!=keyState) {
		keyState = tmp;
		if (keyState == 1) {
			if(status == 0) status = 1;
			else status = 0;
			keyValuePair values[] = {{ "status", status?(char *)"on":(char *)"off" }, {NULL, NULL}};
			client.update(values, &errorCallback);
			lastUpdate = millis();
		}
	}		
```
or expose functions for turning light on or off (light-bulb)
```
	if(connected && exposed<2) {
		switch(exposed) {
			case 0: 
				if(!client.expose("lightOff", lightOff, exposingErrorCallback)) exposed++;
				break;
			case 1:
				if(!client.expose("lightOn", lightOn, exposingErrorCallback)) exposed++;
				break;
		}	
	}
```
Corresponding callbacks:
```
void exposingErrorCallback(void) {
	exposed--;
}
void lightOn(const char *parameters[]) {
	digitalWrite(LED_OUT,LOW);
	status = 0;
	keyValuePair values[] = {{ "status", "on" }, {NULL, NULL}};
	client.update(values, errorCallback);
	lastUpdate = 0;
}
void lightOff(const char *parameters[]) {
	digitalWrite(LED_OUT,HIGH);
	status = 1;
	keyValuePair values[] = {{ "status", "off" }, {NULL, NULL}};
	client.update(values, errorCallback);
	lastUpdate = 0;
}
```
And finally, there is an additional task on the main loop. If you are offering a server, check it for connections and process requests
```
	WiFiClient c = ws.available(); //Check if any client connected to server
	if (c) client.process(&c);
	else client.process(NULL);
```
# Configuring the library

There are some compile time configuration options in the library, mainly for shrinking code size and/or increasing debug level. You will find all available options together on the file ```config.h```.