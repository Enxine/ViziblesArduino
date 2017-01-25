// (c) Copyright 2016 Enxine DEV S.C.
// Released under Apache License, version 2.0
//
// This library is used to send data froma simple sensor 
// to the Vizibles IoT platform in an easy and secure way.

#include <ViziblesArduino.h>
#include "strings.h"
#include <HttpClient.h>
#include <sha1.h>
#include "b64.h"
#include <TimeLib.h>
#include "utils.h"

const char numericChars[] = { "0123456789.-" };

void printChar(char c) {
	if(c < 33 || c > 127) {
		char b[5];
		Serial.print("[");
		Serial.print(itoa(c, b, 10));
		Serial.print("]");
		//Serial.print(c);
	} else Serial.print(c);
}

/** 
 *	@brief Check if string represents a number.
 * 
 *	@return zero if not a number or one if number.
 */
int isNumber(char *str) {
	int res = 1;
	int i = 0, j;
	unsigned int dots = 0, minus = 0;
	while(str[i]!='\0') {
		int found = 0;
		for(j = 0; j < 12; j++) {
			if(str[i] == numericChars[j]) {
				found = 1;
				if(j==10) dots++;
				if(j==11) minus++;
				break;
			}	
		}
		if(!found) {
			res = 0;
			break;
		}	
		i++;
	}
	if(dots>1 || minus>1) res = 0;
	return res;
}

/** 
 *	@brief Check if string represents an integer number.
 * 
 *	@return zero if not an integer, -1 if it is an integer or a positive number if the
 *	string representation has decimal point but all decimal positions are zero. This 
 *	positive number is the position of the decimal point in the string.
 */
int isInteger(char *str) {
	int res = 1;
	int i = 0, dot = 0;
	while(str[i]!='\0') {
		if(dot==0 && str[i] == '.') {
			dot = i;
			//break;
		}	
		if(dot!=0 && i!=dot && str[i]!='0') {
			res = 0;
			break;
		}	
		i++;
	}
	if(res==1 && dot==0) return -1;
	else if(res==0) return 0;
	else return dot;
}

/** 
 *	@brief Receive HTTP response.
 * 	
 *	Generic function to use HTTP communication to receive response
 *	originated by a request.
 * 
 *	@return zero if everyting is ok or a negative error code if not
 */
int HTTPGetResponse(
		    HttpClient *http,	/*!< [in] HTTP client to be used.*/
		    char *response,		/*!< [out] Pointer to buffer where response must be stored (optional).*/
		    unsigned int resLen /*!< [in] Maximum buffer length for response (optional).*/) { 
#ifdef VZ_CLOUD_DEBUG
	LOG(F("HTTPGetResponse("));
	if(http!=NULL) {
		LOG(F("&HttpClient, "));
	} else LOG(F("NULL, "));
	if(response!=NULL) LOG(F("&response, "));
	else LOG(F("NULL, "));
	LOG(resLen);
	LOGLN(F(")"));
#endif /*VZ_CLOUD_DEBUG*/
	printTimeCreateVariables();
	//Wait for an answer and analyze it
	int err = http->responseStatusCode();
	printTimeSpent("HTTP Client responseStatusCode(): \t");
	if (err == 200) {
		err = http->skipResponseHeaders();
		printTimeSpent("HTTP Client skipResponseHeaders(): \t");
		if (err >= 0) {
			int bodyLen = http->contentLength();
			printTimeSpent("HTTP Client contentLength(): \t\t");
			//char resBody[bodyLen];
			//Serial.print("Body Length: ");
			//Serial.println(bodyLen);
			int l = 0;
			unsigned long timeoutStart = millis();
			char c;
			while ( (http->connected() || http->available()) && ((millis() - timeoutStart) < NETWORK_TIMEOUT) ) {
				if (http->available()) {
					c = http->read();
					bodyLen--;
					if(response!=NULL && l<resLen) response[l++] = c;  
					//else Serial.println(bodyLen);	
					timeoutStart = millis();
				} else {
					//Serial.println("Nothing to read");
					//delay(NETWORK_DELAY);
				}	
			}
			printTimeSpent("HTTP Client receive payload: \t\t");
			if(response!=NULL) {
				if(l<resLen) response[l] = '\0';
				else response[resLen-1] = '\0';
				LOG(F("HTTPGetResponse(): Response received "));
				LOGLN(response);
			}
			//if(response!=NULL) {				
			//	Serial.println(response);
			//	Serial.println(strlen(response));
			//}	
		} else {
			LOGLN(F("HTTPGetResponse() error: Failed to skip headers"));
			return(err);
		}	
	} else {
		if(err==401) {
			LOGLN(F("HTTPGetResponse() error: Response form server, Unauthorized (401)"));
			return(VIZIBLES_ERROR_UNAUTHORIZED);
		}	
		else {
			LOG(F("HTTPGetResponse() error: Response from server, error "));
			LOGLN(err);
			return(err);	
		}	
	}
	LOGLN(F("HTTPGetResponse(): Finished OK"));
	return(0);
}

/** 
 *	@brief Send HTTP request and receive response.
 * 	
 *	Generic function to use HTTP communication to send or receive data.
 * 
 *	@return zero if everyting is ok or a negative error code if not
 */
int HTTPRequest (
		 HttpClient *http,	/*!< [in] HTTP client to be used.*/
		 char *hostname,		/*!< [in] Destination host name.*/
		 unsigned int port,	/*!< [in] Destination port.*/
		 char *path,			/*!< [in] Path of the request.*/
		 char *method,		/*!< [in] HTTP method of the request.*/
		 char *contentType,	/*!< [in] Content type header of the payload (Must be NULL if no payload is provided).*/
		 char *payload,		/*!< [in] Cahr array with the payload to be send (Must be NULL if there is no payload to be sent).*/
		 char *headers[],	/*!< [in] Array of pointers to char arrays to be used as headers (NULL terminated).*/
		 char *response,		/*!< [out] Pointer to buffer where response must be stored (optional).*/
		 unsigned int resLen /*!< [in] Maximum buffer length for response (optional).*/) { 
#ifdef VZ_CLOUD_DEBUG
	LOG(F("HTTPRequest("));
	if(http!=NULL) {
		LOG(F("&HttpClient, \""));
	} else LOG(F("NULL, \""));
	LOG(hostname);
	LOG(F("\", "));
	LOG(port);
	LOG(F(", \""));
	LOG(path);
	LOG(F("\", \""));
	LOG(method);
	LOG(F("\", "));
	if(payload!=NULL) {
		LOG(F("\""));
		LOG(payload);
		LOG(F("\""));
	} else LOG(F("NULL"));
	if(headers!=NULL) {
		LOG(F(", {"));
		int l = 0;
		while (headers[l]!=NULL) {
			if(l>0) LOG(F(", "));
			LOG(F("\""));
			LOG(headers[l]);
			LOG(F("\""));
			l++;
		}
		LOG(F("}, "));
	} else {
		LOG(F(", NULL, "));
	}	
	if(response!=NULL) LOG(F("&response, "));
	else LOG(F("NULL, "));
	LOG(resLen);
	LOGLN(F(")"));
#endif /*VZ_CLOUD_DEBUG*/
	int err = 0;
	printTimeCreateVariables();
	http->beginRequest();
	printTimeSpent("HTTP Client beginRequest(): \t\t");
	err = http->startRequest(hostname, port, path, method, NULL, contentType, payload);
	printTimeSpent("HTTP Client startRequest(): \t\t");
	if(err == 0) {
		if(headers) {
			int n = 0;
			while (headers[n]) {
				http->sendHeader(headers[n]);
				n++;
			}
			printTimeSpent("HTTP Client send Headers: \t\t");
		}
		http->endRequest();
		printTimeSpent("HTTP Client endRequest(): \t\t");
		err = HTTPGetResponse(http, response, resLen);
		printTimeSpent("HTTP Client HTTPGetResponse(): \t\t");
		http->stop();
		printTimeSpent("HTTP Client stop(): \t\t\t");
		if(err) {
			LOGLN(F("HTTPRequest() error: HTTPGetResponse() returned with error"));
			return err;
		}	
	} else {
		LOGLN(F("HTTPRequest() error: Error trying to create request"));
		http->stop();
		printTimeSpent("HTTP Client stop(): \t\t\t");
		return(err);
	}	
	LOGLN(F("HTTPRequest(): Finished OK"));
	return(0);
}

/** 
 *	@brief Send HTTP fast request and receive response.
 * 	
 *	Generic function to use HTTP communication to send or receive data.
 *	This is a special version of the function HTTPRequest that makes sure 
 *	all HTTP request is sent in only one TCP packet. This need for 
 *	communicating with slow devices, like the ones based on ESP8266, where
 *	sending the HTTP request in chunks is too costly in terms of delay.
 * 
 *	@return zero if everyting is ok or a negative error code if not
 */
int HTTPFastRequest (
		     HttpClient *http,	/*!< [in] HTTP client to be used.*/
		     char *hostname,		/*!< [in] Destination host name.*/
		     unsigned int port,	/*!< [in] Destination port.*/
		     char *path,			/*!< [in] Path of the request.*/
		     char *method,		/*!< [in] HTTP method of the request.*/
		     char *contentType,	/*!< [in] Content type header of the payload (Must be NULL if no payload is provided).*/
		     char *payload,		/*!< [in] Cahr array with the payload to be send (Must be NULL if there is no payload to be sent).*/
		     char *headers[],	/*!< [in] Array of pointers to char arrays to be used as headers (NULL terminated).*/
		     char *response,		/*!< [out] Pointer to buffer where response must be stored (optional).*/
		     unsigned int resLen /*!< [in] Maximum buffer length for response (optional).*/) { 
#ifdef VZ_CLOUD_DEBUG
	LOG(F("HTTPFastRequest("));
	if(http!=NULL) {
		LOG(F("&HttpClient, \""));
	} else LOG(F("NULL, \""));
	LOG(hostname);
	LOG(F("\", "));
	LOG(port);
	LOG(F(", \""));
	LOG(path);
	LOG(F("\", \""));
	LOG(method);
	LOG(F("\", "));
	if(payload!=NULL) {
		LOG(F("\""));
		LOG(payload);
		LOG(F("\""));
	} else LOG(F("NULL"));
	if(headers!=NULL) {
		LOG(F(", {"));
		int l = 0;
		while (headers[l]!=NULL) {
			if(l>0) LOG(F(", "));
			LOG(F("\""));
			LOG(headers[l]);
			LOG(F("\""));
			l++;
		}
		LOG(F("}, "));
	} else {
		LOG(F(", NULL, "));
	}	
	if(response!=NULL) LOG(F("&response, "));
	else LOG(F("NULL, "));
	LOG(resLen);
	LOGLN(F(")"));
#endif /*VZ_CLOUD_DEBUG*/
	printTimeCreateVariables();
	//Calculate request length
	char aPort[6];
	itoa(port, aPort, 10);
	char aCL[6];
	itoa(strlen(payload), aCL, 10);
	int requestLen = 77 + strlen(method) + strlen(path) + strlen(hostname) + strlen(aPort) + strlen(contentType) + strlen(aCL) + strlen(payload);
	int i = 0;
	if(headers) {
		while(headers[i]!=NULL) {
			requestLen += strlen(headers[i])+2;
			i++;
		}
	}	
	//Build request
	char request[requestLen];
	int k = 0;
	strcpy(request, method);
	k += strlen(method);
	strcpy(&request[k], " ");
	k++;
	strcpy(&request[k], path);
	k += strlen(path);
	int fl = k + 11;
	strcpy_P(&request[k], connectionAndHost);
	k += 36;
	strcpy(&request[k], hostname);
	k += strlen(hostname);
	strcpy(&request[k], ":");
	k++;
	strcpy(&request[k], aPort);
	k += strlen(aPort);
	strcpy_P(&request[k], newLine);
	k += 2;
	//User agent can go here if needed
	strcpy_P(&request[k], contentTypeHeaderName);
	k += 12;
	strcpy_P(&request[k], dotSpace);
	k += 2;
	strcpy_P(&request[k], contentType);
	k += strlen(contentType);
	strcpy_P(&request[k], newLine);
	k += 2;
	if(headers) {
		i = 0;
		while(headers[i]!=NULL) {
			strcpy(&request[k], headers[i]);
			k += strlen(headers[i]);
			strcpy_P(&request[k], newLine);
			k += 2;
			i++;
		}
	}	
	strcpy_P(&request[k], contentLenghtHeaderName);
	k += 14;
	strcpy_P(&request[k], dotSpace);
	k += 2;
	strcpy(&request[k], aCL);
	k += strlen(aCL);
	strcpy_P(&request[k], newLine);
	k += 2;
	strcpy_P(&request[k], newLine);
	k += 2;
	strcpy(&request[k], payload);
	k += strlen(payload);
	request[k] = '\0';
	printTimeSpent("HTTP Client create request: \t\t");
	LOGLN(F("HTTPFastRequest(): HTTP request created:"));
	LOGLN(request);
	//Connect to thing and send request
	http->beginRequest();
	if (!http->connect((const char *)hostname, (uint16_t) port) > 0) {
		LOGLN(F("HTTPFastRequest() error: Connection failed"));
		return HTTP_ERROR_CONNECTION_FAILED;
	}
	printTimeSpent("HTTP Client connect: \t\t\t");
	http->forceEndRequest();
	http->write((const uint8_t *)request, k);
	printTimeSpent("HTTP Client send request: \t\t");
	int err = HTTPGetResponse(http, response, resLen);
	printTimeSpent("HTTP Client HTTPGetResponse(): \t\t");
	//http->stop();
	printTimeSpent("HTTP Client stop(): \t\t\t");
	if(err) {
		LOGLN(F("HTTPFastRequest() error: HTTPGetResponse() returned with error"));
		return err;
	}
	LOGLN(F("HTTPFastRequest(): Finished OK"));
	return(0);
}

#ifdef VZ_CLOUD_DEBUG
#define DEBUG_HASH
#endif /*VZ_CLOUD_DEBUG*/
/** 
 *	@brief Create signature.
 * 	
 *	This function constructs the relevant text based on the information provided through
 *	parameters and creates an SHA1 HMAC of it (base64) using the provided secret key.
 * 
 *	@return nothing
 */
void createHashSignature(
			 char *buffer,		/*!< [out] Buffer to write the final hash string (result has fixed length, 27 bytes. 28 with the \0 string terminator). */
			 char *key,			/*!< [in] Key secret for the HMAC. */
			 char *httpMethod,	/*!< [in] HTTP method that will be used (optional). */
			 char *protocol,		/*!< [in] Protocol to be used in the communication. */
			 char *hostName,		/*!< [in] Destination host name of the communication. */
			 unsigned int port,	/*!< [in] Destination port. */
			 char *path,			/*!< [in] Full path of the url being accesed. */
			 char *contentType,	/*!< [in] Content type header string (optional). */
			 char *id,			/*!< [in] Thing not scoped ID followed by two dots and the type ID if any (optional).*/ 
			 char *dateString,	/*!< [in] UTC date header string. */
			 char *payload		/*!< [in] Payload to be included in the communication (optional).*/){
#ifdef DEBUG_HASH
	LOG(F("createHashSignature(&buffer, \""));
	LOG(key);
	LOG(F("\", "));
	if(httpMethod!=NULL) {
		LOG(F("\""));
		LOG(httpMethod);
		LOG(F("\""));
	} else LOG(F("NULL"));
	LOG(F(", \""));
	LOG(protocol);
	LOG(F("\", \""));
	LOG(hostName);
	LOG(F("\", "));
	LOG(port);
	LOG(F(", "));
	if(contentType!=NULL) {
		LOG(F("\""));
		LOG(contentType);
		LOG(F("\""));
	} else LOG(F("NULL"));
	LOG(F(", "));
	if(id!=NULL) {
		LOG(F("\""));
		LOG(id);
		LOG(F("\""));
	} else LOG(F("NULL"));
	LOG(F(", \""));
	LOG(protocol);
	LOG(F("\","));
	if(payload!=NULL) {
		LOG(F("\""));
		LOG(payload);
		LOGLN(F("\")"));
	} else LOGLN(F("NULL)"));
	LOGLN(F("createHashSignature(): Text to hash:"));
#endif /*DEBUG_HASH*/
	Sha1.initHmac((uint8_t *)key, HMAC_SHA1_KEY_LENGTH);
	int len;
	if(httpMethod!=NULL) {
		len = strlen(httpMethod);
		for(int i = 0; i < len; i++) {
			Sha1.write(httpMethod[i]);
#ifdef DEBUG_HASH
			LOG(httpMethod[i]);
#endif		
		}	
		Sha1.write('\n');
#ifdef DEBUG_HASH
		LOG('\n');
#endif	
	}
	len = strlen(protocol);
	for(int i = 0; i < len; i++) {
		Sha1.write(protocol[i]);
#ifdef DEBUG_HASH
		LOG(protocol[i]);
#endif		
	}	
	Sha1.write(':');
#ifdef DEBUG_HASH
	LOG(':');
#endif	
	Sha1.write('/');
#ifdef DEBUG_HASH
	LOG('/');
#endif
	Sha1.write('/');
#ifdef DEBUG_HASH
	LOG('/');
#endif	
	len = strlen(hostName);
	for(int i = 0; i < len; i++) {
		Sha1.write(hostName[i]);
#ifdef DEBUG_HASH
		LOG(hostName[i]);
#endif		
	}
	//REMOVE ME	
	Sha1.write(':');
#ifdef DEBUG_HASH
	LOG(':');
#endif	
	{
		char number[6];
		itoa(port, number, 10);
		for(int i=0; i<strlen(number); i++)	{
			Sha1.write(number[i]);
#ifdef DEBUG_HASH
			LOG(number[i]);
#endif			
		}	
	}

	if(path[0]!='/') {
		Sha1.write('/');
#ifdef DEBUG_HASH
		LOG('/');
#endif		
	}	
	len = strlen(path);
	for(int i = 0; i < len; i++) {
		Sha1.write(path[i]);
#ifdef DEBUG_HASH
		LOG(path[i]);
#endif		
	}	
	Sha1.write('\n');
#ifdef DEBUG_HASH
	LOG('\n');
#endif	
	if(contentType!=NULL) {
		len = strlen(contentType);
		for(int i = 0; i < len; i++) {
			Sha1.write(contentType[i]);
#ifdef DEBUG_HASH
			LOG(contentType[i]);
#endif		
		}	
		Sha1.write('\n');
#ifdef DEBUG_HASH
		LOG('\n');
#endif	
	}
	if(id!=NULL) {
		len = strlen(id);
		for(int i = 0; i < len; i++) {
			Sha1.write(id[i]);
#ifdef DEBUG_HASH
			LOG(id[i]);
#endif		
		}	
		Sha1.write('\n');
#ifdef DEBUG_HASH
		LOG('\n');
#endif	
	}
	len = strlen(dateString);
	for(int i = 0; i < len; i++) {
		Sha1.write(dateString[i]);
#ifdef DEBUG_HASH
		LOG(dateString[i]);
#endif		
	}
	if(payload!=NULL) {	
		Sha1.write('\n');
#ifdef DEBUG_HASH
		LOG('\n');
#endif	
		len = strlen(payload);
		for(int i = 0; i < len; i++) {
			Sha1.write(payload[i]);
#ifdef DEBUG_HASH
			LOG(payload[i]);
#endif		
		}
	}
	b64_encode(Sha1.resultHmac(), HMAC_SHA1_HASH_LENGTH_BINARY, (unsigned char *)buffer, HMAC_SHA1_HASH_LENGTH_CODE64); 
	//base64_encode((char *)Sha1.resultHmac(),(char *)buffer, HMAC_SHA1_HASH_LENGTH_BINARY); 
	buffer[HMAC_SHA1_HASH_LENGTH_CODE64+1] = '\0';
#ifdef DEBUG_HASH
	LOGLN();
	LOGLN(F("createHashSignature(): Resulting hash:"));
	LOGLN(buffer);
#endif	
}

/** 
 *	@brief Create date string.
 * 	
 *	This function creates a standard UTC date/time string in the
 *	right format to be sent as Date header in an HTTP communication. 
 * 
 *	@return nothing
 */
void getDateString(
		   char *UTCTime /*!< [out] Pointer to the buffer where UTC time string will be stored (fixed length, 30 bytes).*/){
	LOGLN(F("getDateString(&buffer)"));	
	strcpy_P(UTCTime, UTCBase);
	char aux[5] = "";
	strncpy_P(&UTCTime[0], &weekdays[weekday()-1][0], 3);
	itoa(day(), aux, 10);
	strncpy(&UTCTime[6-(strlen(aux)-1)], aux, strlen(aux));
	strncpy_P(&UTCTime[8], &months[month()-1][0], 3);
	itoa(year(), aux, 10);
	strncpy(&UTCTime[12], aux, strlen(aux));
	itoa(hour(), aux, 10);
	strncpy(&UTCTime[18-(strlen(aux)-1)], aux, strlen(aux));
	itoa(minute(), aux, 10);
	strncpy(&UTCTime[21-(strlen(aux)-1)], aux, strlen(aux));
	itoa(second(), aux, 10);
	strncpy(&UTCTime[24-(strlen(aux)-1)], aux, strlen(aux));
	LOG(F("getDateString(): "));
	LOGLN(UTCTime);
}

/** 
 *	@brief Parse date string.
 * 	
 *	This function parses a standard UTC date/time string recived in the
 *	format of a Date header in an HTTP communication. 
 *	Calculus are based on the following constants:
 *		1 minute = 60 seconds
 *		1 hour = 3600 seconds
 *		1 day = 86400 seconds
 *		1 week = 604800 seconds
 *		1 month (30.44 days) = 2629743 seconds
 *		1 year (365.24 days) = 31556926 seconds
 * 
 *	@return nothing
 */
long parseDateString(
		     char *UTCTime /*!< [out] Pointer to the buffer with a UTC time string (fixed length, 30 bytes).*/){
	LOG(F("parseDateString(\""));	
	LOG(UTCTime);
	LOGLN(F("\")"));
	//Base string: "Mon, 01 Jan 1970 00:00:00 GMT"
	unsigned int day = 0, month = 0, year = 0, hour = 0, minute = 0, second = 0;
	char buffer[5];
	//Serial.println(UTCTime);
	//Parse day
	buffer[0] = UTCTime[5];
	buffer[1] = UTCTime[6];
	buffer[2] = '\0';
	day = atoi(buffer);
	//parse hour
	buffer[0] = UTCTime[17];
	buffer[1] = UTCTime[18];
	hour = atoi(buffer);
	//parse minute
	buffer[0] = UTCTime[20];
	buffer[1] = UTCTime[21];
	minute = atoi(buffer);
	//parse second
	buffer[0] = UTCTime[23];
	buffer[1] = UTCTime[24];
	second = atoi(buffer);
	//parse year
	buffer[0] = UTCTime[12];
	buffer[1] = UTCTime[13];
	buffer[2] = UTCTime[14];
	buffer[3] = UTCTime[15];
	buffer[4] = '\0';
	year = atoi(buffer);
	//Parse month
	buffer[0] = UTCTime[8];
	buffer[1] = UTCTime[9];
	buffer[2] = UTCTime[10];
	buffer[3] = '\0';
	int i;
	for(i=0; i<12; i++) {
		//convertFlashStringToMemString(month_, );
		if(!strcmp_P(buffer, &months[i][0])) break; 
	}
	month = i;
	long t = (year-1970) * 31556926 + month * 2629743 + day * 86400 + hour * 3600 + minute * 60 + second - 86400;
	LOG(F("parseDateString(): "));	
	LOG(t);
	LOGLN(F(" seconds from January 1st 1970"));
	return t;
}

//Functions to convert from hexadecimal to ASCII and viceversa

void hexToAscii(char *hex, char *ascii, unsigned int len) {
	byte conv[23] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1, -1, -1, -1, -1, -1, -1, 10, 11, 12, 13, 14, 15};
	int j = 0;
	for (int i = 0; i < 2*len; i += 2) {
		int k = hex[i]>70? hex[i]-'0'-32 : hex[i]-'0';
		int l = hex[i+1]>70? hex[i+1]-'0'-32 : hex[i+1]-'0';    
		ascii[j++] = conv[k] * 16 + conv[l];
	}
	ascii[len] = '\0';
}

void asciiToHex(char *ascii, char *hex, unsigned int len) {
	char inv[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
	int j=0;
	for (int i=0; i<len; i++) {
		hex[j++] = inv[ascii[i]/16];
		hex[j++] = inv[ascii[i]&0x0F];
	}
	hex[2*len] = '\0'; 
}

unsigned long elapsedMillis(
			    unsigned long m /*!< [in] Start time in millis. */) {
	unsigned long now = millis();
	if (now>=m) return (now-m);
	else return ((0xFFFFFFFF - m) + now);
}

