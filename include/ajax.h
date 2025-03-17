#ifndef _AJAX_H_
#define _AJAX_H_


#include <stdint.h>
#include <ArduinoJson.h>

void ajaxHandle();
bool ajaxSave(uint8_t page, JsonObject& json);
void ajaxLoad(uint8_t page, JsonObject& jsonReply);


#endif
