// Specifically for use with the Adafruit Feather, the pins are pre-set here!
/* commands:
 *  s: start/stop
 *  p: pause/resume
 */

// include SPI, MP3 and SD libraries
#include <SPI.h>
#include <SD.h>
#include <Adafruit_VS1053.h>
// mqtt related libraries
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include "wifi_settings.h"
const int mqttPort = 1883;
const char* mqttUser = "YourMqttUser";
const char* mqttPassword = "YourMqttUserPassword";

char playfile[15];
char volume[5];
char command[10];
//#define ESP8266

// These are the pins used
#define VS1053_RESET   -1     // VS1053 reset pin (not used!)

// Feather ESP8266
#if defined(ESP8266)
  #define VS1053_CS      15 //16     // VS1053 chip select pin (output)
  #define VS1053_DCS     16 //15     // VS1053 Data/command select pin (output)
  #define CARDCS          2     // Card chip select pin
  #define VS1053_DREQ     0     // VS1053 Data request, ideally an Interrupt pin

// Feather ESP32
#elif defined(ESP32)
  #define VS1053_CS      32     // VS1053 chip select pin (output)
  #define VS1053_DCS     33     // VS1053 Data/command select pin (output)
  #define CARDCS         14     // Card chip select pin
  #define VS1053_DREQ    15     // VS1053 Data request, ideally an Interrupt pin

#endif

Adafruit_VS1053_FilePlayer musicPlayer =
  Adafruit_VS1053_FilePlayer(VS1053_RESET, VS1053_CS, VS1053_DCS, VS1053_DREQ, CARDCS);


 WiFiClient espClient;
 PubSubClient mqttClient(espClient);

void setup() {
  Serial.begin(115200);

// connect to wifi network and mqtt server
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");

  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(callback);

  while (!mqttClient.connected()) {
    Serial.println("Connecting to MQTT...");

//    if (client.connect("ESP8266Client", mqttUser, mqttPassword )) {
    if (mqttClient.connect("ESP8266Client")) {

      Serial.println("connected");

    } else {

      Serial.print("failed with state ");
      Serial.print(mqttClient.state());
      delay(2000);

    }
 }
 mqttClient.publish("/Zaudio", "Hello from MQTT MP3 Servre");
 mqttClient.subscribe("/Zaudio/play"); // we need the audio file name
 mqttClient.subscribe("/Zaudio/volume"); // for volume changes
 mqttClient.subscribe("/Zaudio/command"); // to get the stop command

  // if you're using Bluefruit or LoRa/RFM Feather, disable the BLE interface
  //pinMode(8, INPUT_PULLUP);

  // Wait for serial port to be opened, remove this line for 'standalone' operation
  while (!Serial) { delay(1); }

  Serial.println("\n\nZ-Control MQTT VS1053 based audio player");

  if (! musicPlayer.begin()) { // initialise the music player
     Serial.println(F("Couldn't find VS1053, do you have the right pins defined?"));
     while (1);
  }

  Serial.println(F("VS1053 found"));

  musicPlayer.sineTest(0x44, 500);    // Make a tone to indicate VS1053 is working

  if (!SD.begin(CARDCS)) {
    Serial.println(F("SD failed, or not present"));
    while (1);  // don't do anything more
  }
  Serial.println("SD OK!");

  // list files
  printDirectory(SD.open("/"), 0);

  // Set volume for left, right channels. lower numbers == louder volume!
  musicPlayer.setVolume(50,50);

#if defined(__AVR_ATmega32U4__)
  // Timer interrupts are not suggested, better to use DREQ interrupt!
  // but we don't have them on the 32u4 feather...
  musicPlayer.useInterrupt(VS1053_FILEPLAYER_TIMER0_INT); // timer int
#elif defined(ESP32)
  // no IRQ! doesn't work yet :/
#else
  // If DREQ is on an interrupt pin we can do background
  // audio playing
  musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);  // DREQ int
#endif

  // Play a file in the background, REQUIRES interrupts!
//  Serial.println(F("Playing ready.mp3"));
//  musicPlayer.startPlayingFile("ready.mp3");

} // end of setup

void callback(char* topic, byte* payload, unsigned int length) {

  int i;
//  Serial.print("Message arrived in topic: ");
//  Serial.println(topic);

//------------ decode audio file --------------
  if(strcmp(topic,"/Zaudio/play")==0) {

  Serial.print("Audio: ");
  for (i = 0; i < length; i++) {
         playfile[i] = payload[i];
      }
         playfile[i] = '\0';

    Serial.println(playfile);
    }

//------------decode volume---------------
  if(strcmp(topic,"/Zaudio/volume")==0){

  Serial.print("Volume: ");
  for (i = 0; i < length; i++) {
      volume[i] = payload[i];
    }
      volume[i] = '\0';

      Serial.println(volume);

      musicPlayer.setVolume(atoi(volume),atoi(volume));
  }

//-------------decode command -----------
  if(strcmp(topic,"/Zaudio/command")==0){

  Serial.print("command: ");
  for (i = 0; i < length; i++) {
      command[i] = payload[i];
    }
      command[i] = '\0';

      Serial.println(command);
      if(strcmp(command,"stop")==0)
          musicPlayer.stopPlaying();
  }
}

void loop() {

  static int lock=0; // lock the message at end of playing to avoid multiple messages

  mqttClient.loop();

  // File is playing in the background
    if (musicPlayer.stopped() && lock==0) {
    Serial.println("done");
    mqttClient.publish("/Zaudio/state", "done");
    lock=1; // we block new messages
    }

  if( playfile[0] != '\0'){
      musicPlayer.startPlayingFile(playfile);
      playfile[0] = '\0';
  }

  // sending commands from the UART interface (USB console)

  if (Serial.available()) {
    char c = Serial.read();

    if (c == 'h') {
      Serial.println("\r\n");
      Serial.println("Z-Audio MQTT Server");
      Serial.println("===================");
      Serial.println("MP3 audio files on SD card");
      Serial.println("MQTT address /Zaudio/play xxx.mp3");
      Serial.println("Volume /Zaudio/volume 100-0dB");
      Serial.println("Stop /Zaudio/control stop");
      Serial.println("UART commands");
      Serial.println("1,2,3,4,5 play audio files");
      Serial.println("p = pause");
      Serial.println("s = stop");
      Serial.println("l = list SD content");
      Serial.println("d = dump vs1053 status");
      Serial.println("----------------------\r\n");
    }

    if (c == 'l') {
      Serial.println("\r\n");
      printDirectory(SD.open("/"), 0);
    }

    // if we get an 's' on the serial console, stop!
    if (c == 's') {
      musicPlayer.stopPlaying();
      Serial.println("Stopped");
    }

    if(c == 'd'){
      Serial.println("Memory Dump");
      musicPlayer.dumpRegs();
    }

    // if we get an 'p' on the serial console, pause/unpause!
    if (c == 'p') {
      if (! musicPlayer.paused()) {
        Serial.println("Paused");
        musicPlayer.pausePlaying(true);
      } else {
        Serial.println("Resumed");
        musicPlayer.pausePlaying(false);
      }
    }

    if(c == '1'){
          Serial.println("playing track001");
            lock=0;
      musicPlayer.startPlayingFile("track001.mp3");
    }
    if(c == '2'){
      Serial.println("playing track002");
     lock=0;
      musicPlayer.startPlayingFile("track002.mp3");
    }
    if(c == '3'){
      Serial.println("playing track003");
      lock=0;
      musicPlayer.startPlayingFile("track003.mp3");
    }
    if(c == '4'){
      Serial.println("playing beep-sh");
      lock=0;
      musicPlayer.startPlayingFile("beep-sh.mp3");
    }
    if(c == '5'){
      Serial.println("playing beep-lo");
      lock=0;
      musicPlayer.startPlayingFile("beep-lo.mp3");
    }
  }
  delay(100);
}

/// File listing helper lock=0;
void printDirectory(File dir, int numTabs) {
   while(true) {

     File entry =  dir.openNextFile();
     if (! entry) {
       // no more files
       //Serial.println("**nomorefiles**");
       break;
     }
     for (uint8_t i=0; i<numTabs; i++) {
       Serial.print('\t');
     }
     Serial.print(entry.name());
     if (entry.isDirectory()) {
       Serial.println("/");
       printDirectory(entry, numTabs+1);
     } else {
       // files have sizes, directories do not
       Serial.print("\t\t");
       Serial.println(entry.size(), DEC);
     }
     entry.close();
   }
   Serial.println("\ndone\n");
}
