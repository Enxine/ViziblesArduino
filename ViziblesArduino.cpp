// (c) Copyright 2016 Enxine DEV S.C.
// Released under Apache License, version 2.0
//
// This library is used to send data froma simple sensor 
// to the Vizibles IoT platform in an easy and secure way.
// It was developed over common Arduino libraries which need
// to be present in your system for the examples to compile
// rigth:
// Crypto suite: https://github.com/spaniakos/Cryptosuite
// Time: https://github.com/PaulStoffregen/Time
// HttpClient: https://github.com/pablorodiz/HttpClient
// Arduino web server library: https://github.com/lasselukkari/aWOT
// JSON: https://github.com/bblanchon/ArduinoJson
// AES: https://github.com/qistoph/ArduinoAES256
// Websockets: https://github.com/pablorodiz/Arduino-Websocket.git

#include <ViziblesArduino.h>
#include "strings.h"
#include <HttpClient.h>
#include <TimeLib.h>
#include <ArduinoJson.h>
#ifdef VZ_HTTP_SERVER
#include <aWOT.h>
#endif /*VZ_HTTP_SERVER*/
#include "platform.h"
#ifdef VZ_ITTT_RULES
#include "ittt.h"
#endif
#include "utils.h"
#ifdef VZ_WEBSOCKETS
#include <WebSocketClient.h>
#endif
#include <sha1.h>
#include "b64.h"


//Constants
#ifdef VZ_HTTP_SERVER
const char authorizationHeaderName[] = "Authorization";
const char dateHeaderName[] = "VZ-Date";
#endif /*VZ_HTTP_SERVER*/

//Internal variables
unsigned long ViziblesArduino::lastSync = 0; 										/*!< Time when time was updated last time.*/
tOptions ViziblesArduino::options;													/*!< Main configuration options.*/
HttpClient *ViziblesArduino::httpClient;											/*!< HTTP client used in all outgoing communications.*/
Client *ViziblesArduino::mainClient = NULL;                                        /*!< Network client used in all outgoing communications.*/
#ifdef VZ_EXECUTE_FUNCTIONS
functions_t ViziblesArduino::functions[MAX_EXPOSED_FUNCTIONS]; 					/*!< Functions available for remote calls.*/
unsigned char ViziblesArduino::exposed = 0;										/*!< Number of functions available for remote calls.*/
char ViziblesArduino::functionNames[MAX_EXPOSED_FUNCTIONS_NAME_BUFFER];
int ViziblesArduino::functionNamesIndex = 0;
#endif /*VZ_EXECUTE_FUNCTIONS*/
#ifdef VZ_HTTP_SERVER
ViziblesArduino *instance = NULL; 													/*!< Instance of the ViziblesArduino class to allow external access. */
WebApp *ViziblesArduino::httpService = NULL;                                       /*!< Webserver content manager.*/
char ViziblesArduino::serverReady = 0;												/*!< Flag to check if server was configured properly.*/
int ViziblesArduino::pendingWifiConfig = 0;										/*!< Flag to check if there is a pending try to connect to WiFi.*/
char ViziblesArduino::pendingSsid[33];												/*!< SSID for a pending try to connect to WiFi.*/
char ViziblesArduino::pendingPass[65];												/*!< Password for a pending try to connect to WiFi.*/
char ViziblesArduino::pendingCfId[41];												/*!< ConfigId for a pending try to connect to WiFi.*/
#ifdef VZ_EXECUTE_FUNCTIONS
Router ViziblesArduino::do_("/do/"); 												/*!< Webserver router for do commands.*/
#endif /*VZ_EXECUTE_FUNCTIONS*/
char ViziblesArduino::headerDateBuffer[DATE_HEADER_MAX_LENGTH];                    /*!< Buffer to store Date header content on received HTTP requests.*/
char ViziblesArduino::headerAuthorizationBuffer[AUTHORIZATION_HEADER_MAX_LENGTH];	/*!< Buffer to store Authorization header content on received HTTP requests.*/
char ViziblesArduino::headerContentTypeBuffer[CONTENT_TYPE_HEADER_MAX_LENGTH];     /*!< Buffer to store Content-type header content on received HTTP requests.*/
#endif /*VZ_HTTP_SERVER*/
double ViziblesArduino::lastPing;													/*!< Last ping try time.*/
double ViziblesArduino::lastConnection;											/*!< Last connection try time.*/
double ViziblesArduino::lastWiFiConnection = 0;									/*!< Last WiFi connection try time.*/
unsigned char ViziblesArduino::cloudConnected = 0;									/*!< Connection flag. Zero if not connected to the cloud or one if connected.*/
unsigned char ViziblesArduino::tryToConnect = 0;                                   /*!< Try to connect flag. Zero if must remain unconnected to the cloud or one if must try to connect.*/
#ifdef VZ_WEBSOCKETS
WebSocketClient ViziblesArduino::webSocketClient;									/*!< Main websocket client.*/
int ViziblesArduino::lastWebsocketRead = 0; 										/*!< Last websocket read try time.*/
pendingAck_t ViziblesArduino::pendingAcks[MAX_PENDING_ACKS];						/*!< Array of pending message acknowledgements.*/
unsigned int ViziblesArduino::lastSequenceNumber;									/*!< Last message sequence number used.*/
#endif /*VZ_WEBSOCKETS*/
#ifdef VZ_HTTP
int ViziblesArduino::pingRetries;													/*!< Number of ping retries still left.*/
#endif /*VZ_HTTP*/

/** 
 *	@brief ViziblesArduino class constructor.
 */
ViziblesArduino::ViziblesArduino(Client& client, Client& client1) {
	mainClient = &client;
	httpClient = new HttpClient(client1);
#ifdef VZ_HTTP_SERVER
	httpService = new WebApp();
	instance = this;
#endif /*VZ_HTTP_SERVER*/
#ifdef VZ_WEBSOCKETS
	lastSequenceNumber = 0;
	for(int i = 0; i < MAX_PENDING_ACKS; i++) pendingAcks[i].pending = 0;
#endif
	initializeOptions();
}
#ifdef VZ_HTTP_SERVER
/** 
 *	@brief Create random alfanumeric value.
 * 	
 *	Create a random string to init the process for a user to capture the ownership of a thing.
 * 
 *	@return nothing
 */
void ViziblesArduino::getConfigId(
				       char *buffer /*!< [out] Pointer to the buffer where config ID will be created (40 bytes long plus null terminator = 41 bytes).*/) {	
	LOGLN(F("getConfigId()"));	
	randomSeed(second() + getMACSeed());
	unsigned int max = strlen(alfanumericValues);
	for (int i=0; i<40; i++) {
		buffer[i] = pgm_read_byte(alfanumericValues+random(max));
	}
	buffer[40] = '\0';
	LOG(F("getConfigId(): ConfigId="));
	LOGLN(buffer);
}	
#endif /*VZ_HTTP_SERVER*/
	
/** 
 *	@brief Syncronize time.
 * 	
 *	Function to get the current time of the server and synchronize the
 *	internal clock to the same value. Used only for security purposes. 
 *  Real time synchronization is not good enough, since network propagation
 *	delay is not taken into account.
 * 
 *	@return zero if everyting is ok or a negative error code if not
 */
int ViziblesArduino::syncTime(void) {
	//TODO implement signed version or use a more extended method for time synchronization
	int err = 0;
	int pathLen = options.apiBasePath==NULL?strlen_P(timeUrl):strlen(options.apiBasePath)+strlen_P(timeUrl);
	char path[pathLen];
	if(options.apiBasePath!=NULL) strcpy(path, options.apiBasePath);
	strcpy_P(&path[options.apiBasePath==NULL?0:strlen(options.apiBasePath)],timeUrl);
	char response[22];
	if (!(err = HTTPRequest(httpClient, options.hostname, !strcmp_P(options.protocol, optionsProtocolWss)?DEFAULT_VIZIBLES_HTTP_PORT:options.port, path, HTTP_METHOD_GET, NULL, NULL, NULL, response, 22))) {
		if (strlen(response)==21) {
			response[19] = '\0';
			unsigned long now = atol(&response[9]);
			setTime(now);
			lastSync = now;
		} else {
			ERRLN(F("[VSC] syncTime() error: Incorrect server answer"));
			return (VIZIBLES_ERROR_SYNCHRONIZING);
		}
	} else {
		ERRLN(F("[VSC] syncTime() error: HTTPRequest returned error"));
		return err;
	}	
	LOGLN(F("syncTime(): Time synchronized OK"));
	return 0;
}

/** 
 *	@brief Test connection status.
 * 	
 *	Function to send a basic packet to de platform, with no information at all
 *  and wait for a response to check if the connection is still available.
 * 
 *	@return zero if everyting is ok or a negative error code if not
 */
int ViziblesArduino::sendPing (void) {
	LOGLN(F("sendPing()"));	
	keyValuePair payload[] = { {NULL, NULL} };
	return sendObject("ping", payload);
};

/** 
 *	@brief Confirm connection status is OK.
 * 	
 *	This internal function is used by all communication functions to confirm
 *	they tried to connect to the Vizibles platform and they got a response, 
 *	so the connection can be considered available.
 * 
 *	@return nothing
 */
void ViziblesArduino::confirmConnectionStatusOK(void) {
	//LOGLN(F("[VSC] confirmConnectionStatusOK()"));	
#ifdef VZ_HTTP
	if (!strcmp_P(options.protocol, optionsProtocolHttp)) pingRetries = options.pingRetries;
#endif	
	lastPing = millis();
}

/** 
 *	@brief Confirm connection status is ERROR.
 * 	
 *	This internal function is used by all communication functions to confirm
 *	they tried to connect to the Vizibles platform and they did NOT get a response, 
 *	so the connection can be considered unavailable.
 * 
 *	@return nothing
 */
void ViziblesArduino::confirmConnectionStatusERROR(void) {
	ERRLN(F("[VSC] confirmConnectionStatusERROR()"));
	if (cloudConnected && options.onDisconnectCallback != NULL) options.onDisconnectCallback(); 
	cloudConnected = 0;
#ifdef VZ_WEBSOCKETS
	invalidatePendingAcks();
#endif
	lastPing = millis();
}
#ifdef VZ_HTTP_SERVER
/** 
 *	@brief Non-member function for config commands.
 * 	
 *	This function allows calling config commands executer member function 
 *	from outside the ViziblesArduino class.
 * 
 *	@return nothing
 */
void cmdConfig(
	       Request &req,	/*!< [in] HTTP request.*/ 
	       Response &res	/*!< [in] HTTP response.*/) {
	LOGLN(F("cmdConfig()"));	
	if (instance!=NULL) instance->cmdConfig_(req, res);
}

/** 
 *	@brief Config commands exec.
 * 	
 *	This function handles the calls to configure parameters from the
 *	platform, mobile apps or other things in the same network.
 * 
 *	@return nothing
 */
void ViziblesArduino::cmdConfig_(
				      Request &req,	/*!< [in] HTTP request.*/ 
				      Response &res	/*!< [in] HTTP response.*/) {
	LOGLN(F("cmdConfig_()"));	
	char *f = req.route(1);
	if (!strcmp(f, "wifi")) {
		//char configId[41];
		getConfigId(pendingCfId);
		//Read post body
		int i = 0;
		while (!req.available() && i<100) {
			//Sometimes payload takes longer to be received 
			delay(10);
			i++;
		}	
		unsigned int bodyLen = req.available();
		char body[bodyLen+1];
		i = 0;
		while (i<bodyLen) body[i++] = req.read();
		body[bodyLen] = '\0';

		//Testing JSON parsing library
		StaticJsonBuffer<NORMAL_JSON_PARSE_BUFFER> jsonBuffer;
		
		JsonObject& root = jsonBuffer.parseObject((char*)body);
		
		if (!root.success()) {
			ERRLN(F("[VSC] parseObject() failed"));
			res.fail();
			return;
		}
		//Send answer to the client
		char configResponse[58];
		strcpy_P(configResponse, configWifiResponse);
		strcpy(&configResponse[14], pendingCfId);
		strcpy(&configResponse[54], "'}");
		convertFlashStringToMemString(contentType, ct);
		res.success(ct);
		res.write((uint8_t*) configResponse, (size_t)strlen(configResponse));
		
		//delay(1000);
		
		//Perform connection to the requested WiFi network
		char* ssid = (char *)root["ssid"].asString();
		strcpy(pendingSsid, ssid);
		char* key = (char *)root["key"].asString();
		strcpy(pendingPass, key);

		//root["key"].asString().printTo(pendingPass, sizeof(pendingPass));
		//root["ssid"].asString().printTo(pendingSsid, sizeof(pendingSsid));
		
		LOGLN(F("Configuring WiFi network"));
		LOG(F("SSID="));
		LOGLN(pendingSsid);
		LOG(F("password="));
		LOGLN(pendingPass);

		LOGLN(configResponse);
		
		pendingWifiConfig = 3;
		/*

		
		  if (!connectToWiFiAP(ssid, key, key[0]=='\0'? OPEN:WPA2)) {
		  LOGLN("Connected to WiFi");
		  disableWiFiAp();
		  if (connectToVizibles()) {
		  LOGLN(F("Connection to Vizibles failed."));
		  enableWiFiAp();
		  } else {
		  LOGLN(F("Connection to Vizibles established OK"));
		  keyValuePair values[3];
		  values[0].name = "wifiApplied";
		  values[0].value = configId;
		  values[1].name = "capture";
		  values[1].value = "true";
		  values[2].name = NULL;
		  values[2].value = NULL;
		  sendConfig(values); 
		  }	
		  lastConnection = millis();
		  } else {
		  LOGLN("Connection to WiFi failed");
		  }
		*/
	}
}

void ViziblesArduino::configureWiFi(void) {
	switch(pendingWifiConfig) {
	case 0:
	case 1:
		return;
	case 3:
		if (!connectToWiFiAP(pendingSsid, pendingPass, pendingPass[0]=='\0'? OPEN:WPA2)) {
			LOGLN("Connected to WiFi");
			disableWiFiAp();
			pendingWifiConfig = 2;
			lastConnection = millis();
		} else {
			pendingWifiConfig = 0;
			ERRLN(F("[VSC] Connection to WiFi failed"));
		}
		return;
	case 2:
		if (connectToVizibles()) {
			ERRLN(F("[VSC] Connection to Vizibles failed."));
			enableWiFiAp();
			pendingWifiConfig = 1;
		} else {
			LOGLN(F("Connection to Vizibles established OK"));
			if (options.protocol != NULL) {
				int ws = strcmp_P(options.protocol, optionsProtocolHttp);
				if (!ws) {
					keyValuePair values[3];
					convertFlashStringToMemString(wifiApplied, _wifiApplied);					
					convertFlashStringToMemString(wifiCapture, _capture);					
					convertFlashStringToMemString(wifiTrue, _true);					
					values[0].name = _wifiApplied;
					values[0].value = pendingCfId;
					values[1].name = _capture;
					values[1].value = _true;
					values[2].name = NULL;
					values[2].value = NULL;
					sendConfig(values);
					pendingWifiConfig = 0;
				} else pendingWifiConfig = 1;	
			}
		}	
		return;
	}	
}

#ifdef VZ_HTTP
/** 
 *	@brief Non-member function for new local commands.
 * 	
 *	This function allows calling new local commands executer member function 
 *	from outside the ViziblesArduino class.
 * 
 *	@return nothing
 */
void cmdNewLocal(
		 Request &req,	/*!< [in] HTTP request.*/ 
		 Response &res	/*!< [in] HTTP response.*/) {
	LOGLN(F("cmdNewLocal()"));	
	if (instance!=NULL) instance->cmdNewLocal_(req, res);
}

/** 
 *	@brief New local HTTP commands exec.
 * 	
 *	This function handles the calls to modify the behavior of the thing
 *	from the platform, mobile apps or other things in the same network.
 * 
 *	@return nothing
 */
void ViziblesArduino::cmdNewLocal_(
					Request &req,	/*!< [in] HTTP request.*/ 
					Response &res	/*!< [in] HTTP response.*/) {
	LOGLN(F("cmdNewLocal_()"));	
	//Read post body
	int i = 0;
	while (!req.available() && i<100) {
		//Sometimes payload takes longer to be received 
		delay(10);
		i++;
	}	
	unsigned int bodyLen = req.available();
	char body[bodyLen+1];
	i = 0;
	while (i<bodyLen) body[i++] = req.read();
	body[bodyLen] = '\0';
	//Check if request is authorized
	if (!isHTTPAuthorized(req, body)) {
		res.unauthorized();
		return;
	}
	confirmConnectionStatusOK();
	StaticJsonBuffer<BIG_JSON_PARSE_BUFFER> jsonBuffer;
	JsonObject& root = jsonBuffer.parseObject((char*)body);
	if (!root.success()) {
		ERRLN(F("[VSC] parseObject() failed"));
		res.fail();
		return;
	}
	int error = parseNewLocal(&root);
	if (error) { 	
		res.fail();
	} else {
		convertFlashStringToMemString(contentType, ct);
		res.success(ct);
		res.printP(newLocalResponseOk);
	}	
	return;
}
#endif /*VZ_HTTP*/
#endif /*VZ_HTTP_SERVER*/

/** 
 *	@brief Initialize options values.
 * 	
 *	Set all options to their initial value.
 * 
 *	@return nothing.
 */
void ViziblesArduino::initializeOptions(void) {
	options.index = 0;
	options.protocol = NULL;
	options.hostname = NULL;
	options.port = 0;
	options.pingDelay = 0;
	options.pingRetries = 0;
	//options.path = NULL;
	//options.credentials = NULL;
	//options.serverPort = 0;
	options.id = NULL;
	options.keyID = NULL;
	options.keySecret = NULL;
	options.thingId = NULL;
	options.ackTimeout = 0;
	options.onConnectCallback = NULL;
	options.onDisconnectCallback = NULL;
	options.apiBasePath = NULL;
	options.type = NULL;
}	

/** 
 *	@brief Set a value for an option.
 * 	
 *	Set a value in the options structure. The function includes a mechanism
 *	to check if the option was already set previously and if so delete the
 *	previous value and store the new in the char array asociated with the
 *	memory storage of options.
 * 
 *	@return zero if everyting is ok or a negative error code if not
 */
int ViziblesArduino::setOption(
				    char *value, 		/*!< [in] Value to store.*/
				    char**copyValueTo	/*!< [in] Pointer to the variable containing the pointer to the option storage space.*/) {
	LOG(F("setOption(\""));
#ifdef VZ_CLOUD_DEBUG
	LOG(value);
	LOG(F("\", "));
	if (copyValueTo == &options.protocol) LOG(F("&options.protocol"));
	else if (copyValueTo == &options.hostname) LOG(F("&options.hostname"));
	else if (copyValueTo == &options.id) LOG(F("&options.id"));
	else if (copyValueTo == &options.keyID) LOG(F("&options.keyID"));
	else if (copyValueTo == &options.keySecret) LOG(F("&options.keySecret"));
	else if (copyValueTo == &options.thingId) LOG(F("&options.thingId"));
	else if (copyValueTo == &options.type) LOG(F("&options.type"));
	else if (copyValueTo == &options.apiBasePath) LOG(F("&options.apiBasePath"));
	else ERR(F("UNKNOWN OPTION"));
#endif	
	LOGLN(F(")"));
	if (*copyValueTo!=NULL) {
		if (!strcmp(value, *copyValueTo)) {
			LOGLN(F("  Option already stored. Ignoring update"));
			return 0;
		}	
		else { //Delete previous content
			LOGLN(F("  Deleting previously stored data for the same option"));
			char *startScan = *copyValueTo + strlen(*copyValueTo) + 1;
			unsigned int length = strlen(*copyValueTo)+1;
			char *k = startScan; 
			for(char *i = *copyValueTo; i <= *copyValueTo + length; i++) {
				*(k++)=*i;
			}
			options.index-=length;
			if (options.protocol!=NULL && options.protocol>=startScan) options.protocol -= length;
			if (options.hostname!=NULL && options.hostname>=startScan) options.hostname -= length;
			if (options.id!=NULL && options.id>=startScan) options.id -= length;
			if (options.keyID!=NULL && options.keyID>=startScan) options.keyID -= length;
			if (options.keySecret!=NULL && options.keySecret>=startScan) options.keySecret -= length;
			if (options.thingId!=NULL && options.thingId>=startScan) options.thingId -= length;
			if (options.type!=NULL && options.type>=startScan) options.type -= length;
			if (options.apiBasePath!=NULL && options.apiBasePath>=startScan) options.apiBasePath -= length;
		}
	}
	if (options.index+strlen(value)<OPTIONS_BUFFER_LENGTH) {
		strcpy(&options.stringBuffer[options.index], value);
		*copyValueTo = &options.stringBuffer[options.index];
		options.index+=strlen(value)+1;
		LOGLN(F("setOption(): Option stored OK"));
		return 0;
	} else {
		ERRLN(F("setOption() error: No space available to store option"));
		return -1;
	}	
}

/** 
 *	@brief Parse a string with an option.
 * 	
 *	Check if the key name string is the same as the option name provided and if so
 *	store the value string in the corresponding option.
 * 
 *	@return zero if everyting is ok or a negative error code if not
 */
int ViziblesArduino::parseOption (
				       char *key,			/*!< [in] Name of the option.*/
				       char *value,		/*!< [in] Value to store.*/
				       char *search,		/*!< [in] String to compare with the name of the option.*/
				       char **copyValueTo	/*!< [in] Pointer to the variable containing the pointer to the option storage space.*/) {
	/*	LOG(F("parseOption(\""));	
		#ifdef VZ_CLOUD_DEBUG
		LOG(key);
		LOG(F("\", \""));
		LOG(value);
		LOG(F("\", \""));
		LOG(search);
		LOG(F("\", "));
		if (copyValueTo == &options.protocol) LOG(F("&options.protocol"));
		else if (copyValueTo == &options.hostname) LOG(F("&options.hostname"));
		else if (copyValueTo == &options.id) LOG(F("&options.id"));
		else if (copyValueTo == &options.keyID) LOG(F("&options.keyID"));
		else if (copyValueTo == &options.keySecret) LOG(F("&options.keySecret"));
		else if (copyValueTo == &options.thingId) LOG(F("&options.thingId"));
		else if (copyValueTo == &options.type) LOG(F("&options.type"));
		else if (copyValueTo == &options.apiBasePath) LOG(F("&apiBasePath"));
		else LOG(F("UNKNOWN OPTION"));
		#endif	
		LOGLN(F(")"));*/
	if (!strcmp(key, search)) {
		if (setOption(value, copyValueTo)) {
			ERRLN(F("parseOption() error: call to setOption() returned with error"));	
			return -1;
		}
		LOGLN(F("parseOption(): Option stored OK"));	
		return 0;
	} else {
		//ERRLN(F("parseOption(): Option does not fit on provided search name"));
		return -1;
	}	
}	

/** 
 *	@brief Set default value for an option.
 * 	
 *	Set an option to a fixed value provided as a string.
 * 
 *	@return zero if everyting is ok or a negative error code if not
 */
int ViziblesArduino::setDefaultOption (
					    char *value, 		/*!< [in] Value to store.*/
					    char **copyValueTo	/*!< [in] Pointer to the variable containing the pointer to the option storage space.*/) {
	LOG(F("setDefaultOption(\""));	
#ifdef VZ_CLOUD_DEBUG
	LOG(value);
	LOG(F("\", "));
	if (copyValueTo == &options.protocol) LOG(F("&options.protocol"));
	else if (copyValueTo == &options.hostname) LOG(F("&options.hostname"));
	else if (copyValueTo == &options.id) LOG(F("&options.id"));
	else if (copyValueTo == &options.keyID) LOG(F("&options.keyID"));
	else if (copyValueTo == &options.keySecret) LOG(F("&options.keySecret"));
	else if (copyValueTo == &options.thingId) LOG(F("&options.thingId"));
	else if (copyValueTo == &options.type) LOG(F("&options.type"));
	else if (copyValueTo == &options.apiBasePath) LOG(F("&apiBasePath"));
	else ERR(F("UNKNOWN OPTION"));
#endif	
	LOGLN(F(")"));
	if (setOption(value, copyValueTo)) {
		ERRLN(F("setDefaultOption() error: call to setOption() returned with error"));
		return -1;
	} else {
		LOGLN(F("setDefaultOption(): Option stored OK"));
		return 0;	
	}	
}	

/** 
 *	@brief Set a value for an option.
 * 	
 *	Check if option name is among the known options and if so set the value in the options structure.
 * 
 *	@return nothing
 */
void ViziblesArduino::option(
				  char *name,	/*!< [in] Name of the option.*/
				  char *value	/*!< [in] Value of the option.*/) {
	LOG(F("option(\""));
	LOG(name);
	LOG(F("\", \""));
	LOG(value);
	LOGLN(F("\")"));
	if (!strcmp(name, "port")) {
		options.port = atoi(value);
	} else if (!strcmp(name, "pingDelay")) {
		options.pingDelay = 1000 * atof(value);
	} else if (!strcmp(name, "pingRetries")) {
		options.pingRetries = atoi(value);
	} else if (!strcmp(name, "ackTimeout")) {
		options.ackTimeout =  1000 * atof(value);
	} else
		//if (!strcmp(name, "serverPort")) {
		//	options.serverPort = atoi(value);
		//} else
		if (parseOption (name, value, (const __FlashStringHelper *) optionsProtocol, &options.protocol)==-1)
			if (parseOption (name, value, (const __FlashStringHelper *) optionsHostname, &options.hostname)==-1)
				//if (parseOption (name, value, (const __FlashStringHelper *) optionsCredentials, &options.credentials)==-1)
				if (parseOption (name, value, (const __FlashStringHelper *) optionsId, &options.id)==-1)
					if (parseOption (name, value, (const __FlashStringHelper *) optionsKeyId, &options.keyID)==-1)
						if (parseOption (name, value, (const __FlashStringHelper *) optionsKeySecret, &options.keySecret)==-1)
							if (parseOption (name, value, (const __FlashStringHelper *) optionsType, &options.type)==-1)
								if (parseOption (name, value, (const __FlashStringHelper *) optionsApiBase, &options.apiBasePath)==-1);
}

/** 
 *	@brief Disconnect from the platform.
 * 	
 *	Function to nicely disconnect from the Vizibles platform.
 * 
 *	@return zero if everything is ok or a negative error code if not
 */
int ViziblesArduino::disconnect(void) {
	tryToConnect = 0;
	if (!cloudConnected) return -1;
	int ws = strcmp_P(options.protocol, optionsProtocolHttp);
#ifdef VZ_WEBSOCKETS
	if (ws && cloudConnected) {
		cloudConnected = 0;
		if (options.onDisconnectCallback != NULL) options.onDisconnectCallback();
		webSocketClient.disconnect();
	}	
#endif /*VZ_WEBSOCKETS*/ 
#ifdef VZ_HTTP
	if (!ws && cloudConnected) {
		cloudConnected = 0;
		if (options.onDisconnectCallback != NULL) options.onDisconnectCallback();
		//There is no HTTP message for disconnecting. Just do not send more messages and wait for server timeout 
	}	
#endif /*VZ_WEBSOCKETS*/ 
	return 0;
} 
 
/** 
 *	@brief Connect to the platform.
 * 	
 *	Function to stablish initial parameters and test connection with the Vizibles platform.
 *	It requires at least the credentials of the device to access the platform (key-ID and 
 *	secret API-key) and the identifier of the device instance.
 * 
 *	@return zero if everyting is ok or a negative error code if not
 */
int ViziblesArduino::connect (
				   keyValuePair values[],				/*!< [in] Array of key-value pairs with configuration options, terminated with NULL-named pair.*/
				   cloudCallback onConnectCallback,	/*!< [in] Pointer to function to be called when connected to vizibles (optional).*/
				   cloudCallback onDisconnectCallback /*!< [in] Pointer to function to be called when disconnected from vizibles (optional).*/) {
#ifdef ESP8266
	ESP.wdtFeed();
#endif		
	LOG(F("connect("));
#ifdef VZ_CLOUD_DEBUG
	if (values!=NULL) {
		LOG(F("values={"));
		int z = 0;
		while (values[z].name!=NULL) {
			if (z>0) LOG(F(", "));
			LOG(F("{\""));
			LOG(values[z].name);
			LOG(F("\", \""));
			LOG(values[z].value);
			LOG(F("\"}"));
			z++;
		}	
		LOG(F("}"));
	} else {
		LOG(F("NULL, "));
	}
	if (onConnectCallback!=NULL) LOG(F(", &onConnectCallback"));
	else LOG(F(", NULL"));
	if (onDisconnectCallback!=NULL) LOG(F(", &onDisconnectCallback"));
	else LOG(F(", NULL"));
#endif
	LOGLN(F(")"));
	tryToConnect = 1;
	//if (values!=NULL) initializeOptions(); 
	options.onDisconnectCallback = onDisconnectCallback; 
	options.onConnectCallback = onConnectCallback;
	if (values!=NULL) {
		//Parse and store options
		int i = 0;
		while (values[i].name!=NULL) {
			option(values[i].name, values[i].value);
			i++;
		}
	}	
	//Check all mandatory options are set
	if (options.id==NULL || options.keySecret==NULL || options.keyID==NULL) return VIZIBLES_ERROR_INCORRECT_OPTIONS;

	//Use default options if not specified
#if defined(VZ_WEBSOCKETS) && !defined(VZ_HTTP)
	if (options.protocol==NULL || (strcmp_P(options.protocol, optionsProtocolWss) && strcmp_P(options.protocol, optionsProtocolWs))) setDefaultOption ((const __FlashStringHelper *)optionsProtocolWs, &options.protocol);
#endif
#if !defined(VZ_WEBSOCKETS) && defined(VZ_HTTP)
	if (options.protocol==NULL || strcmp_P(options.protocol, optionsProtocolHttp)) setDefaultOption ((const __FlashStringHelper *)optionsProtocolHttp, &options.protocol);
#endif
#if defined(VZ_WEBSOCKETS) && defined(VZ_HTTP)
	if (options.protocol==NULL || (strcmp_P(options.protocol, optionsProtocolWss) && strcmp_P(options.protocol, optionsProtocolWs) && strcmp_P(options.protocol, optionsProtocolHttp))) setDefaultOption ((const __FlashStringHelper *)optionsProtocolWs, &options.protocol);
#endif
#ifdef VZ_WEBSOCKETS
	if (options.port==0) {
		if (!strcmp_P(options.protocol, optionsProtocolWss)) options.port = DEFAULT_VIZIBLES_HTTPS_PORT;
		else options.port = DEFAULT_VIZIBLES_HTTP_PORT;
	}
#else
	if (options.port==0) options.port = DEFAULT_VIZIBLES_HTTP_PORT;
#endif /*VZ_WEBSOCKETS*/
	if (options.hostname==NULL) setDefaultOption ((const __FlashStringHelper *)viziblesUrl, &options.hostname);	
	////if (options.hostname==NULL) setDefaultOption ((const __FlashStringHelper *)optionsHostLocalhost, &options.hostname);
	//if (options.hostname==NULL) setDefaultOption ("192.168.0.18", &options.hostname);
	
	//if (options.path==NULL) setDefaultOption ("/thing", &options.path);
	if (options.pingDelay==0) options.pingDelay = 20000;
	if (options.pingRetries==0) options.pingRetries = 3;
	if (options.ackTimeout==0) options.ackTimeout = 10000;
	//if (options.serverPort==0) options.serverPort = 3000;
#ifdef VZ_HTTP_SERVER
	listen();
#endif /*VZ_HTTP_SERVER*/
	confirmConnectionStatusOK(); //Initialize connection status algorithm even if we did not try to connect yet	
	if (!isWiFiConnected()) {
		//enableWiFiAp();
		//disconnectFromWiFiAp();
		tryToConnectToLastAP(true);
		ERRLN(F("[VSC] connect() error: Impossible to connect to Vizibles cloud. WiFi not connected"));
		return -1;
	} else return connectToVizibles();
}

/** 
 *	@brief Connect to the Vizibles platform.
 * 	
 *	Function that actually performs the communication with the platform. It can be called 
 *	initially from connect() or later on if the connection is lost to try to reconnect.
 * 
 *	@return zero if everyting is ok or a negative error code if not
 */ 
int ViziblesArduino::connectToVizibles (void) {
	LOGLN(F("[VSC] connectToVizibles()"));	
	//Syncronize time with the server if this is the first time or more than one day passed from last synchronization
	if (!strcmp_P(options.protocol, optionsProtocolWs) || (strcmp_P(options.protocol, optionsProtocolWss) && strcmp_P(options.protocol, optionsProtocolWs))) {
		if (lastSync==0 || (lastSync + SYNCHRONIZATION_PERIOD < now())) {
			int err = syncTime();
			if (err) return err;
		}
	}	
#ifdef VZ_WEBSOCKETS
	// Connect to the websocket server
	if (!strcmp_P(options.protocol, optionsProtocolWss) || !strcmp_P(options.protocol, optionsProtocolWs)) {
		LOG(F("connectToVizibles(): (mainClient)->connect("));
		LOG(options.hostname);
		LOG(F(", "));
		LOG(options.port);
		LOGLN(F(")"));
		if ((mainClient)->connect(options.hostname, options.port)) {
			LOGLN(F("connectToVizibles(): Socket opened, preparing headers for websocket"));
			int pathLen = options.apiBasePath==NULL?7:strlen(options.apiBasePath)+7;
			char path[pathLen];
			if(options.apiBasePath!=NULL) strcpy(path, options.apiBasePath);
			strcpy(&path[options.apiBasePath==NULL?0:strlen(options.apiBasePath)],"/thing");
			webSocketClient.path=path;
			//Create authorization headers
			unsigned int keyIDLen = strlen(options.keyID);
			unsigned int idLen = strlen(options.id);
			unsigned int typeIdLen = 0;
			if (options.type!=NULL) typeIdLen = strlen(options.type) + 1; 
			char headerAuthorization[93+keyIDLen+idLen+typeIdLen];
			if (!strcmp_P(options.protocol, optionsProtocolWss)) {
				LOGLN(F("connectToVizibles(): Creating authorization headers for secure websocket"));
				strcpy_P(headerAuthorization, authorization);
				strcpy(&headerAuthorization[24], options.keyID);
				headerAuthorization[36] = ':';    	
				strcpy(&headerAuthorization[37], options.keySecret);
				headerAuthorization[57] = ':';
				strcpy(&headerAuthorization[58], options.id);
				if (options.type!=NULL) {
					headerAuthorization[58+idLen] = ':';
					strcpy(&headerAuthorization[59+idLen], options.id);
				}
			} else {
				LOGLN(F("connectToVizibles(): Creating authorization headers for normal websocket"));
				strcpy_P(&headerAuthorization[28 + keyIDLen + HMAC_SHA1_HASH_LENGTH_CODE64 + idLen + typeIdLen], date);
				getDateString(&headerAuthorization[37 + keyIDLen + HMAC_SHA1_HASH_LENGTH_CODE64 + idLen + typeIdLen]);
				strcpy_P(headerAuthorization, authorization);

				strcpy(&headerAuthorization[24], options.keyID);
				headerAuthorization[24 + keyIDLen] = ':';    	
				strcpy(&headerAuthorization[27 + keyIDLen + HMAC_SHA1_HASH_LENGTH_CODE64], options.id);
				if (options.type!=NULL) {
					headerAuthorization[27 + keyIDLen + HMAC_SHA1_HASH_LENGTH_CODE64 + idLen] = ':';
					strcpy(&headerAuthorization[28 + keyIDLen + HMAC_SHA1_HASH_LENGTH_CODE64 + idLen], options.type);
				}
				convertFlashStringToMemString(optionsProtocolWs, _ws);
				createHashSignature(&headerAuthorization[24 + keyIDLen + 1], options.keySecret, NULL, _ws, options.hostname, options.port, path, NULL, &headerAuthorization[27 + keyIDLen + HMAC_SHA1_HASH_LENGTH_CODE64], &headerAuthorization[37 + keyIDLen + HMAC_SHA1_HASH_LENGTH_CODE64 + idLen + typeIdLen], NULL); 
				headerAuthorization[25 + keyIDLen + HMAC_SHA1_HASH_LENGTH_CODE64 + 1] = ':';
				headerAuthorization[27 + keyIDLen + HMAC_SHA1_HASH_LENGTH_CODE64 + idLen + typeIdLen] = '\n';
			}	
			// Handshake with the server
			char host[strlen(options.hostname) + 7];
			strcpy(host, options.hostname);
			host[strlen(options.hostname)] = ':';
			itoa(options.port, &host[strlen(options.hostname)+1], 10);
			webSocketClient.host = host;
			webSocketClient.headers = headerAuthorization;
			LOGLN(F("connectToVizibles(): Creating websocket"));
			if (webSocketClient.handshake(*mainClient)) {
				cloudConnected = 1;
			} else {
				ERRLN(F("[VSC] connectToVizibles() error: Websocket creation failed."));
				return -1;
			}
		} else {
			ERRLN(F("[VSC] connectToVizibles() error: Socket connection failed."));
			
			return -1;
		}
	}	
#endif /*VZ_WEBSOCKETS*/
#ifdef VZ_HTTP
	//Inform the platform thing is connected and get thing ID
	if (!strcmp_P(options.protocol, optionsProtocolHttp)) {
		LOGLN(F("connectToVizibles(): Trying socket connection"));
		keyValuePair empty[] = { {NULL, NULL} };
		char buf[79];
		if (!this->sendMe(empty, buf, 79)) {
			cloudConnected = 1;
			if (options.onConnectCallback != NULL) options.onConnectCallback();
		}
		StaticJsonBuffer<NORMAL_JSON_PARSE_BUFFER> jsonBuffer;
		JsonObject& root = jsonBuffer.parseObject(buf);
		convertFlashStringToMemString(ccThingId, _ccThingId);
		if (!root.success() || !root.containsKey((const char*)_ccThingId) || !root[(const char*)_ccThingId].is<const char*>()) return VIZIBLES_ERROR_CONNECTING;
		setDefaultOption ((char *)root[(const char*)_ccThingId].asString(), &options.thingId);
		/*		if (pendingWifiConfig==1) {
				keyValuePair values[3];
				convertFlashStringToMemString(wifiApplied, _wifiApplied);					
				convertFlashStringToMemString(wifiCapture, _capture);					
				convertFlashStringToMemString(wifiTrue, _true);					
				values[0].name = _wifiApplied;
				values[0].value = pendingCfId;
				values[1].name = _capture;
				values[1].value = _true;
				values[2].name = NULL;
				values[2].value = NULL;
				sendConfig(values);
				pendingWifiConfig = 0;
				}*/
	}
#endif /*VZ_HTTP*/

#ifdef VZ_ITTT_RULES
	initITTT(httpClient, options.thingId);
#endif
	//delay(500);
	//Get LAN information and send it to the cloud 
	keyValuePair *lanInfo;
	char clientIp[16], gatewayIp[16], clientMac[18], gatewayMac[18];
	if (getGatewayMac(gatewayMac)!=NULL) {
		getGatewayIp(gatewayIp);
		getLocalIp(clientIp);
		getLocalMac(clientMac);
		keyValuePair lanInfo[] = { { "ip", clientIp }, { "mac", clientMac}, { "gateway", gatewayIp }, { "LAN", gatewayMac}, {NULL, NULL} };
		sendLocal(lanInfo); 
	};
}

/** 
 *	@brief Send JSON object.
 * 	
 *	Function to send sensor values to the platform. It requires just
 *  a list of key-value pairs with the information to be send.
 * 
 *	@return zero if everyting is ok or a negative error code if not
 */
int ViziblesArduino::sendObject (
				      char *prim,						/*!< [in] Primitive command (update, functions, local or result). */
				      keyValuePair payLoadValues[],	/*!< [in] Array of key-value pairs to send, terminated with NULL-named pair.*/
				      char *response,		 			/*!< [out] Pointer to buffer where response must be stored (optional).*/
				      unsigned int resLen, 			/*!< [in] Maximum buffer length for response(optional).*/
				      cloudCallback cb 				/*!< [in] Callback function to be executed in case of error.*/) {
#ifdef VZ_CLOUD_DEBUG
	LOG(F("[VSC] sendObject(\""));
	LOG(prim);
	LOG(F("\", "));
	LOG(F("{"));
	int z = 0;
	while (payLoadValues[z].name!=NULL) {
		if (z>0) LOG(F(", "));
		LOG(F("{\""));
		LOG(payLoadValues[z].name);
		LOG(F("\", \""));
		LOG(payLoadValues[z].value);
		LOG(F("\"}"));
		z++;
	}	
	LOG(F("}"));
	if (response!=NULL) LOG(F(", &response"));
	if (resLen!=0) {
		LOG(F(", "));
		LOG(resLen);
	}
	if (cb!=NULL) LOG(F(", &callback"));
	else LOG(F(", NULL"));
	LOGLN(F(")"));	
#endif
	StaticJsonBuffer<NORMAL_JSON_PARSE_BUFFER> jsonBuffer;
	JsonObject& root = jsonBuffer.createObject();
	int ws = strcmp_P(options.protocol, optionsProtocolHttp);
#ifdef VZ_WEBSOCKETS
	char c[strlen(prim)+3];
	if (ws) {
		strcpy(c, "t:");
		strcpy(&c[2], prim);
		root.set("c",(const char*)c);
		root.createNestedObject("p");
		root["n"] = lastSequenceNumber;
	} 	
#endif /*VZ_WEBSOCKETS*/
#if defined(VZ_WEBSOCKETS) && !defined(VZ_HTTP)
	JsonObject& p = root[(const char*)"p"];
#endif
#if !defined(VZ_WEBSOCKETS) && defined(VZ_HTTP)
	JsonObject& p = root;
#endif
#if defined(VZ_WEBSOCKETS) && defined(VZ_HTTP)
	JsonObject& p = ws?root[(const char*)"p"]:root;
#endif
	int i = 0;	
	while (payLoadValues[i].name) {
#ifdef VZ_ITTT_RULES
		if (!strcmp_P(payLoadValues[i].name, itttMetaKey)) {
			JsonObject& rootMeta = p.createNestedObject(payLoadValues[i].name);
			p[payLoadValues[i].name] = jsonBuffer.parseObject(payLoadValues[i].value);
		} else 
#endif /*VZ_ITTT_RULES*/
			if (!strcmp(payLoadValues[i].value, "true")) p[payLoadValues[i].name] = true;
			else if (!strcmp(payLoadValues[i].value, "false")) p[payLoadValues[i].name] = false;
			else if (isNumber(payLoadValues[i].value)){
				//Serial.println(isNotInteger(payLoadValues[i].value));
				if (!isInteger(payLoadValues[i].value)) p[payLoadValues[i].name] = atof(payLoadValues[i].value);
				else p[payLoadValues[i].name] = atoi(payLoadValues[i].value);
			} 
			else p[payLoadValues[i].name] = payLoadValues[i].value;
		i++;
	}
	size_t size = root.measureLength() + 1;
	char payload[size];
	root.printTo(payload, size);
#ifdef VZ_WEBSOCKETS
	if (ws) {
		return WSSendData (payload, cb);
	}	
#endif /*VZ_WEBSOCKETS*/
#ifdef VZ_HTTP 
	if (!ws) {
		int err = HTTPSendData(prim, payload, response, resLen);
		if (err && cb!=NULL) cb();
		return err;
	}	
#endif /*VZ_HTTP*/
}

#ifdef VZ_EXECUTE_FUNCTIONS
/** 
 *	@brief Send JSON array.
 * 	
 *	Function to send sensor values to the platform. It requires just
 *  a list of key-value pairs with the information to be send.
 * 
 *	@return zero if everyting is ok or a negative error code if not
 */
int ViziblesArduino::sendArray (
				     char *prim,				/*!< [in] Primitive command (update, functions, local or result). */
				     char *payLoadValues[],	/*!< [in] Array of strings to send as an array, NULL terminated.*/
				     char *response,		 	/*!< [out] Pointer to buffer where response must be stored (optional).*/ 
				     unsigned int resLen,		/*!< [in] Maximum buffer length for response(optional).*/
				     cloudCallback cb 		/*!< [in] Callback function to be executed in case of error.*/) { 	
	LOG(F("sendArray(\""));
	LOG(prim);
	LOG(F("\", "));
#ifdef VZ_CLOUD_DEBUG
	LOG(F("{"));
	int z = 0;
	while (payLoadValues[z]!=NULL) {
		if (z>0) LOG(F(", "));
		LOG(F("\""));
		LOG(payLoadValues[z]);
		LOG(F("\""));
		z++;
	}	
	LOG(F("}"));
	if (response!=NULL) LOG(F(", &response"));
	if (resLen!=0) {
		LOG(F(", "));
		LOG(resLen);
	}	
	if (cb!=NULL) LOG(F(", &callback"));
	else LOG(F(", NULL"));
#endif
	LOGLN(F(")"));	
	StaticJsonBuffer<NORMAL_JSON_PARSE_BUFFER> jsonBuffer;
	int ws = strcmp_P(options.protocol, optionsProtocolHttp);
	JsonObject& root = jsonBuffer.createObject();
#ifdef VZ_WEBSOCKETS
	char c[strlen(prim)+3];
	if (ws) {
		strcpy(c, "t:");
		strcpy(&c[2], prim);
		root.set("c",(const char*)c);
		root.createNestedArray("p");
		root["n"] = (int)lastSequenceNumber;
	}	
#endif /*VZ_WEBSOCKETS*/	
#ifdef VZ_HTTP
	if (!ws) {
		root.createNestedArray("p");
	}	
#endif /*VZ_HTTP*/
	JsonArray& p = root["p"];
	int i = 0;	
	while (payLoadValues[i]) {
		p.add(payLoadValues[i]);
		i++;
	}
#ifdef VZ_WEBSOCKETS
	if (ws) {
		size_t size = root.measureLength() + 1;
		char payload[size];
		root.printTo(payload, size);
		return WSSendData (payload, cb);
	}	
#endif /*VZ_WEBSOCKETS*/
#ifdef VZ_HTTP 
	if (!ws) {
		size_t size = p.measureLength() + 1;
		char payload[size];
		p.printTo(payload, size);
		int err = HTTPSendData(prim, payload, response, resLen);
		if (err && cb!=NULL) cb();
		return err;
	}	
#endif /*VZ_HTTP*/
} 
#endif /*VZ_EXECUTE_FUNCTIONS*/

#ifdef VZ_WEBSOCKETS
/** 
 *	@brief Send data to the platform using Websockets. 
 * 	
 *	Function to send data to Vizibles platform through Websockets. It requires just
 *  a character array with the information to be send and an optional callabck to be 
 *	called if the transmission is not acknowledged 
 * 
 *	@return zero if everyting is ok or a negative error code if not
 */
int ViziblesArduino::WSSendData (
				      char *payload,		/*!< [in] Char array to be sent as payload.*/
				      cloudCallback cb 	/*!< [in] Callback function to be executed in case of error (optional).*/) { 
#ifdef VZ_CLOUD_DEBUG
	LOG(F("[VSC] WSSendData(\""));
	LOG(payload);
	LOG(F("\""));
	if (cb!=NULL) LOG(F(", &callback"));
	LOGLN(F(")"));	
#endif /*VZ_CLOUD_DEBUG*/
	if (!strcmp_P(options.protocol, optionsProtocolWs) || (strcmp_P(options.protocol, optionsProtocolWss) && strcmp_P(options.protocol, optionsProtocolWs))) {
		if (lastSync==0 || (lastSync + SYNCHRONIZATION_PERIOD < now())) {
			int err = syncTime();
			if (err) return err;
		}
	}	
	if (cloudConnected && webSocketClient.connected()) {
		setNewPendingAck(cb, lastSequenceNumber++);
		webSocketClient.sendData(payload, 1);
		return 0;
	} else {
		ERR(F("[VSC] WSSendData() error: Not connected to cloud."));
		ERR(F(" cloudConnected: "));
		ERR(cloudConnected);
		ERR(F(", webSocketClient.connected(): "));
		ERR(webSocketClient.connected());
		ERRLN();
		
		confirmConnectionStatusERROR();
		return -1;	
	}
}
#endif /*VZ_WEBSOCKETS*/
#ifdef VZ_HTTP 
/** 
 *	@brief Send data to the platform using HTTP. 
 * 	
 *	Function to send data to Vizibles platform through HTTP. It requires just
 *  a character array with the information to be send and the primitive 
 *	command to construct the path for the request.
 * 
 *	@return zero if everyting is ok or a negative error code if not
 */
int ViziblesArduino::HTTPSendData (
					char *prim,			/*!< [in] Primitive command (update, functions, local, result...). */
					char *payload,		/*!< [in] Char array to be sent as payload.*/
					char *response,		/*!< [out] Pointer to buffer where response must be stored (optional).*/
					unsigned int resLen /*!< [in] Maximum buffer length for response (optional).*/) { 
	LOG(F("[VSC] HTTPSendData(\""));
	LOG(prim);
	LOG(F("\", \""));
	LOG(payload);
	LOG(F("\""));
#ifdef VZ_CLOUD_DEBUG
	if (response!=NULL) LOG(F(", &response"));
	if (resLen!=0) {
		LOG(F(", "));
		LOG(resLen);
	}	
#endif /*VZ_CLOUD_DEBUG*/
	LOGLN(F(")"));	
	if (lastSync==0 || (lastSync + SYNCHRONIZATION_PERIOD < now())) {
		int err = syncTime();
		if (err) return err;
	}
	//Get url path
	convertFlashStringToMemString(Path, kPath);
	int pathLen = 0;
	pathLen = strlen_P(Path) + strlen(prim) + 1; 
	char path[pathLen];
	strcpy_P(path, Path);
	strcpy(&path[strlen_P(Path)], prim);
	//Create content type header
	convertFlashStringToMemString(contentType, headerContentType);
	//Create date header
	char headerDate[39];
	strcpy_P(headerDate, date);
	getDateString(&headerDate[9]); 
	//Create hash signature.
	unsigned int keyIDLen = strlen(options.keyID);
	unsigned int idLen = strlen(options.id);
	unsigned int typeIdLen = 0;
	if (options.type!=NULL) typeIdLen = strlen(options.type) + 1; 
	char headerAuthorization[56+keyIDLen+idLen+typeIdLen];
	strcpy_P(headerAuthorization, authorization);
	strcpy(&headerAuthorization[24], options.keyID);
	headerAuthorization[24 + keyIDLen] = ':';    	
	strcpy(&headerAuthorization[27 + keyIDLen + HMAC_SHA1_HASH_LENGTH_CODE64], options.id);
	if (options.type!=NULL) {
		headerAuthorization[27 + keyIDLen + HMAC_SHA1_HASH_LENGTH_CODE64 + idLen] = ':';
		strcpy(&headerAuthorization[28 + keyIDLen + HMAC_SHA1_HASH_LENGTH_CODE64 + idLen], options.type);
	}	
	convertFlashStringToMemString(optionsProtocolHttp, _http);
	createHashSignature(&headerAuthorization[24 + keyIDLen + 1], options.keySecret, "POST", _http, options.hostname, options.port, path, headerContentType, &headerAuthorization[27 + keyIDLen + HMAC_SHA1_HASH_LENGTH_CODE64], &headerDate[9], payload); 
	headerAuthorization[25 + keyIDLen + HMAC_SHA1_HASH_LENGTH_CODE64 + 1] = ':';
	char *headers[] = { headerDate, headerAuthorization, NULL };
	int err = HTTPRequest(httpClient, options.hostname, options.port, path, HTTP_METHOD_POST, headerContentType, payload, headers, response, resLen);
	if (!err) confirmConnectionStatusOK();
	else confirmConnectionStatusERROR();
	return err;
}
#endif /*VZ_HTTP*/
#ifdef VZ_HTTP_SERVER
/** 
 *	@brief Create ticket.
 * 	
 *	This function re-creates the ticket issued by Vizibles to other thing to send orders to this thing.
 * 
 *	@return nothing
 */
void ViziblesArduino::createTicket(
					char *buffer,		/*!< [out] Buffer to write the final ticket string (result has fixed length, 20 bytes. 21 with the \0 string terminator). */
					char *key,			/*!< [in] Key secret for the HMAC. */
					char *keyId,		/*!< [in] Key identifier of the key used to create the ticket. */
					char *id,		/*!< [in] Identifier of the thing receiving the ticket. */
					char *issuedTo		/*!< [in] Identifier of the thing sending the ticket. */ ){
	LOG(F("createTicket(&buffer, \""));
	LOG(key);
	LOG(F("\", \""));
	LOG(keyId);
	LOG(F("\", \""));
	LOG(id);
	LOG(F("\", \""));
	LOG(issuedTo);
	LOGLN(F("\")"));	
	Sha1.initHmac((uint8_t *)key, HMAC_SHA1_KEY_LENGTH);
	int len = strlen(id);
	for(int i = 0; i < len; i++) Sha1.write(id[i]);
	len = strlen(keyId);
	for(int i = 0; i < len; i++) Sha1.write(keyId[i]);
	len = strlen(issuedTo);
	for(int i = 0; i < len; i++) Sha1.write(issuedTo[i]);
	char hash[27];
	b64_encode(Sha1.resultHmac(), HMAC_SHA1_HASH_LENGTH_BINARY, (unsigned char *)hash, HMAC_SHA1_HASH_LENGTH_CODE64); 
	//base64_encode((char *)Sha1.resultHmac(), (char *)hash, HMAC_SHA1_HASH_LENGTH_BINARY); 
	hash[20] = '\0';
	strncpy(buffer,hash, 21); 
}
#endif /*VZ_HTTP_SERVER*/

#ifdef VZ_WEBSOCKETS
/** 
 *	@brief Set new message pending for confirmation.
 * 	
 *	Every time a message is sent through a websocket a note must be stored for notifying
 *	application if the message gets lost. For doing so we need to wait for an ACK message
 *	from the server.
 * 
 *	@return nothing
 */
void  ViziblesArduino::setNewPendingAck(
					     cloudCallback callback, /*!< [in] Function to call in case the message does not get confirmed.*/
					     unsigned int n 			/*!< [in] Sequence number of the message we are waiting for confirmation.*/) {
	LOG(F("[VSC] setNewPendingAck("));
#ifdef VZ_CLOUD_DEBUG
	if (callback==NULL) LOG(F("NULL, "));
	else LOG(F("&callback, "));
	LOG(n);
#endif /*VZ_CLOUD_DEBUG*/	
	LOGLN(F(")"));	
	int i;
	for(i = 0; i < MAX_PENDING_ACKS; i++) {
		if (!pendingAcks[i].pending) {
			pendingAcks[i].pending = 1;
			pendingAcks[i].callback = callback;
			pendingAcks[i].n = n;
			pendingAcks[i].time = millis();
			break;
		}
	}
#ifdef VZ_CLOUD_DEBUG
	if (i==MAX_PENDING_ACKS) ERRLN(F("setNewPendingAck() error: Too much pending ACKs. Not enough space to keep track of more"));
	else LOGLN(F("setNewPendingAck(): Keeping track of new ACK")); 
#endif /*VZ_CLOUD_DEBUG*/	
}

/** 
 *	@brief Confirm message arrived.
 * 	
 *	Every time we receive an ACK confirming the reception of a message we must confirm
 *	it using this function, so the timed error notification is cancelled.
 * 
 *	@return nothing
 */
void ViziblesArduino::confirmPendingAck(
					     unsigned int n /*!< [in] Sequence number of the message to be confirmed.*/) {
	LOG(F("[VSC] confirmPendingAck("));
	LOG(n);
	LOGLN(")");	
	confirmConnectionStatusOK();
	int i;
	for(i = 0; i < MAX_PENDING_ACKS; i++) {
		if (pendingAcks[i].pending && pendingAcks[i].n == n) {
			pendingAcks[i].pending = 0;
			break;
		}
	}
#ifdef VZ_CLOUD_DEBUG
	if (i==MAX_PENDING_ACKS) ERRLN(F("[VSC] confirmPendingAck() error: Sequence number not found"));
	else LOGLN(F("[VSC] confirmPendingAck(): ACK confirmed OK")); 
#endif /*VZ_CLOUD_DEBUG*/	
}

/** 
 *	@brief Check all ACKs.
 * 	
 *	This function must be called periodically to check if the time out of any message
 *	pending for confirmation has expired.
 * 
 *	@return nothing
 */
void ViziblesArduino::processPendingAcks(void) {
	//LOGLN(F("processPendingAcks()"));	
	for(int i = 0; i < MAX_PENDING_ACKS; i++) {
		if (pendingAcks[i].pending && elapsedMillis(pendingAcks[i].time) > options.ackTimeout) {
			ERR(F("[VSC] processPendingAcks() error: ACK not received for message "));
			ERRLN(pendingAcks[i].n);
			if (pendingAcks[i].callback!=NULL) {
				ERRLN(F("[VSC] processPendingAcks() error: Calling error callback"));
				pendingAcks[i].callback();
			} else {
				ERRLN(F("[VSC] processPendingAcks() error: No error callback defined for this ACK"));
			}
			
			pendingAcks[i].pending = 0;
			invalidatePendingAcks();
			confirmConnectionStatusERROR();
		}
	}	
}

/** 
 *	@brief Invalidate all ACKs.
 * 	
 *	This function must be called if disconnected so all pending ACKs are invalidated to avoid 
 *	problems in the next connection.
 * 
 *	@return nothing
 */
void ViziblesArduino::invalidatePendingAcks(void) {
	for(int i = 0; i < MAX_PENDING_ACKS; i++) {
		pendingAcks[i].pending = 0;
	}	
}

/** 
 *	@brief Process Websocket received messages.
 * 	
 *	This function must be peridically called to process all messages received
 *	through Websocket connection.
 * 
 *	@return zero if everyting is ok or a negative error code if not
 */
int ViziblesArduino::processWebsocket(void) {
	//LOGLN(F("[VSC] processWebsocket()"));
	while (mainClient->available() && 1) {
#ifdef ESP8266
		ESP.wdtFeed();
#endif		
		//LOGLN(F("[VSC] processWebsocket(): There is data pending to be read"));
		char data[1024];
		char opCode;
		//LOGLN(F("[VSC] processWebsocket(): Reading data"));
		webSocketClient.getData(data, 1024, (uint8_t *)&opCode);
		opCode = opCode & 0x0F;
		//LOGLN(data);
		/*              if (opCode!=3 &&opCode!=12 && opCode!=0 && opCode!=9 && opCode!=1 && opCode!=8) {
				Serial.print(F("Received opcode: "));
				Serial.println((int) opCode);
				if (data.length() > 0) {
				Serial.print(F("Received data: "));
				Serial.println(data);
				}       
				} */
		if (opCode == 9) { //Ping received
			LOGLN(F("[VSC] processWebsocket(): Ping received"));
			webSocketClient.sendData("", 0x80 | 10); //Send Pong
			confirmConnectionStatusOK();
		} else if (opCode == 8) { //Connection close asked by server
			ERRLN(F("[VSC] processWebsocket():  Connection close request received"));
			confirmConnectionStatusERROR();
		} else if ((opCode == 1 || opCode == 0) /*&& data.length() > 0*/) {
			LOGLN(F("[VSC] processWebsocket(): Message received"));
			confirmConnectionStatusOK();
			StaticJsonBuffer<BIG_JSON_PARSE_BUFFER> jsonBuffer;
			JsonObject& root = jsonBuffer.parseObject(data);
			if (root.success() && root.containsKey("c") && root.containsKey("p")) {
#ifdef VZ_CLOUD_DEBUG
				LOG(F("[VSC] processWebsocket() message: "));
				root.printTo(Serial);
				LOGLN();
#endif /*VZ_CLOUD_DEBUG*/	
				if (!strcmp_P((char *)root["c"].asString(), WSProtocolPingAck)) { //Pong received
					LOGLN(F("processWebsocket(): Ping Ack received"));
					confirmPendingAck(root["n"].as<unsigned int>());
				} else if (!strcmp_P((char *)root["c"].asString(), WSProtocolMe)) { //Me received
					LOGLN(F("[VSC] processWebsocket(): 'me' received"));
					JsonObject& p = root["p"].asObject();
					convertFlashStringToMemString(ccThingId, _thingId);
					setDefaultOption ((char *)p[(const char*)_thingId].asString(), &options.thingId);
#ifdef VZ_ITTT_RULES
					initITTT(httpClient, options.thingId);
#endif
#ifdef VZ_HTTP_SERVER
					if (pendingWifiConfig == 1) {
						keyValuePair values[3];
						convertFlashStringToMemString(wifiApplied, _wifiApplied);					
						convertFlashStringToMemString(wifiCapture, _capture);					
						convertFlashStringToMemString(wifiTrue, _true);					
						values[0].name = _wifiApplied;
						values[0].value = pendingCfId;
						values[1].name = _capture;
						values[1].value = _true;
						values[2].name = NULL;
						values[2].value = NULL;
						sendConfig(values);
						pendingWifiConfig = 0;
					}
#endif /*VZ_HTTP_SERVER*/
					if (options.onConnectCallback != NULL) options.onConnectCallback(); 
				} 
#ifdef VZ_EXECUTE_FUNCTIONS
				else if (!strcmp_P((char *)root["c"].asString(), WSProtocolThingDo)) { //ThingDo received
					LOGLN(F("processWebsocket(): ThingDo received"));
					//Execute function
					JsonObject& p0 = root["p"][0].asObject();
					int err = executeFunction(&p0);
					if (err != -1) {
						convertFlashStringToMemString(WSProtocolFunctionId, _functionId);
						convertFlashStringToMemString(ccTask, _task);
						convertFlashStringToMemString(sResult, _result);
						LOG(F("processWebsocket(): function "));
						LOG(root["p"][0][(const char*)_functionId].asString());
						LOGLN(F(" executed OK"));
						//Create t:result response
						StaticJsonBuffer<NORMAL_JSON_PARSE_BUFFER> resJsonBuffer;
						JsonObject& res = resJsonBuffer.createObject();
						res["c"] = "t:result";
						JsonObject& p = res.createNestedObject("p");
						p[(const char*)_functionId] = root["p"][0][(const char*)_functionId].asString();
						p[(const char*)_task] = root["p"][0][(const char*)_task].asString();
						p[(const char*)_result] = err==1?"ERROR":"OK";
						res["n"] = lastSequenceNumber;
						int len = res.measureLength();
						char cRes[len+1];
						res.printTo(cRes, len+1);
						LOGLN(F("processWebsocket(): Sending result answer"));
						//delay(200);
						WSSendData(cRes);
					} 
#ifdef VZ_CLOUD_DEBUG					
					else {
						convertFlashStringToMemString(WSProtocolFunctionId, _functionId);
						ERR(F("processWebsocket() error: Function "));
						ERR(root["p"][0][(const char*)_functionId].asString());
						ERRLN(F(" execution returned with error"));
					}
#endif /*VZ_CLOUD_DEBUG*/
				}
#endif /*VZ_EXECUTE_FUNCTIONS*/				
#ifdef VZ_ITTT_RULES
				else if (!strcmp_P((char *)root["c"].asString(), WSProtocolThingNewLocal)) { //ThingNewLocal received
					LOGLN(F("processWebsocket(): ThingNewLocal received"));
					JsonObject& p = root["p"].asObject();
					//p.printTo(Serial);
					parseNewLocal(&p);
				}		
#endif	/*VZ_ITTT_RULES*/	
				else if (!strcmp_P((char *)root["c"].asString(), WSProtocolUpdateAck) 	||	//AcK received
#ifdef VZ_EXECUTE_FUNCTIONS
					 !strcmp_P((char *)root["c"].asString(), WSProtocolFunctionsAck) ||
					 !strcmp_P((char *)root["c"].asString(), WSProtocolResultAck)	||
#endif /*VZ_EXECUTE_FUNCTIONS*/
#ifdef VZ_HTTP_SERVER
					 !strcmp_P((char *)root["c"].asString(), WSProtocolConfigAck)	||
#endif /*VZ_HTTP_SERVER*/
					 !strcmp_P((char *)root["c"].asString(), WSProtocolLocalAck)) {
					LOGLN(F("[VSC] processWebsocket(): ACK received"));	
					confirmPendingAck(root["n"].as<unsigned int>()); 
				} else {
					ERRLN(F("[VSC] Received unknown message"));
#ifdef VZ_CLOUD_DEBUG
					root.printTo(Serial);
					LOGLN();
#endif /*VZ_CLOUD_DEBUG*/
					return 0;
				}
			} else {
				ERRLN(F("[VSC] Error reading received Object"));
				ERRLN(data);
				ERRLN();
				return -1;
			}
		}
	}
#ifdef WS_BUFFERED_SEND
	webSocketClient.process();
#endif	
	return 0;
}

#endif /*VZ_WEBSOCKETS*/
#ifdef VZ_HTTP_SERVER
#ifdef VZ_EXECUTE_FUNCTIONS
/** 
 *	@brief Non-member function for do commands.
 * 	
 *	This function allows calling do commands executer member function 
 *	from outside the ViziblesArduino class.
 * 
 *	@return nothing
 */
void cmdDo(
	   Request &req,	/*!< [in] HTTP request.*/ 
	   Response &res	/*!< [in] HTTP response.*/) {
	LOGLN(F("cmdDo()"));		
	if (instance!=NULL) instance->cmdDo_(req, res);
}
#endif /*VZ_EXECUTE_FUNCTIONS*/

/** 
 *	@brief Check HTTP request is Authorized.
 * 	
 *	This function analizes all security information	in the 
 *	HTTP request and checks if it is from an authorized source.
 *	Note aWOT server library must be configured properly to store
 *	Authorization, Content-Type and Date headers in HTTP requests
 *	for this function to work properly.
 * 
 *	@return 0 if not authorized, 1 if authorized
 */
int ViziblesArduino::isHTTPAuthorized(
					   Request &req,	/*!< [in] HTTP request.*/
					   char *body		/*!< [in] Post payload if any.*/) {
	LOGLN(F("isHTTPAuthorized()"));	
 	//Check if request is authorized
	//Check if requred headers are present
	//convertFlashStringToMemString(authorizationHeaderName, _authorization);
	char *authorizationHeader = req.header(authorizationHeaderName);
	//convertFlashStringToMemString(contentTypeHeaderName, _contentType);
	char *contentTypeHeader = req.header(contentTypeHeaderName);
	//convertFlashStringToMemString(dateHeaderName, _date);
	char *dateHeader = req.header(dateHeaderName);
	if (!dateHeader || !authorizationHeader || !contentTypeHeader) {
		LOGLN(F("isHTTPAuthorized() error: Required headers not present in the request"));
		//if (!dateHeader) Serial.println("No date header");
		//if (!authorizationHeader) Serial.println("No authorization header");
		//if (!contentTypeHeader) Serial.println("No content-type header");
		return 0;
	}
	//Serial.println(contentTypeHeader);
	//Serial.println(dateHeader);
	//Serial.println(authorizationHeader);
	//Check if date and time are within range
	{
		char now[30];
		getDateString(now);
		long seconds_now = parseDateString(now);
		long seconds_header = parseDateString(dateHeader);
		if (seconds_header<(seconds_now-300) || seconds_header>(seconds_now+300)) {
			ERRLN(F("[VSC] isHTTPAuthorized() error: Received date is not within accepted range"));
			return 0;	
		}
	}
	//Check authorization header
	//Authorization: VIZIBLES [keyid|thingid]:hash
	{
		char v[10];
		strncpy(v,authorizationHeader, 10);
		v[9] = '\0';
		if (strcmp(v,"VIZIBLES ")) {
			ERRLN(F("[VSC] isHTTPAuthorized() error: Not VIZIBLES authorization scheme"));
			return 0;	
		}
	}
	int i=9;
	while (authorizationHeader[i]!=':' && authorizationHeader[i]!='\0') i++;
	unsigned int keyIdLen = i-9;
	char keyId[keyIdLen+1];
	for(i = 0; i < keyIdLen; i++) keyId[i] = authorizationHeader[i+9];
	keyId[keyIdLen]='\0';
	//Serial.println(&authorizationHeader[0]);
	convertFlashStringToMemString(optionsProtocolHttp, _http);
	char ip[16];
	if (!strcmp(options.keyID, keyId) || keyId[0]=='\0') {
		LOGLN(F("isHTTPAuthorized(): Using main key"));
		char signature[28];
		createHashSignature(signature, options.keySecret, "POST", _http, getLocalIp(ip), DEFAULT_THING_HTTP_SERVER_PORT, req.urlPath(), contentTypeHeader, NULL, dateHeader, body); 
		if (strcmp(signature, &authorizationHeader[keyIdLen+10])) {
			ERRLN(F("[VSC] isHTTPAuthorized() error: Wrong signature"));
			return 0;
		} else {
			confirmConnectionStatusOK();
		}
	} else {
		LOGLN(F("isHTTPAuthorized(): Must build ticket"));
		char ticket[28];
		createTicket(ticket, options.keySecret, options.keyID, options.thingId, keyId);
		//Serial.println(ticket);
		char signature[28];
		createHashSignature(signature, ticket, "POST", _http, getLocalIp(ip), DEFAULT_THING_HTTP_SERVER_PORT, req.urlPath(), contentTypeHeader, NULL, dateHeader, body); 
		if (strcmp(signature, &authorizationHeader[keyIdLen+10])) {
			//LOGLN(signature);
			//LOGLN(&authorizationHeader[keyIdLen+10]);
			ERRLN(F("[VSC] isHTTPAuthorized() error: Wrong signature"));
			return 0;
		}
	}	
	LOGLN(F("isHTTPAuthorized(): Authorized"));
	return 1;
}
#endif /*VZ_HTTP_SERVER*/

#ifdef VZ_EXECUTE_FUNCTIONS
/** 
 *	@brief Execute function.
 * 	
 *	Execute a pre-registered function given its name and the params to pass.
 * 
 *	@return 0 if everything is ok or 1 if the function was not found.
 */
int ViziblesArduino::execute(
				  char *functionName,       /*!< [in] Function name.*/
				  const char *parameters[]  /*!< [in] Parameters to pass to the function (NULL terminated).*/) {
	LOG(F("execute(\""));
	LOG(functionName);
	LOG(F("\", "));
#ifdef VZ_CLOUD_DEBUG
	LOG(F("{"));
	int z = 0;
	while (parameters[z] != NULL) {
		if (z>0) LOG(F(", "));
		LOG(F("\""));
		LOG(parameters[z]);
		LOG(F("\""));
		z++;
	}	
	LOG(F("}"));
#endif
	LOGLN(F(")"));	
	int i = 0;
	int err = 1;
	while (i<exposed) {
		if (!strcmp(functions[i].functionId, functionName)) {
			functions[i].handler(parameters);
			err = 0;
			break;
		}
		i++;
	}
	return err;
}

/** 
 *	@brief Execute a pre-registered function.
 * 	
 *	Parse the thingdo request and determine which function is requested to be called
 *	and the parameters for the call. Take into account that the first parameter will be
 *	the function name.
 * 
 *	@return 0 if everything is ok or an error code if something is not right
 */
int ViziblesArduino::executeFunction(
					  JsonObject *thingDo, 	/*!< [in] JSON object with the ThingDo request information.*/
					  char *function	/*!< [in] Function name (optional).*/) {
	LOGLN(F("[VSC] executeFunction()"));	
	JsonObject& root = *thingDo;
	convertFlashStringToMemString(ccParams, _params);
	if (!root.success() || !root.containsKey((const char*)_params) || !root[(const char*)_params].is<JsonArray&>()) { // || !root.containsKey("task") || !root["task"].is<const char*>()) {
		return -1;
	}
	//Calculate params array length
	int i = 0;
	while ((const char *)root[(const char*)_params][i] != NULL) i++;
	//Create params array for the function call
	const char *values[i+2];
	values[i+1] = NULL;
	for (int n = 0; n < i; n++) values[n + 1] = root[(const char*)_params][n].asString();
	//Get name of the function to call
	char *f;
	int ws = strcmp_P(options.protocol, optionsProtocolHttp);
#ifdef VZ_WEBSOCKETS
	if (ws) {
		convertFlashStringToMemString(WSProtocolFunctionId, _functionId);
		if (function == NULL && !root.containsKey((const char*)_functionId)) return -1;
		else {
			if (function != NULL) f = function;
			else if (root.containsKey((const char*)_functionId)) f = (char *)root[(const char*)_functionId].asString();
		}
	}
#endif	/*VZ_WEBSOCKETS*/
#ifdef VZ_HTTP
	if (!ws) {
		if (function != NULL) f = function;
		else return -1;
	}	
#endif	/*VZ_HTTP*/
	//Include function name as the first parameter
	values[0] = f;
	//Execute function
	return execute(f, values);
}
#endif /*VZ_EXECUTE_FUNCTIONS*/

#ifdef VZ_HTTP_SERVER
#ifdef VZ_EXECUTE_FUNCTIONS
/** 
 *	@brief Do commands exec.
 * 	
 *	This function handles the calls to exposed functions.
 * 
 *	@return nothing
 */
void ViziblesArduino::cmdDo_(
				  Request &req, 	/*!< [in] HTTP request.*/
				  Response &res	/*!< [in] HTTP response.*/) {
	LOGLN(F("cmdDo_()"));	
	int i=0;
	//Read post body
	printTimeCreateVariables();
	while (!req.available() && i<100) {
		//Sometimes payload takes longer to be received 
		delay(10);
		i++;
	}	
	unsigned int bodyLen = req.available();
	char body[bodyLen+1];
	i = 0;
	while (i<bodyLen) body[i++] = req.read();
	body[bodyLen] = '\0';
	printTimeSpent("HTTP Server read payload: \t\t");
	//Check if request is authorized
	if (!isHTTPAuthorized(req, body)) {
		ERRLN(F("[VSC] HTTP Unauthorized"));
		res.unauthorized();
		return;
	}
	printTimeSpent("HTTP Server check authorization: \t");
	char *f = req.route(0);
	//Parse and check JSON parameters provided in the body
	//The expected format is something like this: {"params":[],"task":"ABCD1234QWERTY567890"}
	StaticJsonBuffer<NORMAL_JSON_PARSE_BUFFER> jsonBuffer;
	JsonObject& root = jsonBuffer.parseObject((char*)body);
	int err = executeFunction(&root, f);
	if (err == -1) { 
		res.fail();
		return;
	}
	printTimeSpent("HTTP Server execute function: \t\t");
	//Calculate content length 
	convertFlashStringToMemString(ccTask, _task);
	char *task = NULL;
	if (root.containsKey((const char*)_task) && root[(const char*)_task].is<const char*>()) {
		task = (char *)root[(const char*)_task].asString();		
		//Serial.println(task);
	}
	char sBodyLen[10];
	int iBodyLen = 45 + strlen(f);
	iBodyLen += err?5:2;
	iBodyLen += task==NULL?0:strlen(task);
	itoa(iBodyLen, sBodyLen,10);
	//Build response
	//I build the whole response here so it can be sent in only one message instead of using 
	//aWOT library for doing it, since the library sends it in many TCP packets.
	char response[iBodyLen+strlen(sBodyLen)+98];
	int k = 0;
	strcpy_P(response, doResponse_0);
	k += strlen_P(doResponse_0);
	strcpy(&response[k], sBodyLen);
	k += strlen(sBodyLen);
	strcpy_P(&response[k], newLine);
	k += strlen_P(newLine);
	strcpy_P(&response[k], newLine);
	k += strlen_P(newLine);
	strcpy_P(&response[k], doResponse_2);
	k += strlen_P(doResponse_2);
	strcpy(&response[k], f);
	k += strlen(f);
	strcpy_P(&response[k], doResponse_3);
	k += strlen_P(doResponse_3);
	if (task!=NULL) strcpy(&response[k], task);
	k += task==NULL?0:strlen(task);
	strcpy_P(&response[k], doResponse_4);
	k += strlen_P(doResponse_4);
	strcpy_P(&response[k], err?doResponse_6:doResponse_7);
	k += strlen_P(err?doResponse_6:doResponse_7);
	strcpy_P(&response[k], doResponse_5);
	k += strlen_P(doResponse_5);
	response[k] = '\0';
	res.write((uint8_t*)response, k);
	printTimeSpent("HTTP Server send response: \t\t");
}
#endif /*VZ_EXECUTE_FUNCTIONS*/
#endif /*VZ_HTTP_SERVER*/

#ifdef VZ_ITTT_RULES

/** 
 *	@brief parse thingNewLocal information.
 * 	
 *	This function gets a thingNewLocal JSON object and parses it to extract and store 
 *	information on these messages, lite ITTT rules and updates, addresses of other things
 *	on LAN and permissions to communicate with them.
 * 
 *	@return nothing
 */
int ViziblesArduino::parseNewLocal(
					JsonObject *root_	/*!< [in] Parsed JSON Object with thingNewLocal content.*/ ) {
	LOGLN(F("parseNewLocal()"));	
	//Parse and store ITTT rules, and addresses or tickets related
	JsonObject& root = *root_;
	int error = 0, i;
	char *ittts = NULL; 
	convertFlashStringToMemString(itttsKey, _ittts);
	convertFlashStringToMemString(itttsAddedKey, _itttsAdded);
	//root.printTo(Serial);
	if (root.containsKey((const char*)_ittts)) {
		initITTTRules();
		ittts = _ittts;
	} else if (root.containsKey((const char*)_itttsAdded)) {
		ittts = _itttsAdded;
	}
	if (ittts!=NULL) {
		i=0;
		while ((const char *)root[(const char*)ittts][i]["if"][0]!=NULL) {
			if (!addITTTRule(root[(const char*)ittts][i])) {
				error = 1;
			};
			i++;
		}
	}
	convertFlashStringToMemString(newLocalAddresses, _addresses);
	if (root.containsKey((const char*)_addresses)) {
		if (!addITTTAddress(root[(const char*)_addresses])) {
			error = 1;
		};
	}
	convertFlashStringToMemString(newLocalTickets, _tickets);
	if (root.containsKey((const char*)_tickets)) {
		if (!addITTTTickets(root[(const char*)_tickets], options.keySecret)) {
			error = 1;
		};
	}
	convertFlashStringToMemString(newLocalItttsRemoved, _itttsRemoved);
	if (root.containsKey((const char*)_itttsRemoved)) {
		i=0;
		while ((const char *)root[(const char*)_itttsRemoved][i]!=NULL && root[(const char*)_itttsRemoved][i].is<const char*>()) {
			if (!removeITTTRule((char *)root[(const char*)_itttsRemoved][i].asString())) {
				error = 1;
			};
			i++;
		}
	}
	return error;
}

#endif /*VZ_ITTT_RULES*/
/** 
 *	@brief Update values in the platform.
 * 	
 *	Function to send sensor or state values to the platform. It requires just
 *  a list of key-value pairs with the information to be send.
 * 
 *	@return zero if everyting is ok or a negative error code if not
 */
int ViziblesArduino::update (
				  keyValuePair payLoadValues[],	/*!< [in] Array of key-value pairs to send, terminated with NULL-named pair.*/
				  cloudCallback cb 				/*!< [in] Callback function to be executed in case of error.*/){
	//LOGLN(F("[VSC] update()"));	
#ifdef VZ_ITTT_RULES
	int nRules = itttGetNumberOfActiveRules();
	char _meta[17 + (nRules * 15)]; 
	//Serial.print("Number of active rules: ");
	//Serial.println(nRules);
	int check = testITTTRules(payLoadValues, _meta, this);
	convertFlashStringToMemString(itttMetaKey, _Meta);
#endif
	int i = 0;
	while (payLoadValues[i++].name);
	keyValuePair *_payLoadValues = NULL;
#ifdef VZ_ITTT_RULES
	keyValuePair newValues[i+1];
	if (check) {
		i = 0; 
		while (payLoadValues[i].name) {
			newValues[i].name = payLoadValues[i].name;
			newValues[i].value = payLoadValues[i].value;
			i++;
		}
		newValues[i].name = _Meta;
		newValues[i++].value = _meta;
		newValues[i].name = NULL;
		newValues[i].value = NULL;
		_payLoadValues = newValues;
	} else _payLoadValues = payLoadValues;
#else
	_payLoadValues = payLoadValues;
#endif
	return sendObject("update", _payLoadValues, NULL, 0, cb);
};

#ifdef VZ_EXECUTE_FUNCTIONS

/** 
 *	@brief Expose a function call.
 * 	
 *	This function enables calling an internal function of this thing from the
 *	Vizibles cloud or form another thing in the local network.
 * 
 *	@return zero if everyting is ok or a negative error code if not
 */
int ViziblesArduino::expose(
				 char *functionId, 		/*!< [in] Name that will reference to the exposed function.*/
				 function_t *function,	/*!< [in] Function pointer to call the functionality.*/
				 cloudCallback cb 		/*!< [in] Callback function to be executed in case of error.*/){
	LOGLN(F("expose()"));	
	//Check if function ID is already known
	int found = 0;
	for (int i=0; i<exposed; i++) {
		if (!strcmp(functionId, functions[i].functionId)) {
			found = 1;
		}
	}
	//Add function if new
	if (!found) {
		if (exposed<MAX_EXPOSED_FUNCTIONS && (functionNamesIndex+strlen(functionId)+1<MAX_EXPOSED_FUNCTIONS_NAME_BUFFER)) {
			//Add function to exposed functions list
			exposed++;
			functions[exposed-1].functionId = &functionNames[functionNamesIndex];
			strcpy(&functionNames[functionNamesIndex], functionId);
			functionNamesIndex += strlen(functionId)+1;
			//functions[exposed-1].functionId = functionId;
			functions[exposed-1].handler = function;
			//Create parameters for sendFunctions
		} else return VIZIBLES_ERROR_TOO_MANY_FUNCTIONS;
	}	
	//inform the platform about the current list of exposed functions
	char *values[exposed+1];
	for (int i=0; i<exposed; i++) {
		values[i] = functions[i].functionId;
	}
	values[exposed] = NULL;
	//Create new route in the HTTP server
#ifdef VZ_HTTP_SERVER
	do_.post( functionId, cmdDo);
#endif /*VZ_HTTP_SERVER*/
	return sendFunctions(values, cb);		
}
#endif /*VZ_EXECUTE_FUNCTIONS*/

#ifdef VZ_HTTP_SERVER
/** 
 *	@brief Configure HTTP service.
 * 	
 *	This function performs the initial configuration of the HTTP server
 *	which handles incomming communications with this thing.
 * 
 *	@return nothing
 */
void ViziblesArduino::listen ( void ) {
	LOGLN(F("[VSC] listen()"));
	if (!serverReady){
		httpService->readHeader(authorizationHeaderName, headerAuthorizationBuffer, AUTHORIZATION_HEADER_MAX_LENGTH);	
		httpService->readHeader(contentTypeHeaderName, headerContentTypeBuffer, CONTENT_TYPE_HEADER_MAX_LENGTH);
		httpService->readHeader(dateHeaderName, headerDateBuffer, DATE_HEADER_MAX_LENGTH);
#ifdef VZ_EXECUTE_FUNCTIONS
		httpService->use(&this->do_);
#endif /*VZ_EXECUTE_FUNCTIONS*/	
		httpService->post("/config/wifi", cmdConfig);
#ifdef VZ_ITTT_RULES
#ifdef VZ_HTTP
		if (options.protocol!=NULL && !strcmp_P(options.protocol, optionsProtocolHttp)) httpService->post("/newlocal", cmdNewLocal);
#endif	
#endif
		
 		serverReady = 1;
	}	
}
#endif

/** 
 *	@brief Process periodical requests.
 * 	
 *	This function must be called periodically with a handler to an incomming client
 *	for the system to be able to service clients' requests.
 * 
 *	@return nothing
 */
void ViziblesArduino::process ( 
				    Client *client /*!< [in] Pointer to a client pending to be serviced.*/) {
	//LOGLN("process()");	
#ifdef VZ_HTTP_SERVER
	configureWiFi();
	if (serverReady && client) {
		printTimeCreateVariables();
		//Serial.println(F("[VSC] process(). client request"));
		double t = millis();
		while (!client->available() && elapsedMillis(t) < 5000) delay (1); //Wait for client to send message
		if (client->available()) { //Process client request
			httpService->process(client);
			printTimeSpent("Server processing time: ");
			client->stop();
		}
	}
#endif	
	if (options.protocol!=NULL) {
		int ws = strcmp_P(options.protocol, optionsProtocolHttp);
#ifdef VZ_WEBSOCKETS
		if (ws) {
			if (tryToConnect && !mainClient->connected()) {
				ERRLN(F("[VSC] ERROR: mainClient NOT connected!"));
				confirmConnectionStatusERROR();
			}
			else {
				//Serial.println(elapsedMillis(lastPing));
				if (cloudConnected && (elapsedMillis(lastPing) > options.pingDelay))	{
					//Serial.println("Sending Ping");
					sendPing();
					lastPing = millis();
				}	
				if (elapsedMillis(lastWebsocketRead) > 100) {
					if (mainClient->available()) lastWebsocketRead = millis();
					//cloudConnected = 1;
					processWebsocket(); 
				}
			}
			processPendingAcks();
		}
#endif /*VZ_WEBSOCKETS*/	
#ifdef VZ_HTTP
		if (!ws) {
			if (cloudConnected && (elapsedMillis(lastPing) > options.pingDelay)) {
				if (sendPing()) {
					lastPing = millis();
					pingRetries--;
					if (pingRetries==0) {
						ERRLN(F("[VSC] pingRetries == 0 => confirmConnectionStatusERROR()"));
						confirmConnectionStatusERROR();
					}
				} else {
					confirmConnectionStatusOK();
				}
			}
		}
#endif /*VZ_HTTP*/
	}
	//Check connection status periodically
	if (!cloudConnected && tryToConnect && (elapsedMillis(lastConnection) > 10000)) {
		if (isWiFiConnected()) {
			lastWiFiConnection = millis();
			if (isApActive()) disableWiFiAp();
			
			//LOG(F("Calling connectToVizibles() because cloudConnected = "));
			//LOGLN(cloudConnected);
			
			if (connectToVizibles()) {
				ERRLN(F("[VSC] Connection failed."));
			} else {
				LOGLN(F("Connection established OK"));
			}	
			lastConnection = millis();
		} else {
			if (lastWiFiConnection!=0 && !isApActive()) enableWiFiAp();
			else {
				if (lastWiFiConnection!=0)
					if (isTryingToConnectToLastAP() && (lastWiFiConnection!=0 && elapsedMillis(lastWiFiConnection)>330000)) {
						tryToConnectToLastAP(false);
						lastWiFiConnection = millis();
					}
				if (isTryingToConnectToLastAP() && (lastWiFiConnection==0 && elapsedMillis(lastWiFiConnection)>30000)) {
					if (!isApActive()) enableWiFiAp();
					lastWiFiConnection = millis();
				}				
				if (!isTryingToConnectToLastAP() && elapsedMillis(lastWiFiConnection)>300000) {
					tryToConnectToLastAP(true);
				}
			}
		}
	}
	if (cloudConnected && isApActive()) disableWiFiAp();
}
