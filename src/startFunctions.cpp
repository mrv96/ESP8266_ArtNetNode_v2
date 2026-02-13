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
*/

#include <stdint.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include <AsyncJson.h>
#include <espDMX_RDM.h>
#include <startFunctions.h>
#include <config.h>
#include <ajax.h>
#include <firmUpdate.h>
#include <main.h>
#include <store.h>

extern "C" {
  #include "user_interface.h"
  extern struct rst_info resetInfo;
}

void doNodeReport() {
  if (nextNodeReport > millis())
    return;

  char c[ARTNET_NODE_REPORT_LENGTH];
  uint8_t c_len;

  if (nodeErrorTimeout > millis())
    nextNodeReport = millis() + 2000;
  else
    nextNodeReport = millis() + 5000;

  if (nodeError[0] != '\0' && !nodeErrorShowing && nodeErrorTimeout > millis()) {

    nodeErrorShowing = true;
    strcpy(c, nodeError);

  } else {
    nodeErrorShowing = false;

    strcpy(c, "OK: PortA:");
    c_len = strlen(c);

    switch (deviceSettings.portAmode) {
      case TYPE_DMX_OUT:
        c_len += sprintf(c + c_len, " DMX Out");
        break;

      case TYPE_RDM_OUT:
        c_len += sprintf(c + c_len, " RDM Out");
        break;

      case TYPE_DMX_IN:
        c_len += sprintf(c + c_len, " DMX In");
        break;

      case TYPE_WS2812:
        if (deviceSettings.portApixMode == FX_MODE_12)
          c_len += sprintf(c + c_len, " 12chan");
        c_len += sprintf(c + c_len, " WS2812 %ipixels", deviceSettings.portAnumPix);
        break;
    }

    #ifndef ONE_PORT
      c_len +=sprintf(c + c_len, ". PortB:");

      switch (deviceSettings.portBmode) {
        case TYPE_DMX_OUT:
          sprintf(c + c_len, " DMX Out");
          break;

        case TYPE_RDM_OUT:
          sprintf(c + c_len, " RDM Out");
          break;

        case TYPE_WS2812:
          if (deviceSettings.portBpixMode == FX_MODE_12)
            c_len += sprintf(c + c_len, " 12chan");
          sprintf(c + c_len, " WS2812 %ipixels", deviceSettings.portBnumPix);
          break;
      }
    #endif
  }

  artRDM.setNodeReport(c, ARTNET_RC_POWER_OK);
}

void portSetup() {
  if (deviceSettings.portAmode == TYPE_DMX_OUT || deviceSettings.portAmode == TYPE_RDM_OUT) {
    #ifndef ESP_01
      setStatusLed(STATUS_LED_A, BLUE);
    #endif

    dmxA.begin(DMX_DIR_A, artRDM.getDMX(portA[0], portA[1]));
    if (deviceSettings.portAmode == TYPE_RDM_OUT && !dmxA.rdmEnabled()) {
      dmxA.rdmEnable(ESTA_MAN, ESTA_DEV);
      dmxA.rdmSetCallBack(rdmReceivedA);
      dmxA.todSetCallBack(sendTodA);
    }

  } else if (deviceSettings.portAmode == TYPE_DMX_IN) {
    #ifndef ESP_01
      setStatusLed(STATUS_LED_A, CYAN);
    #endif

    dmxA.begin(DMX_DIR_A, artRDM.getDMX(portA[0], portA[1]));
    dmxA.dmxIn(true);
    dmxA.setInputCallback(dmxIn);

    dataIn = (byte*) os_malloc(sizeof(byte) * 512);
    memset(dataIn, 0, 512);

  } else if (deviceSettings.portAmode == TYPE_WS2812) {
    #ifndef ESP_01
      setStatusLed(STATUS_LED_A, GREEN);
    #endif

    digitalWrite(DMX_DIR_A, HIGH);
    pixDriver.setStrip(0, DMX_TX_A, deviceSettings.portAnumPix, deviceSettings.portApixConfig);
  }

  #ifndef ONE_PORT
    if (deviceSettings.portBmode == TYPE_DMX_OUT || deviceSettings.portBmode == TYPE_RDM_OUT) {
      setStatusLed(STATUS_LED_B, BLUE);

      dmxB.begin(DMX_DIR_B, artRDM.getDMX(portB[0], portB[1]));
      if (deviceSettings.portBmode == TYPE_RDM_OUT && !dmxB.rdmEnabled()) {
        dmxB.rdmEnable(ESTA_MAN, ESTA_DEV);
        dmxB.rdmSetCallBack(rdmReceivedB);
        dmxB.todSetCallBack(sendTodB);
      }

    } else if (deviceSettings.portBmode == TYPE_WS2812) {
      setStatusLed(STATUS_LED_B, GREEN);

      digitalWrite(DMX_DIR_B, HIGH);
      pixDriver.setStrip(1, DMX_TX_B, deviceSettings.portBnumPix, deviceSettings.portBpixConfig);
    }
  #endif

  pixDriver.allowInterruptSingle = WS2812_ALLOW_INT_SINGLE;
  pixDriver.allowInterruptDouble = WS2812_ALLOW_INT_DOUBLE;
}

void artStart() {
  // Initialise out ArtNet
  if (isHotspot)
    artRDM.init(deviceSettings.hotspotIp, deviceSettings.hotspotSubnet, true, deviceSettings.nodeName, deviceSettings.longName, ARTNET_OEM, ESTA_MAN, MAC_array);
  else
    artRDM.init(deviceSettings.ip, deviceSettings.subnet, deviceSettings.dhcpEnable, deviceSettings.nodeName, deviceSettings.longName, ARTNET_OEM, ESTA_MAN, MAC_array);

  // Set firmware
  artRDM.setFirmwareVersion(ART_FIRM_VERSION);

  // Add Group
  portA[0] = artRDM.addGroup(deviceSettings.portAnet, deviceSettings.portAsub);

  bool e131 = (deviceSettings.portAprot == PROT_ARTNET_SACN) ? true : false;

  // WS2812 uses TYPE_DMX_OUT - the rest use the value assigned
  if (deviceSettings.portAmode == TYPE_WS2812)
    portA[1] = artRDM.addPort(portA[0], 0, deviceSettings.portAuni[0], TYPE_DMX_OUT, deviceSettings.portAmerge);
  else
    portA[1] = artRDM.addPort(portA[0], 0, deviceSettings.portAuni[0], deviceSettings.portAmode, deviceSettings.portAmerge);

  artRDM.setE131(portA[0], portA[1], e131);
  artRDM.setE131Uni(portA[0], portA[1], deviceSettings.portAsACNuni[0]);

  // Add extra Artnet ports for WS2812
  if (deviceSettings.portAmode == TYPE_WS2812 && deviceSettings.portApixMode == FX_MODE_PIXEL_MAP) {
    if (deviceSettings.portAnumPix > 170) {
      portA[2] = artRDM.addPort(portA[0], 1, deviceSettings.portAuni[1], TYPE_DMX_OUT, deviceSettings.portAmerge);

      artRDM.setE131(portA[0], portA[2], e131);
      artRDM.setE131Uni(portA[0], portA[2], deviceSettings.portAsACNuni[1]);
    }
    if (deviceSettings.portAnumPix > 340) {
      portA[3] = artRDM.addPort(portA[0], 2, deviceSettings.portAuni[2], TYPE_DMX_OUT, deviceSettings.portAmerge);

      artRDM.setE131(portA[0], portA[3], e131);
      artRDM.setE131Uni(portA[0], portA[3], deviceSettings.portAsACNuni[2]);
    }
    if (deviceSettings.portAnumPix > 510) {
      portA[4] = artRDM.addPort(portA[0], 3, deviceSettings.portAuni[3], TYPE_DMX_OUT, deviceSettings.portAmerge);

      artRDM.setE131(portA[0], portA[4], e131);
      artRDM.setE131Uni(portA[0], portA[4], deviceSettings.portAsACNuni[3]);
    }
  }


  #ifndef ONE_PORT
    // Add Group
    portB[0] = artRDM.addGroup(deviceSettings.portBnet, deviceSettings.portBsub);
    e131 = (deviceSettings.portBprot == PROT_ARTNET_SACN) ? true : false;

    // WS2812 uses TYPE_DMX_OUT - the rest use the value assigned
    if (deviceSettings.portBmode == TYPE_WS2812)
      portB[1] = artRDM.addPort(portB[0], 0, deviceSettings.portBuni[0], TYPE_DMX_OUT, deviceSettings.portBmerge);
    else
      portB[1] = artRDM.addPort(portB[0], 0, deviceSettings.portBuni[0], deviceSettings.portBmode, deviceSettings.portBmerge);

    artRDM.setE131(portB[0], portB[1], e131);
    artRDM.setE131Uni(portB[0], portB[1], deviceSettings.portBsACNuni[0]);

    // Add extra Artnet ports for WS2812
    if (deviceSettings.portBmode == TYPE_WS2812 && deviceSettings.portBpixMode == FX_MODE_PIXEL_MAP) {
      if (deviceSettings.portBnumPix > 170) {
        portB[2] = artRDM.addPort(portB[0], 1, deviceSettings.portBuni[1], TYPE_DMX_OUT, deviceSettings.portBmerge);

        artRDM.setE131(portB[0], portB[2], e131);
        artRDM.setE131Uni(portB[0], portB[2], deviceSettings.portBsACNuni[1]);
      }
      if (deviceSettings.portBnumPix > 340) {
        portB[3] = artRDM.addPort(portB[0], 2, deviceSettings.portBuni[2], TYPE_DMX_OUT, deviceSettings.portBmerge);

        artRDM.setE131(portB[0], portB[3], e131);
        artRDM.setE131Uni(portB[0], portB[3], deviceSettings.portBsACNuni[2]);
      }
      if (deviceSettings.portBnumPix > 510) {
        portB[4] = artRDM.addPort(portB[0], 3, deviceSettings.portBuni[3], TYPE_DMX_OUT, deviceSettings.portBmerge);

        artRDM.setE131(portB[0], portB[4], e131);
        artRDM.setE131Uni(portB[0], portB[4], deviceSettings.portBsACNuni[3]);
      }
    }
  #endif

  // Add required callback functions
  artRDM.setArtDMXCallback(dmxHandle);
  artRDM.setArtRDMCallback(rdmHandle);
  artRDM.setArtSyncCallback(syncHandle);
  artRDM.setArtIPCallback(ipHandle);
  artRDM.setArtAddressCallback(addressHandle);
  artRDM.setTODRequestCallback(todRequest);
  artRDM.setTODFlushCallback(todFlush);


  switch (resetInfo.reason) {
    case REASON_DEFAULT_RST:  // normal start
    case REASON_EXT_SYS_RST:
    case REASON_SOFT_RESTART:
      artRDM.setNodeReport("OK: Device started", ARTNET_RC_POWER_OK);
      nextNodeReport = millis() + 4000;
      break;

    case REASON_WDT_RST:
      artRDM.setNodeReport("ERROR: (HWDT) Unexpected device restart", ARTNET_RC_POWER_FAIL);
      strcpy(nodeError, "Restart error: HWDT");
      nextNodeReport = millis() + 10000;
      nodeErrorTimeout = millis() + 30000;
      break;
    case REASON_EXCEPTION_RST:
      artRDM.setNodeReport("ERROR: (EXCP) Unexpected device restart", ARTNET_RC_POWER_FAIL);
      strcpy(nodeError, "Restart error: EXCP");
      nextNodeReport = millis() + 10000;
      nodeErrorTimeout = millis() + 30000;
      break;
    case REASON_SOFT_WDT_RST:
      artRDM.setNodeReport("ERROR: (SWDT) Unexpected device restart", ARTNET_RC_POWER_FAIL);
      strcpy(nodeError, "Error on Restart: SWDT");
      nextNodeReport = millis() + 10000;
      nodeErrorTimeout = millis() + 30000;
      break;
    case REASON_DEEP_SLEEP_AWAKE:
      // not used
      break;
  }

  // Start artnet
  artRDM.begin();

  yield();
}

void webStart() {
  // webServer.serveStatic("/", LittleFS, "/index.html"); //TODO: si può usare se rimuoviamo artRDM handling
  // webServer.serveStatic("/", LittleFS, "/").setDefaultFile("index.html"); //TODO: utile per debug navigare filesystem
  webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    AsyncWebServerResponse *response;

    response = request->beginResponse(LittleFS, "/index.html", "");
    if (response == NULL) {
      response = request->beginResponse(404, "text/plain", "Page not found"); //TODO
    }
    request->onDisconnect([]() {
      artRDM.begin();
    });

    artRDM.pause();
    request->send(response);
  });

  webServer.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    AsyncWebServerResponse *response;

    response = request->beginResponse(LittleFS, "/style.css", "");
    if (response == NULL) {
      response = request->beginResponse(404, "text/plain", "Page not found"); //TODO
    }
    request->onDisconnect([]() {
      artRDM.begin();
    });

    artRDM.pause();
    request->send(response);
  });

  AsyncCallbackJsonWebHandler* handler = new AsyncCallbackJsonWebHandler("/ajax", ajaxHandle);
  webServer.addHandler(handler);

  webServer.on("/upload", HTTP_POST, webFirmwareUpdate, webFirmwareUpload);

  webServer.serveStatic("/style", LittleFS, "/css_upload.html"); //TODO 404

  webServer.on("/style_delete", HTTP_GET, [](AsyncWebServerRequest *request){
    if (LittleFS.exists("/style.css")) {
      LittleFS.remove("/style.css");
    }
    request->send(200, "text/plain", "style.css deleted. The default style is now in use.");
  });

  webServer.on("/style_upload", HTTP_POST, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "Upload successful!");
  }, [](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final){
    (void)filename;
    if(!index){
      request->_tempFile = LittleFS.open("/style.css", "w");
    }
    if (request->_tempFile) {
      request->_tempFile.write(data, len);
      if(final){
        request->_tempFile.close();
      }
    }
  });

  webServer.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){
    AsyncWebServerResponse *response;

    response = request->beginResponse(LittleFS, "/script.js", "");
    if (response == NULL) {
      response = request->beginResponse(404, "text/plain", "Page not found"); //TODO
    }
    // AsyncClient *currentClient { request->client() };
    // currentClient->onDisconnect([]() {
    //   artRDM.begin();
    // });
    request->onDisconnect([]() {
      artRDM.begin();
    });

    artRDM.pause();
    request->send(response);
  });

  webServer.on("/portb.js", HTTP_GET, [](AsyncWebServerRequest *request){
    AsyncWebServerResponse *response;

#ifdef ONE_PORT
    response = request->beginResponse(LittleFS, "/portb.js");
    if (response == NULL)
#endif
    {
      response = request->beginResponse(404, "text/plain", "Page not found"); //TODO
    }
    request->onDisconnect([]() {
      artRDM.begin();
    });

    artRDM.pause();
    request->send(response);
  });

  webServer.onNotFound([](AsyncWebServerRequest *request){
    request->send(404, "text/plain", "Page not found"); //TODO tenere testo?
  });

  webServer.begin();

  yield();
}

void wifiStart() {
  // If it's the default WiFi SSID, make it unique
  if (strcmp(deviceSettings.hotspotSSID, "espArtNetNode") == 0 || deviceSettings.hotspotSSID[0] == '\0')
    sprintf(deviceSettings.hotspotSSID, "espArtNetNode_%05u", (ESP.getChipId() & 0xFF));

  WiFi.macAddress(MAC_array);
  MAC_array[0] |= 0x02;

  if (deviceSettings.standAloneEnable) {
    startHotspot();
    return;
  }

  if (deviceSettings.dhcpEnable) {
    if (Ethernet.begin(MAC_array) == 0) {
      strcpy(nodeError, "IP error: DHCP");
      startHotspot();
      return;
    }

    deviceSettings.ip = Ethernet.localIP();
    deviceSettings.subnet = Ethernet.subnetMask();

    if (deviceSettings.gateway == INADDR_NONE)
      deviceSettings.gateway = Ethernet.gatewayIP();
  } else {
    Ethernet.begin(MAC_array, deviceSettings.ip, deviceSettings.gateway, deviceSettings.gateway, deviceSettings.subnet);
  }

  deviceSettings.broadcast = {static_cast<uint8_t>(deviceSettings.ip[0] | ~deviceSettings.subnet[0]), static_cast<uint8_t>(deviceSettings.ip[1] | ~deviceSettings.subnet[1]), static_cast<uint8_t>(deviceSettings.ip[2] | ~deviceSettings.subnet[2]), static_cast<uint8_t>(deviceSettings.ip[3] | ~deviceSettings.subnet[3])};

  //sprintf(wifiStatus, "Wifi connected.  Signal: %ld<br />SSID: %s", WiFi.RSSI(), deviceSettings.wifiSSID);
  sprintf(wifiStatus, "Wifi connected.<br />SSID: %s", deviceSettings.wifiSSID);

  yield();
}

void startHotspot() {
  IPAddress firstAddr = IPAddress(
    deviceSettings.hotspotIp[0] & deviceSettings.hotspotSubnet[0],
    deviceSettings.hotspotIp[1] & deviceSettings.hotspotSubnet[1],
    deviceSettings.hotspotIp[2] & deviceSettings.hotspotSubnet[2],
    (deviceSettings.hotspotIp[3] & deviceSettings.hotspotSubnet[3]) + 1
  );

  Ethernet.begin(MAC_array, deviceSettings.hotspotIp, firstAddr, firstAddr, deviceSettings.hotspotSubnet);

  deviceSettings.ip = deviceSettings.hotspotIp;
  deviceSettings.subnet = deviceSettings.hotspotSubnet;
  deviceSettings.broadcast = deviceSettings.hotspotBroadcast;
  deviceSettings.gateway = firstAddr;
}
