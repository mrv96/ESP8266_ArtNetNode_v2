/*
ESP8266_ArtNetNode v2.0.0
Copyright (c) 2016, Matthew Tong
https://github.com/mtongnz/ESP8266_ArtNetNode_v2

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any
later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.
If not, see http://www.gnu.org/licenses/

Note:
This is a pre-release version of this software.  It is not yet ready for prime time and contains bugs (known and unknown).
Please submit any bugs or code changes so they can be included into the next release.

Prizes up for grabs:
I am giving away a few of my first batch of prototype PCBs.  They will be fully populated - valued at over $30 just for parts.
In order to recieve one, please complete one of the following tasks.  You can "win" multiple boards.
1 - Fix the WDT reset issue (https://github.com/mtongnz/ESP8266_ArtNetNode_v2/issues/41)
2 - Implement stored scenes function.  I want it to allow for static scenes or for chases to run.
3 - Most bug fixes, code improvements, feature additions & helpful submissions.
    eg. Fixing the flickering WS2812 (https://github.com/mtongnz/ESP8266_ArtNetNode_v2/issues/36)
        Adding other pixel strips (https://github.com/mtongnz/ESP8266_ArtNetNode_v2/issues/42)
        Creating new web UI theme (https://github.com/mtongnz/ESP8266_ArtNetNode_v2/issues/22)

These prizes will be based on the first person to submit a solution that I judge to be adequate.  My decision is final.
This competition will open to the general public a couple of weeks after the private code release to supporters.
*/

#include <EEPROM.h>
// #pragma GCC diagnostic push //TODO: usando ESPAsyncWebServer questo non dovrebbe più servire
// #pragma GCC diagnostic ignored "-Wmaybe-uninitialized" //TODO: usando ESPAsyncWebServer questo non dovrebbe più servire
#include <ESP8266WiFi.h>
// #pragma GCC diagnostic pop //TODO: usando ESPAsyncWebServer questo non dovrebbe più servire
#include <LittleFS.h>
#include <Ethernet.h>
#include <ESPAsyncWebServer.h>
#include <espDMX_RDM.h>
#include <main.h>
#include <startFunctions.h>
#include <store.h>

extern "C" {
  #include "user_interface.h"
  extern struct rst_info resetInfo;
}

uint8_t portA[5], portB[5];
uint8_t MAC_array[6];
uint8_t dmxInSeqID = 0;
uint8_t statusLedData[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
uint32_t statusTimer = 0;

esp8266ArtNetRDM artRDM;
AsyncWebServer webServer(80);
DynamicJsonBuffer jsonBuffer;
ws2812Driver pixDriver;
File fsUploadFile;
bool statusLedsDim = true;
bool statusLedsOff = false;

pixPatterns pixFXA(0, &pixDriver);
pixPatterns pixFXB(1, &pixDriver);

const char PROGMEM typeHTML[] = "text/html";
const char PROGMEM typeCSS[] = "text/css";
const char PROGMEM typeJS[] = "text/javascript";

char wifiStatus[70] = "";
bool isHotspot = false;
uint32_t nextNodeReport = 0;
char nodeError[ARTNET_NODE_REPORT_LENGTH] = "";
bool nodeErrorShowing = 1;
uint32_t nodeErrorTimeout = 0;
bool pixDone = true;
bool newDmxIn = false;
bool doReboot = false;
byte* dataIn;

void setup(void) {
  bool resetDefaults = false;

  // Make direction input to avoid boot garbage being sent out
  pinMode(DMX_DIR_A, OUTPUT);
  digitalWrite(DMX_DIR_A, LOW);
  #ifndef ONE_PORT
    pinMode(DMX_DIR_B, OUTPUT);
    digitalWrite(DMX_DIR_B, LOW);
  #endif

  #ifdef STATUS_LED_PIN
    pinMode(STATUS_LED_PIN, OUTPUT);
    digitalWrite(STATUS_LED_PIN, LOW);
    delay(1);
    setStatusLed(STATUS_LED_S, PINK);
    doStatusLedOutput();
  #endif

  pinMode(DMX_EN, OUTPUT_OPEN_DRAIN);
  digitalWrite(DMX_EN, HIGH);

  #ifdef SETTINGS_RESET
    pinMode(SETTINGS_RESET, INPUT);

    delay(5);
    // button pressed = low reading
    if (!digitalRead(SETTINGS_RESET)) {
      delay(50);
      if (!digitalRead(SETTINGS_RESET))
        resetDefaults = true;
    }
  #endif

  Ethernet.init(ETHERNET_CS);

  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();
  yield();

  // Start EEPROM
  EEPROM.begin(512);

  // Start LittleFS file system
  if (!LittleFS.begin()) {
    LittleFS.format(); // web server contents won't be shown

    if (!LittleFS.begin()) {
      while (1) {
        // stay forever here as useless to go further
        yield();
      }
    }
  }

  // Load our saved values or store defaults
  if (!resetDefaults)
    eepromLoad();

  // Store our counters for resetting defaults
  if (resetInfo.reason != REASON_DEFAULT_RST && resetInfo.reason != REASON_EXT_SYS_RST && resetInfo.reason != REASON_SOFT_RESTART)
    deviceSettings.wdtCounter++;
  else
    deviceSettings.resetCounter++;

  // Store values
  eepromSave();

  // Start wifi
  wifiStart();

  // Start web server
  webStart();


  // Don't start our Artnet or DMX in firmware update mode or after multiple WDT resets
  if (!deviceSettings.doFirmwareUpdate && deviceSettings.wdtCounter <= 3) {

    // We only allow 1 DMX input - and RDM can't run alongside DMX in
    if (deviceSettings.portAmode == TYPE_DMX_IN && deviceSettings.portBmode == TYPE_RDM_OUT)
      deviceSettings.portBmode = TYPE_DMX_OUT;

    // Setup Artnet Ports & Callbacks
    artStart();

    // Don't open any ports for a bit to let the ESP spill it's garbage to serial
    while (millis() < 3500)
      yield();

    // Port Setup
    portSetup();

  } else
    deviceSettings.doFirmwareUpdate = false;

  delay(10);
}

void loop(void){
  // If the device lasts for 6 seconds, clear our reset timers
  if (deviceSettings.resetCounter != 0 && millis() > 6000) {
    deviceSettings.resetCounter = 0;
    deviceSettings.wdtCounter = 0;
    eepromSave();
  }

  webServer.handleClient();

  // Get the node details and handle Artnet
  doNodeReport();
  artRDM.handler();

  yield();

  // DMX handlers
  dmxA.handler();
  #ifndef ONE_PORT
    dmxB.handler();
  #endif

  // Do Pixel FX on port A
  if (deviceSettings.portAmode == TYPE_WS2812 && deviceSettings.portApixMode != FX_MODE_PIXEL_MAP) {
    if (pixFXA.Update())
      pixDone = 0;
  }

  // Do Pixel FX on port B
  #ifndef ONE_PORT
    if (deviceSettings.portBmode == TYPE_WS2812 && deviceSettings.portBpixMode != FX_MODE_PIXEL_MAP) {
      if (pixFXB.Update())
        pixDone = 0;
    }
  #endif

  // Do pixel string output
  if (!pixDone)
    pixDone = pixDriver.show();

  // Handle received DMX
  if (newDmxIn) {
    uint8_t g, p;

    newDmxIn = false;

    g = portA[0];
    p = portA[1];

    IPAddress bc = deviceSettings.dmxInBroadcast;
    artRDM.sendDMX(g, p, bc, dataIn, 512);

    #ifndef ESP_01
      setStatusLed(STATUS_LED_A, CYAN);
    #endif
  }

  // Handle rebooting the system
  if (doReboot) {
    char c[ARTNET_NODE_REPORT_LENGTH] = "Device rebooting...";
    artRDM.setNodeReport(c, ARTNET_RC_POWER_OK);
    artRDM.artPollReply();

    // Ensure all web data is sent before we reboot
    uint32_t n = millis() + 1000;
    while (millis() < n)
      webServer.handleClient();

    ESP.restart();
  }

  #ifdef STATUS_LED_PIN
    // Output status to LEDs once per second
    if (statusTimer < millis()) {

      // Flash our main status LED
      if ((statusTimer % 2000) > 1000)
        setStatusLed(STATUS_LED_S, BLACK);
      else if (nodeError[0] != '\0')
        setStatusLed(STATUS_LED_S, RED);
      else
        setStatusLed(STATUS_LED_S, GREEN);

      doStatusLedOutput();
      statusTimer = millis() + 1000;
    }
  #endif
}

void dmxHandle(uint8_t group, uint8_t port, uint16_t numChans, bool syncEnabled) {
  if (portA[0] == group) {
    if (deviceSettings.portAmode == TYPE_WS2812) {

      #ifndef ESP_01
        setStatusLed(STATUS_LED_A, GREEN);
      #endif

      if (deviceSettings.portApixMode == FX_MODE_PIXEL_MAP) {
        if (numChans > 510)
          numChans = 510;

        // Copy DMX data to the pixels buffer
        pixDriver.setBuffer(0, port * 510, artRDM.getDMX(group, port), numChans);

        // Output to pixel strip
        if (!syncEnabled)
          pixDone = false;

        return;

      // FX 12 Mode
      } else if (port == portA[1]) {
        byte* a = artRDM.getDMX(group, port);
        uint16_t s = deviceSettings.portApixFXstart - 1;

        pixFXA.Intensity = a[s + 0];
        pixFXA.setFX(a[s + 1]);
        pixFXA.setSpeed(a[s + 2]);
        pixFXA.Pos = a[s + 3];
        pixFXA.Size = a[s + 4];

        pixFXA.setColour1((a[s + 5] << 16) | (a[s + 6] << 8) | a[s + 7]);
        pixFXA.setColour2((a[s + 8] << 16) | (a[s + 9] << 8) | a[s + 10]);
        pixFXA.Size1 = a[s + 11];
        //pixFXA.Fade = a[s + 12];

        pixFXA.NewData = 1;

      }

    // DMX modes
    } else if (deviceSettings.portAmode != TYPE_DMX_IN && port == portA[1]) {
      dmxA.chanUpdate(numChans);

      #ifndef ESP_01
        setStatusLed(STATUS_LED_A, BLUE);
      #endif
    }


  #ifndef ONE_PORT
  } else if (portB[0] == group) {
    if (deviceSettings.portBmode == TYPE_WS2812) {
      setStatusLed(STATUS_LED_B, GREEN);

      if (deviceSettings.portBpixMode == FX_MODE_PIXEL_MAP) {
        if (numChans > 510)
          numChans = 510;

        // Copy DMX data to the pixels buffer
        pixDriver.setBuffer(1, port * 510, artRDM.getDMX(group, port), numChans);

        // Output to pixel strip
        if (!syncEnabled)
          pixDone = false;

        return;

      // FX 12 mode
      } else if (port == portB[1]) {
        byte* a = artRDM.getDMX(group, port);
        uint16_t s = deviceSettings.portBpixFXstart - 1;

        pixFXB.Intensity = a[s + 0];
        pixFXB.setFX(a[s + 1]);
        pixFXB.setSpeed(a[s + 2]);
        pixFXB.Pos = a[s + 3];
        pixFXB.Size = a[s + 4];
        pixFXB.setColour1((a[s + 5] << 16) | (a[s + 6] << 8) | a[s + 7]);
        pixFXB.setColour2((a[s + 8] << 16) | (a[s + 9] << 8) | a[s + 10]);
        pixFXB.Size1 = a[s + 11];
        //pixFXB.Fade = a[s + 12];

        pixFXB.NewData = 1;
      }
    } else if (deviceSettings.portBmode != TYPE_DMX_IN && port == portB[1]) {
      dmxB.chanUpdate(numChans);
      setStatusLed(STATUS_LED_B, BLUE);
    }
  #endif
  }

}

void syncHandle() {
  if (deviceSettings.portAmode == TYPE_WS2812) {
    rdmPause(1);
    pixDone = pixDriver.show();
    rdmPause(0);
  } else if (deviceSettings.portAmode != TYPE_DMX_IN)
    dmxA.unPause();

  #ifndef ONE_PORT
    if (deviceSettings.portBmode == TYPE_WS2812) {
      rdmPause(1);
      pixDone = pixDriver.show();
      rdmPause(0);
    } else if (deviceSettings.portBmode != TYPE_DMX_IN)
      dmxB.unPause();
  #endif
}

void ipHandle() {
  if (artRDM.getDHCP()) {
    deviceSettings.gateway = INADDR_NONE;

    deviceSettings.dhcpEnable = 1;
    doReboot = true;
    /*
    // Re-enable DHCP
    WiFi.begin(deviceSettings.wifiSSID, deviceSettings.wifiPass);

    // Wait for an IP
    while (WiFi.status() != WL_CONNECTED)
      yield();

    // Save settings to struct
    deviceSettings.ip = WiFi.localIP();
    deviceSettings.subnet = WiFi.subnetMask();
    deviceSettings.broadcast = {~deviceSettings.subnet[0] | (deviceSettings.ip[0] & deviceSettings.subnet[0]), ~deviceSettings.subnet[1] | (deviceSettings.ip[1] & deviceSettings.subnet[1]), ~deviceSettings.subnet[2] | (deviceSettings.ip[2] & deviceSettings.subnet[2]), ~deviceSettings.subnet[3] | (deviceSettings.ip[3] & deviceSettings.subnet[3])};

    // Pass IP to artRDM
    artRDM.setIP(deviceSettings.ip, deviceSettings.subnet);
    */

  } else {
    deviceSettings.ip = artRDM.getIP();
    deviceSettings.subnet = artRDM.getSubnetMask();
    deviceSettings.gateway = deviceSettings.ip;
    deviceSettings.gateway[3] = 1;
    deviceSettings.broadcast = {static_cast<uint8_t>(deviceSettings.ip[0] | ~deviceSettings.subnet[0]), static_cast<uint8_t>(deviceSettings.ip[1] | ~deviceSettings.subnet[1]), static_cast<uint8_t>(deviceSettings.ip[2] | ~deviceSettings.subnet[2]), static_cast<uint8_t>(deviceSettings.ip[3] | ~deviceSettings.subnet[3])};
    deviceSettings.dhcpEnable = 0;

    doReboot = true;

    //WiFi.config(deviceSettings.ip,deviceSettings.ip,deviceSettings.ip,deviceSettings.subnet);
  }

  // Store everything to EEPROM
  eepromSave();
}

void addressHandle() {
  memcpy(&deviceSettings.nodeName, artRDM.getShortName(), ARTNET_SHORT_NAME_LENGTH);
  memcpy(&deviceSettings.longName, artRDM.getLongName(), ARTNET_LONG_NAME_LENGTH);

  deviceSettings.portAnet = artRDM.getNet(portA[0]);
  deviceSettings.portAsub = artRDM.getSubNet(portA[0]);
  deviceSettings.portAuni[0] = artRDM.getUni(portA[0], portA[1]);
  deviceSettings.portAmerge = artRDM.getMerge(portA[0], portA[1]);

  if (artRDM.getE131(portA[0], portA[1]))
    deviceSettings.portAprot = PROT_ARTNET_SACN;
  else
    deviceSettings.portAprot = PROT_ARTNET;


  #ifndef ONE_PORT
    deviceSettings.portBnet = artRDM.getNet(portB[0]);
    deviceSettings.portBsub = artRDM.getSubNet(portB[0]);
    deviceSettings.portBuni[0] = artRDM.getUni(portB[0], portB[1]);
    deviceSettings.portBmerge = artRDM.getMerge(portB[0], portB[1]);

    if (artRDM.getE131(portB[0], portB[1]))
      deviceSettings.portBprot = PROT_ARTNET_SACN;
    else
      deviceSettings.portBprot = PROT_ARTNET;
  #endif

  // Store everything to EEPROM
  eepromSave();
}

void rdmHandle(uint8_t group, uint8_t port, rdm_data* c) {
  if (portA[0] == group && portA[1] == port)
    dmxA.rdmSendCommand(c);

  #ifndef ONE_PORT
    else if (portB[0] == group && portB[1] == port)
      dmxB.rdmSendCommand(c);
  #endif
}

void rdmReceivedA(rdm_data* c) {
  artRDM.rdmResponse(c, portA[0], portA[1]);
}

void sendTodA() {
  artRDM.artTODData(portA[0], portA[1], dmxA.todMan(), dmxA.todDev(), dmxA.todCount(), dmxA.todStatus());
}

#ifndef ONE_PORT
void rdmReceivedB(rdm_data* c) {
  artRDM.rdmResponse(c, portB[0], portB[1]);
}

void sendTodB() {
  artRDM.artTODData(portB[0], portB[1], dmxB.todMan(), dmxB.todDev(), dmxB.todCount(), dmxB.todStatus());
}
#endif

void todRequest(uint8_t group, uint8_t port) {
  if (portA[0] == group && portA[1] == port)
    sendTodA();

  #ifndef ONE_PORT
    else if (portB[0] == group && portB[1] == port)
      sendTodB();
  #endif
}

void todFlush(uint8_t group, uint8_t port) {
  if (portA[0] == group && portA[1] == port)
    dmxA.rdmDiscovery();

  #ifndef ONE_PORT
    else if (portB[0] == group && portB[1] == port)
      dmxB.rdmDiscovery();
  #endif
}

void dmxIn(uint16_t num) {
  (void)num;

  // Double buffer switch
  byte* tmp = dataIn;
  dataIn = dmxA.getChans();
  dmxA.setBuffer(tmp);

  newDmxIn = true;
}

#ifdef STATUS_LED_PIN
void doStatusLedOutput() {
  uint8_t a[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};

  if (!statusLedsOff) {
    if (statusLedsDim) {
      for (uint8_t x = 0; x < 9; x++)
        a[x] = statusLedData[x] & STATUS_DIM;
    } else {
      for (uint8_t x = 0; x < 9; x++)
        a[x] = statusLedData[x];
    }
  }

  #ifdef STATUS_LED_MODE_APA106
    pixDriver.doAPA106(&a[0], STATUS_LED_PIN, 9);
  #endif

  #ifdef STATUS_LED_MODE_WS2812
    pixDriver.doPixel(&a[0], STATUS_LED_PIN, 9);
  #endif

  // Tint LEDs red slightly - they'll be changed back before being displayed if no errors
  for (uint8_t x = 1; x < 9; x += 3)
    statusLedData[x] = 125;
}
#endif

void setStatusLed(uint8_t num, uint32_t col) {
  memcpy(&statusLedData[num*3], &col, 3);
}
