# Cliente AT para Vizibles

Leer en otro idioma: [English](https://github.com/Enxine/ViziblesArduino/blob/master/examples/AT/README.md), [Español](https://github.com/Enxine/ViziblesArduino/blob/master/examples/AT/README.es.md)

El firmware para un cliente AT es un recurso muy valioso en plataformas sin los recursos necesarios para ejecutar un cliente de red completo con encriptación SSL. En realidad no es más que un conversor de Puerto Serie a Vizibles, pero seguro y muy sencillo de utilizar. Así que si tienes un puerto serie disponible en tu procesador principal y energía suficiente para alimentar un ESP8266 (lo cual no es tan trivial como puede parecer) puedes acceder a [Vizibles](https://vizibles.com)
Para utilizar el cliente AT puedes compilar este ejemplo para tu placa específica, o simplemente descargar y flashear la [imagen precompilada](https://github.com/Enxine/ViziblesArduino/releases/) en tu módulo. Esta versión se ha probado y funciona en los módulos ESP-01 y ESP-WROOM-02, pero debería funcionar en cualquier otro con memoria suficiente, puesto que de los pines solo se utiliza el puerto serie.

## Comandos AT disponibles

####```AT+WIFICONNECT="<SSID>","<contraseña>"\r\n```

Conecta con un punto de acceso WiFi que tenga el SSID y la contraseña proporcionados. Sólo es necesario utilizar este comando una vez. Si se apaga y se vuelve a encender el módulo este volverá a intentar conectar al punto de acceso con las mismas credenciales automáticamente.
Devuelve ```OK``` si tiene éxito o ```ERROR``` si se produce un error. 

####```AT+SETOPTIONS="<nombre de la opción>","<valor de la opción>"[,...]\r\n```

Establece las opciones básicas de comunicación con la plataforma de Vizibles. Cómo mínimo se deben establecer las opciones Thing ID, Key ID, y Key Secret. Por ejemplo: ```AT+SETOPTIONS="id","light-bulb","keyID","<TU_KEY_ID_AQUÍ","keySecret","<TU_KEY_SECRET_AQUÍ>"```
No es posible cambiar las opciones de comunicación una vez conectado. Para cambiar opciones y que el cambio tenga efecto se debe desconectar y volver a conectar con Vizibles.
Devuelve ```OK``` si tiene éxito o ```ERROR``` si se produce un error.

####```AT+CONNECT[="<nombre de la opción>","<valor de la opción>"[,...]]\r\n```

Conecta con la plataforma de Vizibles. Las opciones básicas de comunicación pueden definirse en esta misma orden o llamando antes a ```AT+SETOPTIONS```. Por ejemplo: ```AT+CONNECT="id","light-bulb","keyID","<TU_KEY_ID_AQUÍ","keySecret","<TU_KEY_SECRET_AQUÍ>"```
Devuelve ```OK``` si tiene éxito o ```ERROR``` si se produce un error. Este comando también devuelve ```+CONNECTED``` cuando el sistema se conecta correctamente a la plataforma de Vizibles.

####```AT+UPDATE="<nombre de la variable>","<valor de la variable>"[,...]\r\n```

Actualiza el valor de una variable en la plataforma. Por ejemplo: ```AT+UPDATE="light","on"```
Devuelve ```OK``` si tiene éxito o ```ERROR``` si se produce un error.
 
####```AT+EXPOSE="<nombre de función>"\r\n```

Expone una función para que pueda ejecutarse desde la plataforma. Por ejemplo: ```AT+EXPOSE="lightOn"``` 
Devuelve ```OK``` si tiene éxito o ```ERROR``` si se produce un error. El resultado, cuando se llama a una función expuesta desde la plataforma, es que aparecerá ```+THINGDO=``` en el puerto serie seguido por el nombre de la función y todos los parámetro incluidos en la llamada. Separados por comas y entre comillas. En nuestro ejemplo: ```+THINGDO="lightOn"```

####```AT+DISCONNECT\r\n```

Rompe cualquier comunicación abierta con la plataforma de Vizibles.
Devuelve ```OK``` si tiene éxito o ```ERROR``` si se produce un error. Este comando también devuelve ```+DISCONNECTED``` cuando el sistema se ha desconectado totalmente de la plataforma.

####```AT+GETMAC\r\n```

Devuelve la dirección MAC de la interfaz WiFi. Se utiliza principalmente para que el firmware pueda distinguir unos módulos de otros, y que así pueda crear Thing IDs distintos. La respuesta tendrá la forma ```+MAC=FF:FF:FF:FF:FF:FF``` o no devolverá nada en caso de error.

## Drivers para otras arquitecturas

Si planeas utilizar el cliente AT para añadir conectividad con Vizibles a tu proyecto quizás quieras revisar la lista que viene a continuación en la que iremos poniendo enlaces al los drivers para otras arquitecturas que vayamos creando.

* [Arduino](https://github.com/Enxine/ViziblesArduinoAT)