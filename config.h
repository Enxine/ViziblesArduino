//Edit this file if you want to tweak available functions within the library to optimize code size for your application
#ifndef _vz_config_h
#define _vz_config_h
#define VZ_DEBUG_LEVEL 1
//Uncomment the following lines to have debug output over serial port
//#define VZ_CLOUD_DEBUG 
//#define VZ_CLOUD_ERR 
//Uncoment the following line to allow function execution and exposition to the cloud and to the local things if HTTP service is activated
#define VZ_EXECUTE_FUNCTIONS
//Uncoment the following line to include the possibility of local HTTP service for receiving data from cloud or from other things
#define VZ_HTTP_SERVER
//Uncoment the following line to include the possibility of local ITTT rule execution 
#define VZ_ITTT_RULES
//Uncoment the line below to enable HTTP connection with Vizibles platform 
//#define VZ_HTTP
//Uncoment the line below to enable Websocket connection with Vizibles platform
#define VZ_WEBSOCKETS

#define VZ_DEFAULT_AP_SSID		"VIZIBLES_xxxx"
#define VZ_DEFAULT_AP_PASSWD	"00000000"

#define VZ_DEFAULT_HOSTNAME "api.vizibles.com"

#endif //_vz_config_h
