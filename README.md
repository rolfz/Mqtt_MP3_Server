# Mqtt MP3 Player based on Wemos-D1 ESP8266 circuit and VS1053 audio module

## Functionality:

	This module was created to be controlled by Node-Red over a MQTT Broker.
	The module understands various tags such as:
			### /Zaudio/play  "file to play with extension" 8.3 format
			### /Zaudio/volume xxx (volume in db) 0 to 100db is possible
			### /Zaudio/command stop (will stop the file played)
			
	The module also allows to connect a Neoled display module with 7 leds
			### //Zaudio/neoled #xxyyzz  (set color in hex rgb format

## Hardware:
	###Wemos D1 module, SPI bus used to communicate with audio module
	
	###Adafruit VS1053 module with 3W amplifier PRODUCT ID: 3436
	
	###Micro SD card 128mB our larger FAT format.
		
## Software Libraries:

	### Adafruit_Neopixel
	### Adafruit_VS1053_Library-master
	### pubsubclient (MQTT library)
	
## Software Main file:
	### mqtt_mp3_server.ino contains all customised code based on the MQTT client example
	### wifi_settings.h 	contains the WIFI private setting
	
	