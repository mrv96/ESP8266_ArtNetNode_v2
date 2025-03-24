#ifndef _CONFIG_H_
#define _CONFIG_H_


#define FIRMWARE_VERSION "v2.0.0 (beta 5g)"
#define ART_FIRM_VERSION 0x0200   // Firmware given over Artnet (2 bytes)


//#define ESP_01              // Un comment for ESP_01 board settings
//#define NO_RESET            // Un comment to disable the reset button

// Wemos boards use 4M (3M LittleFS) compiler option


#define ARTNET_OEM 0x0123    // Artnet OEM Code
#define ESTA_MAN 0x08DD      // ESTA Manufacturer Code
#define ESTA_DEV 0xEE000000  // RDM Device ID (used with Man Code to make 48bit UID)






#ifdef ESP_01
  #define DMX_DIR_A 2   // Same pin as TX1
  #define DMX_TX_A 1
  #define ONE_PORT
  #define NO_RESET

  #define WS2812_ALLOW_INT_SINGLE false
  #define WS2812_ALLOW_INT_DOUBLE false

#else
  #define ETHERNET_CS 5

  #define DMX_DIR_A 15  // D8
  #define DMX_DIR_B 16  // D0
  #define DMX_TX_A 1
  #define DMX_TX_B 2

  #define DMX_EN 4

  // #define STATUS_LED_PIN 12
  // #define STATUS_LED_MODE_WS2812
  #define STATUS_LED_MODE_APA106
  #define STATUS_LED_A 0  // Physical wiring order for status LEDs
  #define STATUS_LED_B 1
  #define STATUS_LED_S 2

  #define WS2812_ALLOW_INT_SINGLE false
  #define WS2812_ALLOW_INT_DOUBLE false
#endif

#ifndef NO_RESET
  #define SETTINGS_RESET 0
#endif


// Definitions for status leds  xxBBRRGG
#define BLACK 0x00000000
#define WHITE 0x00FFFFFF
#define RED 0x0000FF00
#define GREEN 0x000000FF
#define BLUE 0x00FF0000
#define CYAN 0x00FF00FF
#define PINK 0x0066FF22
#define MAGENTA 0x00FFFF00
#define YELLOW 0x0000FFFF
#define ORANGE 0x0000FF33
#define STATUS_DIM 0x0F


#endif
