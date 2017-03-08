# Vizibles AT client

Read this in other languages: [English](https://github.com/Enxine/ViziblesArduino/blob/master/examples/AT/README.md), [Español](https://github.com/Enxine/ViziblesArduino/blob/master/examples/AT/README.es.md)

AT client firmware is valuable resource for platforms with not enough resources for running a full network client with SSL cryptography. It is a Serial-to-Vizibles converter, secure, and very easy to use, so if you have a serial port on your main processor and some power to feed an ESP8266 you can have access to [Vizibles](https://vizibles.com)
For using the AT client you can compile this project for your board, or just download and burn the [precompiled version](https://github.com/Enxine/ViziblesArduino/releases/) in your module. This version is tested to work on ESP-01 and ESP-WROOM-02 modules, but is expected to work in any other with enough memory, since only the serial port is used as hardware resource.  

## AT commands available

####```AT+WIFICONNECT="<SSID>","<password>"\r\n```

Connect to the WiFi access point with the given SSID and password. This command must be used only once. After a power cycle the module will connect automatically to the same access point.
Returns ```OK``` on success or ```ERROR``` on error. 

####```AT+SETOPTIONS="<option name>","<option value>"[,...]\r\n```

Establishes basic communication options for the connection with the Vizibles platform. Minimum required options are a valid thing ID, Key ID, and Key Secret. For example: ```AT+SETOPTIONS="id","light-bulb","keyID","<YOUR_KEY_ID_HERE","keySecret","<YOUR_KEY_SECRET_HERE>"```
It is not possible tho change options while connected. So for changing an option please disconnect first.
Returns ```OK``` on success or ```ERROR``` on error.

####```AT+CONNECT[="<option name>","<option value>"[,...]]\r\n```

Connect to the Vizibles platform. The basic options for the connection can be set on this same command or previously using ```AT+SETOPTIONS```. For example: ```AT+CONNECT="id","light-bulb","keyID","<YOUR_KEY_ID_HERE","keySecret","<YOUR_KEY_SECRET_HERE>"```
Returns ```OK``` on success or ```ERROR``` on error. This command also returns ```+CONNECTED``` when it correctly connects to the Vizibles platform.

####```AT+UPDATE="<variable name>","<variable value>"[,...]\r\n```

Send an update for a variable to the cloud. For example: ```AT+UPDATE="light","on"\r\n```
Returns ```OK``` on success or ```ERROR``` on error.
 
####```AT+EXPOSE="<function name>"\r\n```

Exposes a control function in the Vizibles platform. For example: ```AT+EXPOSE="lightOn"``` 
Returns ```OK``` on success or ```ERROR``` on error. When the function is called from the platform it will result on a ```+THINGDO=``` in the serial port followed by the name of the function and all the provided parameters separated by comas and between quotation marks. In our example: ```+THINGDO="lightOn"```

####```AT+DISCONNECT\r\n```

Break any current connection with the Vizibles platform.
Returns ```OK``` on success or ```ERROR``` on error. This command also returns ```+DISCONNECTED``` when it correctly disconnects from the Vizibles platform.

####```AT+GETMAC\r\n```

Returns the MAC address of the WiFi interface. Mainly used for differentiating among different modules for the Thigh ID name. Returns the MAC address on the format ```+MAC=FF:FF:FF:FF:FF:FF``` or nothing if any error happens.

####```AT+GETIP\r\n```

Returns the MAC address of the WiFi interface once connected to a network. Returns the IP address on the format ```+IP=XXX.XXX.XXX.XXX``` or nothing if any error happens.

####```AT+GMR\r\n```

Returns firmware version running on the module. In our case the result will always include the words ```Vizibles AT```, a version number, and a date. It does exist multiple other versions of firmware for the ESP8266 modules with AT command interfaces, but not related with Vizibles, so this is a good way to know if we flashed the right version.

####```AT+RST\r\n```

Reset the module. The result is the boot sequence of the ESP8266 followed by ```VZ-READY>``` which tells us module is ready to receive our AT commands.

## Drivers for other platforms

If you plan to use this AT client to add Vizibles connectivity to your project, you might want to check the following list, with links to the drivers for using the AT client in other platforms.

* [Arduino](https://github.com/Enxine/ViziblesArduinoAT)

## Changelog

* V 1.1

Added AT+GMR, AT+RST, and AT+GETIP commands.

Resolved a problem causing AT+CONNECT without parameters to fail on previous versions.

Websocket client enhanced to speed up connection.