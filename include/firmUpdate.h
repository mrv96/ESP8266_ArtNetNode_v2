#ifndef _FIRMUPDATE_H_
#define _FIRMUPDATE_H_


#include <stdint.h>
#include <ESPAsyncWebServer.h>

void webFirmwareUpdate(AsyncWebServerRequest *request);
void webFirmwareUpload(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final);


#endif
