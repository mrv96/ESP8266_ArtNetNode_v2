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

#ifndef _STORE_H_
#define _STORE_H_


#include <stdint.h>
#include <IPAddress.h>

// Change this if the settings structure changes
#define CONFIG_VERSION "b5g"

// Dont change this
#define CONFIG_START 0

enum fx_mode {
  FX_MODE_PIXEL_MAP = 0,
  FX_MODE_12 = 1
};

enum p_type {
  TYPE_DMX_OUT = 0,
  TYPE_RDM_OUT = 1,
  TYPE_DMX_IN = 2,
  TYPE_WS2812 = 3
};

enum p_protocol {
  PROT_ARTNET = 0,
  PROT_ARTNET_SACN = 1
};

enum p_merge {
  MERGE_LTP = 0,
  MERGE_HTP = 1
};

struct StoreStruct {
  // StoreStruct version
  char version[4];

  // Device settings:
  IPAddress ip, subnet, gateway, broadcast, hotspotIp, hotspotSubnet, hotspotBroadcast, dmxInBroadcast;
  bool dhcpEnable, standAloneEnable;
  char nodeName[18], longName[64], wifiSSID[40], wifiPass[40], hotspotSSID[20], hotspotPass[20];
  uint16_t hotspotDelay;
  uint8_t portAmode, portBmode, portAprot, portBprot, portAmerge, portBmerge;
  uint8_t portAnet, portAsub, portAuni[4], portBnet, portBsub, portBuni[4], portAsACNuni[4], portBsACNuni[4];
  uint16_t portAnumPix, portBnumPix, portApixConfig, portBpixConfig;
  bool doFirmwareUpdate;
  uint8_t portApixMode, portBpixMode;
  uint16_t portApixFXstart, portBpixFXstart;
  uint8_t resetCounter, wdtCounter;
};

extern struct StoreStruct deviceSettings;

void eepromSave();
void eepromLoad();


#endif
