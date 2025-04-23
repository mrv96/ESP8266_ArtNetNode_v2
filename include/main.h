#ifndef _MAIN_H_
#define _MAIN_H_


#include <stdint.h>
#include <Ethernet.h>
#include <EthernetWebServer.hpp>
#include <ArduinoJson.h>
#include <espArtNetRDM.h>
#include <ws2812Driver.h>
#include <wsFX.h>
#include <config.h>

extern uint8_t portA[5], portB[5];
extern uint8_t MAC_array[6];
extern uint8_t statusLedData[9];
extern uint32_t statusTimer;

extern esp8266ArtNetRDM artRDM;
extern EthernetWebServer webServer;
extern DynamicJsonBuffer jsonBuffer;
extern ws2812Driver pixDriver;
extern File fsUploadFile;
extern bool statusLedsDim;
extern bool statusLedsOff;

extern pixPatterns pixFXA;
extern pixPatterns pixFXB;

extern const char PROGMEM mainPage[];
extern const char PROGMEM cssUploadPage[];

extern const char PROGMEM css[];
extern const char PROGMEM typeHTML[];
extern const char PROGMEM typeCSS[];
extern const char PROGMEM typeJS[];

extern char wifiStatus[70];
extern bool isHotspot;
extern uint32_t nextNodeReport;
extern char nodeError[ARTNET_NODE_REPORT_LENGTH];
extern bool nodeErrorShowing;
extern uint32_t nodeErrorTimeout;
extern bool pixDone;
extern bool newDmxIn;
extern bool doReboot;
extern byte* dataIn;

void dmxHandle(uint8_t group, uint8_t port, uint16_t numChans, bool syncEnabled);
void syncHandle();
void ipHandle();
void addressHandle();
void rdmHandle(uint8_t group, uint8_t port, rdm_data* c);
void rdmReceivedA(rdm_data* c);
void sendTodA();
#ifndef ONE_PORT
void rdmReceivedB(rdm_data* c);
void sendTodB();
#endif
void todRequest(uint8_t group, uint8_t port);
void todFlush(uint8_t group, uint8_t port);
void dmxIn(uint16_t num);
void doStatusLedOutput();
void setStatusLed(uint8_t num, uint32_t col);


#endif
