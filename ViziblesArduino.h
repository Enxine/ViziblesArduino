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

#ifndef _ViziblesArduino_h
#define _ViziblesArduino_h

#include <HttpClient.h>
#include <ArduinoJson.h>
#include "Client.h"

#include "config.h"

#if !defined(VZ_WEBSOCKETS) && !defined(VZ_HTTP)
#error	At least one method to access Vizibles platform (VZ_WEBSOCKETS or VZ_HTTP) must be defined
#endif


#ifdef VZ_HTTP_SERVER
#include <aWOT.h>
#endif /*VZ_HTTP_SERVER*/
#ifdef VZ_WEBSOCKETS
#include <WebSocketClient.h>
#endif

#ifdef VZ_CLOUD_ERR
#define VZ_CLOUD_ERR_TEST 1
#else
#define VZ_CLOUD_ERR_TEST 0
#endif /*VZ_CLOUD_ERR*/
#define ERR(message) do{if(VZ_CLOUD_ERR_TEST) Serial.print(message);} while(0)
#define ERRLN(message) do{if(VZ_CLOUD_ERR_TEST) Serial.println(message);} while(0)

#ifdef VZ_CLOUD_DEBUG
#define VZ_CLOUD_DEBUG_TEST 1
#else
#define VZ_CLOUD_DEBUG_TEST 0
#endif	/*VZ_CLOUD_DEBUG*/
#define LOG(message) do{if(VZ_CLOUD_DEBUG_TEST) Serial.print(message);} while(0)
#define LOGLN(message) do{if(VZ_CLOUD_DEBUG_TEST) Serial.println(message);} while(0)

	
#define LOGLVL(message, level) do{if(VZ_CLOUD_DEBUG_TEST && level<=VZ_DEBUG_LEVEL) Serial.print(message);} while(0)
#define LOGLNLVL(message, level) do{if(VZ_CLOUD_DEBUG_TEST && level<=VZ_DEBUG_LEVEL) Serial.println(message);} while(0)
#define ERRLVL(message, level) do{if(VZ_CLOUD_ERR_TEST && level<=VZ_DEBUG_LEVEL) Serial.print(message);} while(0)
#define ERRLNLVL(message, level) do{if(VZ_CLOUD_ERR_TEST && level<=VZ_DEBUG_LEVEL) Serial.println(message);} while(0)
	
//One day (in seconds) for time synchronization period
#define SYNCHRONIZATION_PERIOD 			86400

#define DEFAULT_THING_HTTP_SERVER_PORT	5000
#define DEFAULT_VIZIBLES_HTTP_PORT		80
#define DEFAULT_VIZIBLES_HTTPS_PORT		443
#define HMAC_SHA1_KEY_LENGTH 20
#define HMAC_SHA1_HASH_LENGTH_BINARY 20
#define HMAC_SHA1_HASH_LENGTH_CODE64 27
#define DATE_HEADER_MAX_LENGTH	35
#define AUTHORIZATION_HEADER_MAX_LENGTH 255
#define CONTENT_TYPE_HEADER_MAX_LENGTH 30

#define VIZIBLES_ERROR_UNAUTHORIZED 		-10
#define VIZIBLES_ERROR_SYNCHRONIZING 		-11
#define VIZIBLES_ERROR_CONNECTING 			-12
#define VIZIBLES_ERROR_TOO_MANY_FUNCTIONS	-13
#define VIZIBLES_ERROR_INCORRECT_OPTIONS	-14

#define OPTIONS_BUFFER_LENGTH 				255
#define MAX_EXPOSED_FUNCTIONS 				10
#define MAX_EXPOSED_FUNCTIONS_NAME_BUFFER	127
#define MAX_PENDING_ACKS					10

#define BIG_JSON_PARSE_BUFFER				1024
#define NORMAL_JSON_PARSE_BUFFER			200

#define convertFlashStringToMemString(flash, memory)			\
	char memory[strlen_P((const char *)flash)+1];			\
	{								\
		PGM_P fPointer = reinterpret_cast<PGM_P>(flash);	\
		{							\
			unsigned int cFMi = 0;				\
			do {						\
				memory[cFMi++] = pgm_read_byte_near(fPointer++); \
			} while(memory[cFMi-1]!='\0');			\
		}							\
	}

const char sFunctions[] PROGMEM = { "functions" };
const char sResult[] PROGMEM = { "result" };
const char sLocal[] PROGMEM = { "local" };
const char sConfig[] PROGMEM = { "config" };
	
typedef void (*cloudCallback)(void); 

/* This struct is used to store all optional values when a connection is created.
   /* All strings are stored on stringBuffer, so be careful with maximum size.
*/
typedef struct tOptions {
	char *protocol;
	char *hostname;
	//char *path;
	//char *credentials;
	//unsigned int serverPort;
	char *id;
	char *keyID;
	char *keySecret;
	char *thingId;
	char *type;
	char *apiBasePath;
	unsigned int port;
	unsigned int pingRetries;
	double pingDelay;
	double ackTimeout;
	cloudCallback onConnectCallback;
	cloudCallback onDisconnectCallback;
	unsigned int index;
	char stringBuffer[OPTIONS_BUFFER_LENGTH];
};

/* This struct is used to pass values to send to the server as JSON enconded key-value pairs.
 * Normally you pass this as a array. The last entry must have the NULL-Pointer as name.
 */
typedef struct {
	char* name;
	char* value;
} keyValuePair;

typedef void function_t (const char *parameters[]	/*!< [in] Array of pointers to char arrays (strings) with the input parameters (NULL terminated).*/);
	
typedef struct {
	char* 		functionId;
	function_t* handler;
} functions_t;
	
typedef struct {
	cloudCallback callback;
	unsigned int n;
	unsigned char pending;
	double time;
} pendingAck_t;	
	
class ViziblesArduino {
 public:
	ViziblesArduino(Client& client, Client& client1);
	void option(char *name, char *value);
	int connect (keyValuePair values[], cloudCallback onConnectCallback = NULL, cloudCallback onDisconnectCallback = NULL);	
	int disconnect(void);
	int update (keyValuePair payLoadValues[], cloudCallback cb=NULL);
#ifdef VZ_EXECUTE_FUNCTIONS
	int expose(char *functionId, function_t *function, cloudCallback cb=NULL);
	int execute(char *functionName, const char *parameters[]);
#endif /*VZ_EXECUTE_FUNCTIONS*/				
	void process (Client *client);
#ifdef VZ_HTTP_SERVER

	void cmdDo_(Request &req, Response &res);
	void cmdConfig_(Request &req, Response &res);
#ifdef VZ_ITTT_RULES
#ifdef VZ_HTTP
	void cmdNewLocal_(Request &req, Response &res);
#endif /*VZ_HTTP*/
#endif /*VZ_ITTT_RULES*/
#endif /*VZ_HTTP_SERVER*/

 protected:

 private:
	int connectToVizibles (void);
	int sendObject (char *prim, keyValuePair payLoadValues[], char *response = NULL, unsigned int resLen = 0, cloudCallback cb=NULL);
	int sendArray (char *prim, char *payLoadValues[], char *response = NULL, unsigned int resLen = 0, cloudCallback cb=NULL);
#ifdef VZ_WEBSOCKETS
	int WSSendData (char *payload, cloudCallback cb=NULL);
#endif /*VZ_WEBSOCKETS*/
#ifdef VZ_HTTP
	int HTTPSendData (char *prim, char *payload, char *response, unsigned int resLen);
#endif /*VZ_HTTP*/
#ifdef VZ_EXECUTE_FUNCTIONS
	int sendFunctions (char *payLoadValues[], cloudCallback cb=NULL) { convertFlashStringToMemString(sFunctions, _functions); return sendArray (_functions, payLoadValues, NULL, 0, cb); };
	int sendResult (keyValuePair payLoadValues[]) { convertFlashStringToMemString(sResult, _result); return sendObject (_result, payLoadValues); };
#endif /*VZ_EXECUTE_FUNCTIONS*/
	int sendLocal (keyValuePair payLoadValues[]) { convertFlashStringToMemString(sLocal, _local); return sendObject (_local, payLoadValues); };
#ifdef VZ_HTTP
	int sendMe (keyValuePair payLoadValues[], char *response, unsigned int maxResponse) { return sendObject ("", payLoadValues, response, maxResponse); };
#endif /*VZ_HTTP*/		
	int sendPing (void);	
	void confirmConnectionStatusOK(void);
	void confirmConnectionStatusERROR(void);
	int syncTime(void);
	int parseOption (char *key, char *value, char *search, char **copyValueTo);
	int setDefaultOption (char *value, char **copyValueTo);
	int parseOption (char *key, char *value, const __FlashStringHelper *search, char **copyValueTo) { convertFlashStringToMemString(search, mSearch); return parseOption(key, value, mSearch, copyValueTo); };
	int setDefaultOption (const __FlashStringHelper *value, char **copyValueTo) { convertFlashStringToMemString(value, mValue); return setDefaultOption(mValue, copyValueTo); };
	void initializeOptions ();
	int setOption(char *value, char**copyValueTo);
#ifdef VZ_HTTP_SERVER
	void listen();
	int sendConfig (keyValuePair payLoadValues[]) { convertFlashStringToMemString(sConfig, _config); return sendObject (_config, payLoadValues); };
	void configureWiFi(void);
	void getConfigId(char *buffer);
	void createTicket(char *buffer, char *key, char *keyId, char *id, char *issuedTo);
	int isHTTPAuthorized(Request &req, char *body);
#endif /*VZ_HTTP_SERVER*/		
#ifdef VZ_EXECUTE_FUNCTIONS
	int executeFunction(JsonObject *root, char *function = NULL);
#endif /*VZ_EXECUTE_FUNCTIONS*/
#ifdef VZ_WEBSOCKETS
	int processWebsocket(void);
	void setNewPendingAck(cloudCallback callback, unsigned int n);
	void confirmPendingAck(unsigned int n);
	void processPendingAcks(void);
	void invalidatePendingAcks(void);
#endif /*VZ_WEBSOCKETS*/
#ifdef VZ_ITTT_RULES
	int parseNewLocal(JsonObject *root_);		
#endif /*VZ_ITTT_RULES*/	

	tOptions options;								/*!< Main configuration options.*/
	unsigned long lastSync;							/*!< Time when time was updated last time.*/
	double lastPing;								/*!< Last ping try time.*/
	double lastConnection;							/*!< Last connection try time.*/
	double lastWiFiConnection;						/*!< Last WiFi connection try time.*/	
	unsigned char cloudConnected;					/*!< Connection flag. Zero if not connected to the cloud or one if connected.*/
	unsigned char tryToConnect;						/*!< Try to connect flag. Zero if must remain unconnected to the cloud or one if must try to connect.*/
	Client *mainClient;
	HttpClient *httpClient;			
#ifdef VZ_EXECUTE_FUNCTIONS
	functions_t functions[MAX_EXPOSED_FUNCTIONS];	/*	!< Functions available for remote calls.*/
	unsigned char exposed;							/*!< Number of functions available for remote calls.*/
	char functionNames[MAX_EXPOSED_FUNCTIONS_NAME_BUFFER];
	int functionNamesIndex;
#endif /*VZ_EXECUTE_FUNCTIONS*/
#ifdef VZ_HTTP_SERVER
	WebApp *httpService;
	static Router do_;
	static char headerDateBuffer[];
	static char headerAuthorizationBuffer[];
	static char headerContentTypeBuffer[];
	char serverReady;								/*!< Flag to check if server was configured properly.*/
#endif /*VZ_HTTP_SERVER*/
#ifdef VZ_WEBSOCKETS
	WebSocketClient* webSocketClient;				/*!< Main websocket client.*/
	int lastWebsocketRead; 							/*!< Last websocket read try time.*/
	pendingAck_t pendingAcks[MAX_PENDING_ACKS];		/*!< Array of pending message acknowledgements.*/
	unsigned int lastSequenceNumber;				/*!< Last message sequence number used.*/
#endif /*VZ_WEBSOCKETS*/	
#ifdef VZ_HTTP
	int pingRetries;								/*!< Number of ping retries still left.*/
#endif /*VZ_HTTP*/		
	int pendingWifiConfig;							/*!< Flag to check if there is a pending try to connect to WiFi.*/
	static char pendingSsid[];
	static char pendingPass[];
	static char pendingCfId[];

};		
#endif //_ViziblesArduino_h
