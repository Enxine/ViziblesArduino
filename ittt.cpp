// (c) Copyright 2016 Enxine DEV S.C.
// Released under Apache License, version 2.0
//
// This library is used to send data froma simple sensor 
// to the Vizibles IoT platform in an easy and secure way.

#include <ViziblesArduino.h>
#include "strings.h"
#include "ittt.h"
#include "utils.h"
#include <ArduinoJson.h>
#include <aes256.h>
#include <HttpClient.h>

#define ITTT_BUFFER_THINGID_MARK 	255
#define ITTT_BUFFER_ADDRESS_MARK 	254
#define ITTT_BUFFER_TICKET_MARK 	253
#define ITTT_BUFFER_RULE_MARK	 	252



//Variables
static HttpClient *httpClientForITTT = NULL;						/*!< HTTP client used in all outgoing communications.*/
static char *thingIdForITTT = NULL;									/*!< Thing ID of this thing, need when calling functions in other things.*/ 
static unsigned char itttRulesBuffer[MAX_ITTT_BUFFER_LENGTH] = ""; 	/*!< Buffer where rules and things information is stored.*/ 
static unsigned int lastBufferChar = 0;								/*!< Index of the first possition available in the buffer.*/ 

/** 
 *	@brief Initialize ITTT system.
 * 	
 *	This function initializes the ITTT rules engine with the information it needs for working.
 * 
 *	@return nothing
 */
void initITTT(
              HttpClient *http, 	/*!< [in] HTTP client to use in the communications.*/
              char *thingId		/*!< [in] Full thing ID of this thing (pointer to char array).*/ ) {
	httpClientForITTT = http;
	thingIdForITTT = thingId;
}

/** 
 *	@brief Print ITTT buffer content.
 * 	
 *	This function prints the contet of the local ITTT rules buffer to console.
 * 
 *	@return nothing
 */
void printBuffer(void) {
	for(int i = 0; i < lastBufferChar; i++) printChar(itttRulesBuffer[i]);
	Serial.println();
}

/** 
 *	@brief Initialize ITTT rules.
 * 	
 *	This function initializes the content of ITTT rules' storage.
 * 
 *	@return nothing
 */
void initITTTRules(void) {
	lastBufferChar = 0;
}
 
/** 
 *	@brief Search for thing ID.
 * 	
 *	This function search the ITTT rules buffer for a particular thing ID.
 * 
 *	@return The position of the thing ID string in the buffero or -1 if not found 
 */
int itttBufferSearchThingId(
                            char *thingId /*!< [in] Thing ID string to search.*/) {
	int i = 0;
	char *_thingId;
	char empty[] = "";
	if(!strcmp(thingId, thingIdForITTT)) _thingId = empty;
	else _thingId = thingId;
	while(i<lastBufferChar) {
		if(itttRulesBuffer[i]==ITTT_BUFFER_THINGID_MARK) {
			if(!strcmp(_thingId, (char *)&itttRulesBuffer[i+1])) {
				i++;
				//Serial.print("ThingId found: ");
				//Serial.println((char *)&itttRulesBuffer[i]);
				break;
			}	
			else i += strlen((char *)&itttRulesBuffer[i+1]) + 1;
		} else {
			if(itttRulesBuffer[i]==ITTT_BUFFER_ADDRESS_MARK || itttRulesBuffer[i]==ITTT_BUFFER_TICKET_MARK || itttRulesBuffer[i] == ITTT_BUFFER_RULE_MARK ) i += 3;
			else i++;
		}	
	}
	if (i==lastBufferChar) return -1;
	else return i;
}

/** 
 *	@brief Add thing ID to buffer.
 * 	
 *	This function adds a new thing identifier to the buffer if the same thing ID is 
 *	not already in the buffer.
 * 
 *	@return The position of the thing ID string in the buffer or -1 if the buffer is full 
 */
int itttBufferAddThingId(
                         char *thingId	/*!< [in] Thing ID string to add.*/) {
	int i = itttBufferSearchThingId(thingId);
	if(i!=-1) return i;
	unsigned int len = strlen(thingId);
	if((lastBufferChar+len+2) >= MAX_ITTT_BUFFER_LENGTH) return -1;
	itttRulesBuffer[lastBufferChar] = ITTT_BUFFER_THINGID_MARK;
	if(!strcmp(thingId, thingIdForITTT)) {
		itttRulesBuffer[lastBufferChar+1] = '\0';
		len = 0;
	} else {
		strcpy((char *)&itttRulesBuffer[lastBufferChar+1], thingId);
	}
	int pos = lastBufferChar+1;
	lastBufferChar += len + 2;
	return pos;
}

/** 
 *	@brief Search thing ID attribute.
 * 	
 *	This function search for a particular atribute related with a thing (ID) in the ITTT buffer. 
 *	If the param provided is found and is different form the provided one it will be deleted, since
 *	those parameters must be unique in the buffer and thsi funcion is intended to be used only before
 *	storing a new parameter. For retrieving data related with a thing use itttGetThingIdAttributes.
 * 
 *	@return The position of the related thing ID string in the buffer or -1 if thing id is not found or the attribute does not exist 
 */
int itttBufferSearchThingIdRealtedAttribute(
                                            char *thingId,		/*!< [in] Thing ID string related with the attribute.*/
                                            unsigned char mark,	/*!< [in] Type of attribute from the possibilities defined.*/
                                            char *att			/*!< [in] Attribute content.*/) {
	if(!strcmp(thingId, thingIdForITTT)) return -1;
	int i = itttBufferSearchThingId(thingId);
	if(i==-1) return -1;
	int thingIdIndex = i;
	char a = thingIdIndex && 0xFF;
	char b = (thingIdIndex & 0xFF00)>>8;
	i += strlen((char *)&itttRulesBuffer[i]);
	while(i<lastBufferChar) {
		if(itttRulesBuffer[i]==mark && itttRulesBuffer[i+1]==a && itttRulesBuffer[i+2]==b) {
			i+=3;
			//Serial.print("Param found: ");
			//Serial.println((char *)&itttRulesBuffer[i]);
			break;
		} else {
			i++;
		}	
	}
	if (i==lastBufferChar) return thingIdIndex;
	else {
		if(!strcmp((char *)&itttRulesBuffer[i], att)) return -1;
		else {
			//Delete old param if new is an update
			int k = i - 3; 
			for(int j = i + strlen((char *)&itttRulesBuffer[i]) + 1; j < lastBufferChar; j++) {
				itttRulesBuffer[k] = itttRulesBuffer[j];
				k++;
			}
			lastBufferChar = k;
			return thingIdIndex;
		}	
	}	
}

/** 
 *	@brief Add thing ID attribute.
 * 	
 *	This function adds a new atribute to a thing ID already present in the buffer.
 * 
 *	@return Zero if everything goes right or -1 if a param with the same value is in the buffer, thing ID is not found or no space available 
 */
int itttBufferAddThingIdRealtedAttribute(
                                         char *thingId,		/*!< [in] Thing ID string related with the attribute.*/
                                         unsigned char mark,	/*!< [in] Type of attribute from the possibilities defined.*/
                                         char *att			/*!< [in] Attribute content.*/) {
	if(!strcmp(thingId, thingIdForITTT)) return 0;
	unsigned int len = strlen(att);
	int i = itttBufferSearchThingIdRealtedAttribute(thingId, mark, att);
	if(i==-1) {
		//Serial.println("Param repeated, discard");
		return -1;
	}	
	if((lastBufferChar+len+3) >= MAX_ITTT_BUFFER_LENGTH) return -1;
	itttRulesBuffer[lastBufferChar] = mark;
	itttRulesBuffer[lastBufferChar+1] = i & 0xFF;
	itttRulesBuffer[lastBufferChar+2] = (i & 0xFF00)>>8;
	strcpy((char *)&itttRulesBuffer[lastBufferChar+3], att);
	lastBufferChar += len + 4;
	//printBuffer();
	return 0;
}

/** 
 *	@brief Search for a rule.
 * 	
 *	This function searchs for a particular rule in the ITTT buffer, identified by rule ID.
 * 
 *	@return The postion of the ID string of the rule in the buffer or -1 if not found. 
 */
int itttBufferSearchRule(
                         char *id	/*!< [in] Rule identifier to search.*/) {
	int i = 0;
	while(i<lastBufferChar) {
		if(itttRulesBuffer[i]==ITTT_BUFFER_RULE_MARK) {
			if(!strcmp(id, (char *)&itttRulesBuffer[i+4])) {
				i+=4;
				//Serial.print("Rule found: ");
				//Serial.println((char *)&itttRulesBuffer[i]);
				break;
			}	
			else {
				i += strlen((char *)&itttRulesBuffer[i+4]) + 5;
				i += strlen((char *)&itttRulesBuffer[i]) + 1;
				i += strlen((char *)&itttRulesBuffer[i]) + 1;
				i += strlen((char *)&itttRulesBuffer[i]) + 1;
			}	
		} else {
			if(itttRulesBuffer[i]==ITTT_BUFFER_ADDRESS_MARK || itttRulesBuffer[i]==ITTT_BUFFER_TICKET_MARK) i += 3;
			else i++;
		}	
	}
	if (i==lastBufferChar) return -1;
	else return i;
}

/** 
 *	@brief Add a rule.
 * 	
 *	This function adds a new rule to the ITTT buffer.
 * 
 *	@return Zero if everything goes right or -1 if no space is available to add the new rule. 
 */
int itttBufferAddRule(
                      char *id,					/*!< [in] Rule identifier.*/
                      char *variable, 			/*!< [in] Variable name of the condition of the rule.*/
                      itttCondition_t condition, 	/*!< [in] Condition of the rule.*/
                      char *value,				/*!< [in] trigger value of the variable.*/
                      char *thingId,				/*!< [in] Identifier of the target thing of the rule.*/
                      char *function				/*!< [in] Name of the function to call in the target thing.*/) {
	int thingIdIndex = itttBufferAddThingId(thingId);
	if (thingIdIndex == -1 || (lastBufferChar + 8 + strlen(id) + strlen(variable) + strlen(value) + strlen(function)) >= MAX_ITTT_BUFFER_LENGTH) return -1;
	if (itttBufferSearchRule(id)!=-1) return -1; 
	unsigned int i = lastBufferChar;
	itttRulesBuffer[i++] = ITTT_BUFFER_RULE_MARK;
	itttRulesBuffer[i++] = thingIdIndex & 0xFF;
	itttRulesBuffer[i++] = (thingIdIndex & 0xFF00)>>8;
	itttRulesBuffer[i++] = (unsigned char) condition;
	strcpy((char *)&itttRulesBuffer[i], id);
	i += strlen(id) + 1;
	strcpy((char *)&itttRulesBuffer[i], variable);
	i += strlen(variable) + 1;
	strcpy((char *)&itttRulesBuffer[i], value);
	i += strlen(value) + 1;
	strcpy((char *)&itttRulesBuffer[i], function);
	i += strlen(function) + 1;
	lastBufferChar = i;
	//printBuffer();
	return 0;
}

/** 
 *	@brief Delete a rule.
 * 	
 *	This function removes a rule from the buffer. It also checks if thre are still more rules
 *	related with the same thing, and if not removes also the information related with the thing, 
 *	like thing ID, address and ticket.
 * 
 *	@return Zero if everything goes right or -1 if the rule is not found. 
 */
int itttBufferDeleteRule (
                          char *id	/*!< [in] Rule identifier to delete.*/) {
	int i = itttBufferSearchRule(id);
	if(i==-1) return -1;
	int k = i + 4;
	int count = 0;
	while(k < lastBufferChar && count < 4) {
		if(itttRulesBuffer[k]=='\0') count++; 
		k++;
	}
	//Keep Thing id index to later check if this was the last rule related with this thing ID
	char a = itttRulesBuffer[i-3] && 0xFF;
	char b = (itttRulesBuffer[i-2] & 0xFF00)>>8;

	int l = i - 4;
	for(int j = k; j < lastBufferChar; j++) {
		itttRulesBuffer[l] = itttRulesBuffer[j];
		l++;
	}
	lastBufferChar = l;
	//printBuffer();
	//Search for other rules related with the same thing ID
	i = 0;
	int found = 0;
	while(i<lastBufferChar) {
		if(itttRulesBuffer[i]==ITTT_BUFFER_RULE_MARK && itttRulesBuffer[i+1]==a && itttRulesBuffer[i+2]==b) {
			//i+=3;
			found = 1;
			break;
		} else {
			if(itttRulesBuffer[i]==ITTT_BUFFER_ADDRESS_MARK || itttRulesBuffer[i]==ITTT_BUFFER_TICKET_MARK) i+=3;
			else i++;
		}	
	}
	if (found) return 0;
	int thingIdIndex = a + (b<<8) - 1;
	//Delete all attributes related with this thing id
	for (int j = thingIdIndex + strlen((char *)&itttRulesBuffer[thingIdIndex+1]); j < lastBufferChar; j++) {
		if((itttRulesBuffer[j]==ITTT_BUFFER_ADDRESS_MARK || itttRulesBuffer[j]==ITTT_BUFFER_TICKET_MARK) && itttRulesBuffer[j+1]==a && itttRulesBuffer[j+2]==b) {
			l = j;
			for (int k = j + 3 +strlen((char *)&itttRulesBuffer[j+3]); k < lastBufferChar; k++) {
				itttRulesBuffer[l] = itttRulesBuffer[k];
				l++;
			};
			lastBufferChar = l-1;
		}
	};
	//printBuffer();
	//Delete thing ID
	l = thingIdIndex;
	for (int j = thingIdIndex + 2 +strlen((char *)&itttRulesBuffer[thingIdIndex+1]); j < lastBufferChar; j++) {
		itttRulesBuffer[l] = itttRulesBuffer[j];
		l++;
	};
	lastBufferChar = l;
	//printBuffer();
	return 0;
}

/** 
 *	@brief Get the number of active rules.
 * 	
 *	This function counts the number of rules available in the ITTT buffer and returns their number.
 * 
 *	@return The number of active rules. 
 */
int itttGetNumberOfActiveRules(void) {
	int count = 0;
	for (int i = 0; i < lastBufferChar; i++) {
		switch(itttRulesBuffer[i]) {
		case ITTT_BUFFER_RULE_MARK:
			count++;
			i += 3;
			break;
		case ITTT_BUFFER_ADDRESS_MARK:
		case ITTT_BUFFER_TICKET_MARK:
			i +=2;
			break;
		}
	}
	return count;
}

/** 
 *	@brief Get the number of active things.
 * 	
 *	This function counts the number of thing identifiers in the ITTT buffer and returns their number.
 * 
 *	@return The number of active things. 
 */
int itttGetNumberOfActiveThingIds(void) {
	int count = 0;
	for (int i = 0; i < lastBufferChar; i++) {
		switch(itttRulesBuffer[i]) {
		case ITTT_BUFFER_THINGID_MARK:
			count++;
			break;
		case ITTT_BUFFER_ADDRESS_MARK:
		case ITTT_BUFFER_TICKET_MARK:
			i +=2;
			break;
		case ITTT_BUFFER_RULE_MARK:
			i +=3;
			break;
		}
	}
	return count;
}

/** 
 *	@brief Get a particular rule.
 * 	
 *	This function looks for the rule number n in the ITTT buffer and returns all its atributes in
 *	a itttRule_t object passed as parameter. The use of this function is intended for a loop consulting
 *	all available rules, since rule number depends only ine the order the rules were stored.
 * 
 *	@return A pointer to an itttRule_ object with all attributes of the rule or NULL if no rule is available. 
 */
itttRule_t *itttGetRule(
                        int n,			/*!< [in] Number of the rule to be retrieved.*/
                        itttRule_t *buf	/*!< [in] Empty itttRule_t object to store information.*/) {
	int count = 0;
	int i = 0;
	int found = 0;
	while(i < lastBufferChar && !found) {
		switch(itttRulesBuffer[i]) {
		case ITTT_BUFFER_RULE_MARK:
			count++;
			if(count==n) found = 1;
			else i += 4;
			break;
		case ITTT_BUFFER_ADDRESS_MARK:
		case ITTT_BUFFER_TICKET_MARK:
			i +=2;
			break;
		default:
			i++;
		}
	}
	if(found) {
		buf->condition = (itttCondition_t) itttRulesBuffer[i+3];
		int thingIdIndex = itttRulesBuffer[i+1] + (itttRulesBuffer[i+2]<<8);
		if(itttRulesBuffer[thingIdIndex]=='\0') buf->thingId = (char *)thingIdForITTT;
		else buf->thingId = (char *)&itttRulesBuffer[thingIdIndex];
		int k = 4;
		buf->id = (char *)&itttRulesBuffer[i+k];
		k += strlen(buf->id) + 1;
		buf->variable = (char *)&itttRulesBuffer[i+k];
		k += strlen(buf->variable) + 1;
		buf->value = (char *)&itttRulesBuffer[i+k];
		k += strlen(buf->value) + 1;
		buf->function = (char *)&itttRulesBuffer[i+k];
		return buf;
	} else return NULL; 
}

/** 
 *	@brief Get a particular thing identifier.
 * 	
 *	This function looks for the thing ID number n in the ITTT buffer and returns a pointer to its name.
 *	The use of this function is intended for a loop consulting all available thing identifiers, since 
 *	thing ID number depends only ine the order the thing IDs were stored.
 * 
 *	@return A pointer to the thing ID or NULL if no thing ID is available. 
 */
char *itttGetThingId(
                     int n /*!< [in] Number of the thing ID to be retrieved.*/) {
	int count = 0;
	int i = 0;
	int found = 0;
	while(i < lastBufferChar && !found) {
		switch(itttRulesBuffer[i]) {
		case ITTT_BUFFER_THINGID_MARK:
			count++;
			if(count==n) found = 1;
			else i++;
			break;
		case ITTT_BUFFER_ADDRESS_MARK:
		case ITTT_BUFFER_TICKET_MARK:
			i +=2;
			break;
		case ITTT_BUFFER_RULE_MARK:
			i += 4;
			break;	
		default:
			i++;
		}
	}
	if(found) {
		if(itttRulesBuffer[i+1]=='\0') return thingIdForITTT;
		else return (char *)&itttRulesBuffer[i+1];
	} else return NULL; 
}

/** 
 *	@brief Get all attributes of a particular thing.
 * 	
 *	This function looks for the thing ID passed as parameter in the ITTT buffer and returns a pointer to a
 *	itttThing_t object with all attributes related with this thing.
 * 
 *	@return A pointer to a itttThing_t object or NULL if the thing ID is not found. 
 */
itttThing_t *itttGetThingIdAttributes(
                                      char *thingId,		/*!< [in] Thing identifier for which we want to retrieve information.*/
                                      itttThing_t *buf	/*!< [in] Empty itttThing_t object to store information.*/) {
	if(!strcmp(thingId, thingIdForITTT)) return NULL;
	int i = itttBufferSearchThingId(thingId);
	if(i==-1) return NULL;
	int thingIdIndex = i;
	char a = thingIdIndex && 0xFF;
	char b = (thingIdIndex & 0xFF00)>>8;
	i += strlen((char *)&itttRulesBuffer[i]);
	int count = 0;
	while(i<lastBufferChar) {
		if((itttRulesBuffer[i]==ITTT_BUFFER_ADDRESS_MARK || itttRulesBuffer[i]==ITTT_BUFFER_TICKET_MARK) && itttRulesBuffer[i+1]==a && itttRulesBuffer[i+2]==b) {
			//Serial.print("Param found: ");
			//Serial.println((char *)&itttRulesBuffer[i]);
			if(itttRulesBuffer[i]==ITTT_BUFFER_ADDRESS_MARK) {
				buf->address = (char *)&itttRulesBuffer[i+3];
				count++;
				if (count==2) break;
			}	
			if(itttRulesBuffer[i]==ITTT_BUFFER_TICKET_MARK) {
				buf->ticket = (char *)&itttRulesBuffer[i+3];
				count++;
				if (count==2) break;
			}	
			i+=3;
		} else {
			i++;
		}	
	}
	buf->thingId = (char *)&itttRulesBuffer[thingIdIndex];
	return buf;
}

/** 
 *	@brief Parse and add rule.
 * 	
 *	This function parses the received message regarding to new ITTT rules
 *	and adds the resulting rule to the system, so it can be checked and 
 *	executed when needed. It is intended for rules whose conditions can be
 *	checked localy and the execution is calling funcions in things on 
 *	local network.
 * 
 *	@return 0 (false) if any error in the rules parameters or 1 (true) if everything was ok
 */
int addITTTRule(
		JsonObject& rule /*!< [in] Parsed JSON object describing an ITTT rule.*/) {
	itttCondition_t ruleCondition;
	convertFlashStringToMemString(itttIf, _if);
	convertFlashStringToMemString(itttThen, _then);
	convertFlashStringToMemString(optionsId, _id);
	//Parse if clausules
	if(!rule.containsKey((const char*)_if) || !rule.containsKey((const char*)_then) || !rule.containsKey((const char*)_id)) return 0; 
	if(strlen((const char *)rule[(const char*)_id].asString())!=ITTT_ID_LENGTH) return 0;	
	if(strlen((const char *)rule[(const char*)_if][0].asString())>=MAX_ITTT_VARIABLE_NAME_LENGTH) return 0;	
	if(!strcmp_P((const char *)rule[(const char*)_if][1].asString(), itttIs)) ruleCondition = IS;
	else if(!strcmp_P((const char *)rule[(const char*)_if][1].asString(), itttIsNot)) ruleCondition = IS_NOT;
	else if(!strcmp_P((const char *)rule[(const char*)_if][1].asString(), itttLessThan)) ruleCondition = LESS_THAN;
	else if(!strcmp_P((const char *)rule[(const char*)_if][1].asString(), itttMoreThan)) ruleCondition = MORE_THAN;
	else return 0;
	if(strlen((const char *)rule[(const char*)_if][2].asString())>=MAX_ITTT_VALUE_LENGTH) return 0;
  
	//Parse then clausules
	if(strcmp_P((const char *)rule[(const char *)_then][0].asString(), itttRunTask)) return 0;
	if(strlen((const char *)rule[(const char *)_then][1].asString())>=MAX_ITTT_THING_ID_LENGTH) return 0;
	if(strlen((const char *)rule[(const char *)_then][2].asString())>=MAX_ITTT_FUNCTION_NAME_LENGTH) return 0;	
	int err = itttBufferAddRule((char *)rule[(const char *)_id].asString() , (char *)rule[(const char *)_if][0].asString(), ruleCondition, (char *)rule[(const char *)_if][2].asString(), (char *)rule[(const char *)_then][1].asString(), (char *)rule[(const char *)_then][2].asString());
	//printBuffer();
	if (err == -1) return 0;
	else return 1;
}

/** 
 *	@brief Delete rule.
 * 	
 *	This function removes a rule form the ITTT rules list, identified by the rule
 *	identifier passed as parameter.
 * 
 *	@return 0 (false) if any error in the rules parameters or 1 (true) if everything was ok
 */
int removeITTTRule(
                   char *id	/*!< [in] Identifier of the rule to be deleted.*/) {
	if(itttBufferDeleteRule(id)==-1) return 0;
	else return 1;
}

/** 
 *	@brief Parse and add addresses.
 * 	
 *	This function parses the received message regarding to new addresses 
 *	related with ITTT rules	and adds the information to the system.
 * 
 *	@return 0 (false) if any error in the rules parameters or 1 (true) if everything was ok
 */
int addITTTAddress(
                   JsonObject& addresses	/*!< [in] Parsed JSON object with addresses of other things in the local network.*/) {
	//Parse addresses
	if(addresses!=JsonObject::invalid()) {
		int n = itttGetNumberOfActiveThingIds();
		int found = 0;
		for (int i = 1; i <= n; i++) {
			char *thingId = itttGetThingId(i);
			if(addresses.containsKey((const char *)thingId) && strlen((const char *)addresses[(const char *)thingId].asString())<MAX_ITTT_ADDRESS_LENGTH) {
				found = 1;
				itttBufferAddThingIdRealtedAttribute(thingId, ITTT_BUFFER_ADDRESS_MARK, (char *)addresses[(const char *)thingId].asString());
				//printBuffer();
			}
		}
		if(found) return 1;
		else return 0;
	} else return 0;
}

/** 
 *	@brief Parse and add tickets.
 * 	
 *	This function parses the received message regarding to new authorization 
 *	tickets for things related with ITTT rules and adds the information to the system.
 * 
 *	@return 0 (false) if any error in the rules parameters or 1 (true) if everything was ok
 */
int addITTTTickets(
                   JsonObject& tickets,	/*!< [in] Parsed JSON object with permission tickets for queriying other things.*/
                   char *key				/*!< [in] Key for decrypting tickets (API key of this thing).*/) {
	//Parse authorization tickets
	if(tickets!=JsonObject::invalid()) {
		int n = itttGetNumberOfActiveThingIds();
		int found = 0;
		for (int i = 1; i <= n; i++) {
			char *thingId = itttGetThingId(i);
			if(tickets.containsKey(thingId) && strlen((const char *)tickets[thingId].asString())==64) {
				found = 1;
				char ticket[33];
				char *ecryptedTicketHex = (char *)tickets[thingId].asString();
				//Convert received hexadecimal byte string into real byte string	
				hexToAscii(ecryptedTicketHex, ticket, 32);
				//Create key from API key secret and add some bytes to complete 32
				char key_[33];
				strcpy(key_, key);
				strcpy_P(&key_[20], itttKeyAddition);
				//Decrypt ticket
				aes256_context ctxt;
				aes256_init(&ctxt, (uint8_t *) key_);
				aes256_decrypt_ecb(&ctxt, (uint8_t *) ticket);
				aes256_decrypt_ecb(&ctxt, (uint8_t *) &ticket[16]);
				aes256_done(&ctxt);
				ticket[26] = '\0';
				//Store ticket
				itttBufferAddThingIdRealtedAttribute(thingId, ITTT_BUFFER_TICKET_MARK, &ticket[6]);
				//printBuffer();
			}
		}
		if(found) return 1;
		else return 0;
	} else return 0;
}

/** 
 *	@brief Evaluate rule condition.
 * 	
 *	This function checks if new variable value must activate rule or not.
 * 
 *	@return 1 (true) if the rule must be activated or 0 (false) if not.
 */
int evaluateCondition(
                      char *variableValue,		/*!< [in] Pointer to variable value as char array.*/
                      char *ruleValue,			/*!< [in] Pointer to rule triger value as char array.*/
                      itttCondition_t condition	/*!< [in] Condition to be checked.*/) {
	if(isNumber(variableValue) && isNumber(ruleValue)) {
		double dVariableValue = atof(variableValue);
		double dRuleValue = atof(ruleValue);
		switch(condition) {
		case IS:
			if(dVariableValue == dRuleValue) return 1;
			else return 0;
			break;
		case IS_NOT:
			if(dVariableValue != dRuleValue) return 1;
			else return 0;
			break;
		case LESS_THAN:
			if(dVariableValue < dRuleValue) return 1;
			else return 0;
			break;
		case MORE_THAN:
			if(dVariableValue > dRuleValue) return 1;
			else return 0;
			break;
		}
	} else {
		switch(condition) {
		case IS:
			if(!strcmp(variableValue, ruleValue)) return 1;
			else return 0;
			break;
		case IS_NOT:
			if(strcmp(variableValue, ruleValue)) return 1;
			else return 0;
			break;
		}		
	}
}

/** 
 *	@brief Execute a function in another thing.
 * 	
 *	It calls the execution of a function performed by other thing in the same network.
 * 
 *	@return zero if everyting is ok or a negative error code if not
 */
int executeFunction (
                     char *fun,		/*!< [in] Name of the function to be executed. */
                     char *hostname,	/*!< [in] Host name of the thing with the function to be executed.*/
                     char *key,		/*!< [in] Authorization key for the remote thing.*/
                     char *id,		/*!< [in] Id of the thing requesting the execution.*/ 
                     char *params[]  /*!< [in] Parameters for the function (NULL terminated array).*/) {
	//TODO: create payload from params list and a task id	
	convertFlashStringToMemString(itttPayload, payload);
	//Get url path
	int pathLen = 5 + strlen(fun); 
	char path[pathLen];
	strcpy_P(path, itttPath);
	strcpy(&path[4], fun);
	//Create content type header
	convertFlashStringToMemString(contentType, headerContentType);
	//Create date header
	char headerDate[36];
	strcpy_P(headerDate, date);
	getDateString(&headerDate[6]); 
	//Create hash signature.
	//Serial.println(id);
	unsigned int idLen = strlen(id);
	char headerAuthorization[56+idLen];
	strcpy_P(headerAuthorization, authorization);
	strcpy(&headerAuthorization[24], id);
	headerAuthorization[24 + idLen] = ':';    	
	convertFlashStringToMemString(itttMethodPost, _post);
	convertFlashStringToMemString(optionsProtocolHttp, _http);
	createHashSignature(&headerAuthorization[24 + idLen + 1], key, _post, _http, hostname, DEFAULT_THING_HTTP_SERVER_PORT, path, headerContentType, NULL, &headerDate[6], payload); 
	char *headers[] = { headerDate, headerAuthorization, NULL };
	return HTTPFastRequest(httpClientForITTT, hostname, DEFAULT_THING_HTTP_SERVER_PORT, path, HTTP_METHOD_POST, headerContentType, payload, headers, NULL, 0);
}

/** 
 *	@brief Check if any value trigers any ITTT rule.
 * 	
 *	This function explores the list of key-value pairs passed as parameter 
 *	to check if any of the key names is included in any of the active rules.
 *	And if so, checks if the new value trigers the condition against the 
 *	value stored in the rule.
 * 
 *	@return the number of executed rules.
 */
int testITTTRules(
                  keyValuePair values[],			/*!< [in] Array of key-value pairs to check against active rules (NULL terminated).*/
                  char *meta,						/*!< [in] Buffer to store _Meta parameter content, i.e. list of executed rules.*/
                  ViziblesArduino *client	/*!< [in] Client object to be able to execute local functions.*/) {
	if(thingIdForITTT==NULL || httpClientForITTT==NULL) return 0;
	int i = 0;
	strcpy_P(meta, itttMetaStart);
	int rules = itttGetNumberOfActiveRules(); 
	int count = 0;
	while(values[i].name) {
		for(int j = 1; j <= rules; j++) {
			itttRule_t rule;
			if(itttGetRule(j, &rule)!=NULL) {
				if(!strcmp(rule.variable, values[i].name)) { //Check if updated variable is in the condition of any active rule
					if(evaluateCondition(values[i].value, rule.value, rule.condition)) { //Check if new value acomplishes the condition
						//Execute then clausule
						int err = 1;
						if(!strcmp(rule.thingId, thingIdForITTT)) {
							//Serial.println("Execute function of myself");
							//Include function name as the first parameter
							char *values[2];
							values[0] = rule.function;
							values[1] = NULL;
							//TODO include extra parameters
							//Execute function
							err = client->execute(rule.function, (const char**)values);
						} else {
							itttThing_t thing;
							if(itttGetThingIdAttributes(rule.thingId, &thing)!=NULL) {
								//Serial.print("Executing external function ");
								//Serial.print(rule.function);
								//Serial.print(" on thing ");
								//Serial.println(rule.thingId);
								err = executeFunction(rule.function, thing.address, thing.ticket, thingIdForITTT, NULL);
								//Serial.println("Done");
							}
						}
						if(!err) { //If function was executed inform the cloud with rule id to avoid repeating the execution
							count++;
							if(count==1){
								strcpy_P(&meta[15], itttMeta_1);
								strcpy(&meta[16], rule.id);
								strcpy_P(&meta[28], itttMeta_1);
							} else {	
								strcpy_P(&meta[29 + ((count-2) * 15)], itttMeta_2); 
								strcpy(&meta[31 + ((count-2) * 15)], rule.id);
								strcpy_P(&meta[43 + ((count-2) * 15)], itttMeta_1); 	
							}
						}
					}
				}
			}	
		}
		i++;
	}
	strcpy_P(&meta[44 + ((count-2) * 15)], itttMetaEnd); 
	return count;
}

