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
#include <firmUpdate.h>
#include <main.h>

/* webFirmwareUpdate()
 *  display update status after firmware upload and restart
 */
void webFirmwareUpdate(AsyncWebServerRequest *request) {
  // Generate the webpage from the variables above
  String fail = "{\"success\":0,\"message\":\"Unknown Error\"}";
  String ok = "{\"success\":1,\"message\":\"Success: Device restarting\"}";

  // Send to the client
  AsyncWebServerResponse *response = request->beginResponse(200, "application/json", (Update.hasError()) ? fail : ok);
  response->addHeader("Connection", "close");
  response->addHeader("Access-Control-Allow-Origin", "*");
  request->send(response);

  doReboot = true;
}



/* webFirmwareUpload()
 *  handle firmware upload and update
 */
void webFirmwareUpload(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final) {
  String reply = "";

  (void)filename;

  if(!index){
    Update.runAsync(true);
    uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
    if(!Update.begin(maxSketchSpace)){//start with max available size
      reply = "{\"success\":0,\"message\":\"Insufficient space.\"}";
    }
  }
  if(!Update.hasError()){
    if(Update.write(data, len) != len){
      reply = "{\"success\":0,\"message\":\"Failed to save\"}";
    }
  }
  if(final){
    if(Update.end(true)){ //true to set the size to the current progress
      reply = "{\"success\":1,\"message\":\"Success: Device Restarting\"}";
    } else {
      reply = "{\"success\":0,\"message\":\"Unknown Error\"}";
    }
  }
  yield();

  // Send to the client
  if (reply.length() > 0) {
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", reply);
    response->addHeader("Connection", "close");
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
  }
}
