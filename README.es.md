# Introducción

Esta librería está pensada para conectar sensores y actuadores basados en Arduino a la [plataforma IoT de Vizibles](https://vizibles.com). La mayor parte de la complejidad de las comunicaciones está oculta dentro de la librería, ofreciendo al programador una interfaz simple y fácil de usar que le permite centrarse en desarrollar su aplicación y olvidarse de cosas como la seguridad, el emparejamiento, etc.
La librería puede ser utilizada en proyectos reales y proporciona algunos ejemplos sencillos que están listos para ser ejecutados. Simplemente sigue las instrucciones que encontrarás más abajo para ponerlo todo a funcionar. Bueno, y no olvides registrarte en [Vizibles](https://vizibles.com) si aún no lo has hecho.

Leer en otros idiomas: [English](https://github.com/Enxine/ViziblesArduino/blob/master/README.md), [Espa?ol](https://github.com/Enxine/ViziblesArduino/blob/master/README.es.md) 

# Instalar el IDE de Arduino

Si aún no tienes el entorno de programación de Arduino o simplemente necesitas una actualización usa, por favor, los enlaces e instrucciones que hay [aquí](https://www.arduino.cc/en/main/software) para instalar la última versión.
Una vez tengas el entorno instalado necesitarás también algunas librerías de soporte para placas si estas provienen de terceras partes no soportadas directamente por el equipo de Arduino, como es el caso de las placas basadas en ESP8266 que hemos usado nosotros durante el desarrollo de esta librería y en las que sw basan los ejmplos disponibles. Para instalar las librerías de soporte para ESP8266 usa el menú principal del IDE de Arduino, Archivo, Preferencias, y en el cuadro de texto para URLs adicionales para el gestor de placas añade ```http://arduino.esp8266.com/stable/package_esp8266com_index.json```.

#Instalando las librerías

Para conseguir que esto funcione vas a tener que clonar o descargar una copia de la librería ViziblesArduino a la carpeta libraries de tu IDE de Arduino, pero también tendrás que hacer lo mismo con las siguientes librerías:

- Crypto suite: [https://github.com/spaniakos/Cryptosuite](https://github.com/spaniakos/Cryptosuite)
- Time: [https://github.com/PaulStoffregen/Time](https://github.com/PaulStoffregen/Time)
- HttpClient: [https://github.com/pablorodiz/HttpClient](https://github.com/pablorodiz/HttpClient)
- Arduino web server library: [https://github.com/lasselukkari/aWOT.git](https://github.com/lasselukkari/aWOT.git)
- JSON: [https://github.com/bblanchon/ArduinoJson](https://github.com/bblanchon/ArduinoJson)
- Websockets: [https://github.com/pablorodiz/Arduino-Websocket.git](https://github.com/pablorodiz/Arduino-Websocket.git)

#Arquitecturas soportadas

Este driver está actualmente en desarrollo, por lo que estamos trabajando para adaptarlo a distintas arquitecturas. Vizibles fue diseñado pensando en plataformas con recursos limitados, pero aún así, debido a sus fuertes requisitos en términos de seguridad, es una plataforma que demanda bastantes recursos. Por tanto la librería de soporte de Vizibles tambien lo es, y actualmente la versión en C++ para Arduino está soportada solamente en las siguientes arquitecturas:

- Placas basadas en ESP8266
- Arduino MKR1000

Por ahora sólo soportamos conexiones SSL con la platforma en ESP8266, pero estamos trabajando duro para añadir el soporte en placas basadas en Atmel.
 
#Explorando los ejemplos

En la carpeta de ejemplos hay varios listos para ejecutar. Algunos son específicos para la placa MKR1000, pero la mayoría fueron escritos para la [Espresso Lite 2.0](http://www.espressolite.com/) que es por ahora la placa que podemos recomendar.
Los ejemplos principales son light-bulb y light-switch, que emulan el comportamento de una bombilla y su interruptor correspondiente. En formato Internet de las Cosas, claro. 
Puedes ejecutar ambos ejemplos en dos placas diferentes o si tienes una sóla placa utilicar un script Node en tu ordendor para emular la otra placa. Bueno, o ejecutar sólamente light-bulb y usar los controles de la plataforma para manejarlo. Simplemente compílalo y a divertirte.
Pero bueno, si eres como nosotros y te gusta la diversión seguro que querrás modificar el código, así que ahí va una pequeña explicación.
La primera parte de la magia es instanciar el propio cliente. No es ni mucho menos simple, puesto que por debajo necesita de dos clientes y un servidor de sockets. Todo esto debe crearse fuera de la librería y pasarse como parámetro para mantener la librería independiente de la arquitectura, incluso aunque no se usen directamente.
```
WiFiClient wc;
WiFiClient wc1;
WiFiServer ws(DEFAULT_THING_HTTP_SERVER_PORT);
ViziblesArduino client(wc, wc1);
```
Si quieres usar una conexión SSL con la plataforma cambia la primera línea del bloque de creación de clientes de sockets por 
```
WiFiClientSecure wc;
```
Si no vas a usar comunicaciones directas entre dispositivos en la misma red local puedes utilizar sólo un objeto WiFiClient, usa el mismo identificador en ambos parámetros en la llamada al constructor. En ese caso yo también consideraría eliminar el servidor salvo que uses comunicación HTTP con la plataforma.
El siguiente paso es definir algunos callbacks que usaremos más adelante para posibilitar que la librería nos informe de cuando se conecta, cuando se desconecta o cuando se produce algún error.
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
Después entramos en la función setup donde lo primero que debemos hacer es conectarnos a la red WiFi de nuestra elección
```
	WiFi.begin("YOUR_WIFI_SSID_HERE", "YOUR_WIFI_PASSWORD_HERE");
	int i = 0;
	while (WiFi.status() != WL_CONNECTED && i<5) {   
		delay(500);
		Serial.print(".");
		i++;
	}
```
Recuerda poner aqui las credenciales correctas para tu red. De nuevo este es un trabajo en desarrollo. Esperamos ser capaces en el futuro de conectar los dispositivo con cualquier red WiFi disponible mediante una configuración por defecto con un AP abierto y una aplicación móvil, pero todo esto aún no está listo, así que por ahora hay que usar el método de codificar las credenciales en el código.
Una vez conectados a la red lanzamos el servidor para escuchar conexiones. Lo usaremos más adelante
```
	ws.begin();
```
Y lo último que debemos hacer en el setup es conectar a la [plataforma Vizibles](https://vizibles.com). Para un ejemplo escillo esto está bien, pero para un producto real sería mejor mover la orden de conexión al bucle principal y gestionar las reconexiones en caso de que se detecte una desconexión. En el código podemos ver cómo se pasan las direcciones de las llamadas para informar de esos eventos.
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
La mayoría de las opciones tienen valores por defecto, por lo que sólo es necesario incluir aquellas que sean relevantes o didtintas en nuestra aplicación concreta.
Si se quieren usar conexiones SSL con la plataforma es necesario cambiar el valor de la opción ```"protocol"``` a ```"wss"```.
 
Puede que te estés preguntando que significa ```convertFlashStringToMemString```. Pues bien, almacenar las cadenas en memoria estática es una buena práctica en Arduino para ahorrar espacio en RAM, pero el código para cargar estas cadenas de nuevo en memoria pronto empieza a replicarse por todas partes. Por lo que hemos escrito una macro para hacerlo que puede encontrarse en el fichero ```ViziblesArduino.h```.

Una vez que la cosa está conectada a la [plataforma de Vizibles](https://vizibles.com) sólo necesita monitorizar el pulsador y reportar los cambios si los hubiese, en el caso de light-switch 
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
U ofrecer funciones para encender y apagar la luz en el caso de light-bulb
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
Estos serían los callbacks correspondientes para las llamadas expose de light-bulb:
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
Finalmente hay una tarea adicional que debe hacerse también en el bucle principal, que es consultar si hay conexiones pendientes al servidor, procesarlas, y atender las peticiones si las hubiese
```
	if(ws.hasClient()) { //Check if any client connected to server
		WiFiClient c = ws.available();
		client.process(&c);
	} else client.process(NULL);
```
#Configurar la librería

La librería dispone de algunas opciones para reducir el tamaño del fichero binario resultante o para incrementar la cantidad de cosas que se reportan por el puerto serie. Todas las opciones de compilación se encuentran agrupadas en el fichero ```config.h```.