# Introduction

This library is intended for connecting Arduino based sensors and actuators to the [Vizibles IoT plaform](https://vizibles.com). Most communications complexity is hidden inside the library, so it offers an easy to use interface for the programmer to focus efforts on its application and forget about things like security, pairing, etc.
The library is ready to use with some simple running examples. Just follow the steps below to get everything working. By the way, do not forget to register into Vizibles.
 
# Install the Arduino IDE

If you do not have it yet or your Arduino IDE verion is not up to date please follow the links and instructions [here](https://www.arduino.cc/en/main/software) for installing the latest version.
Once you have the IDE installed you will need to install also the support libraries for the board you will use, if it is not an original Arduino board, as for example, if you are using ESP8266 based boards, like I am doing, yo will need to install the libraries for supporting ESP8266 based boards. In the main IDE window go to File, Preferences, and in the Additional Boards Manager URLs text box write ```http://arduino.esp8266.com/stable/package_esp8266com_index.json```

#Installing the libraries

To make this run you will need to clone or download a copy of the ViziblesSensorClient library to your Arduino IDE libraries' directory, but you will need to tdo also the same with these othe libraries:

- Crypto suite:[https://github.com/spaniakos/Cryptosuite](https://github.com/spaniakos/Cryptosuite)ç
- Time:[https://github.com/PaulStoffregen/Time](https://github.com/PaulStoffregen/Time)
- HttpClient:[https://github.com/pablorodiz/HttpClient] https://github.com/pablorodiz/HttpClient
- Arduino web server library:[https://github.com/lasselukkari/aWOT.git](https://github.com/lasselukkari/aWOT.git)
- JSON: [https://github.com/bblanchon/ArduinoJson](https://github.com/bblanchon/ArduinoJson)
- Websockets: [https://github.com/pablorodiz/Arduino-Websocket.git](https://github.com/pablorodiz/Arduino-Websocket.git)

#Supported architerctures

This driver is a work in progress, so we are currently working on adapting it to new architectures. Vizibles, even it was designed with constrained resources in mind is a demmanding platform with strong security requirements. So the libray is also quite demanding. Currently the Arduino (C++) version of the library is supported on the following architectures:
- ESP8266 based boards
- Arduino MKR1000

#Exploring the examples

In the examples folder there are some ready to run. Some are specific for the MKR1000 board, but most were writen for the [Espresso Lite 2.0](http://www.espressolite.com/) which is the platorm we can recomend by now.
Main examples are light-bulb and light-switch, that emulate a light and its corresponding switch. 
you can run both examples on two different boards or run a script to emulate one of the parts in your computer. Just compile and have fun 