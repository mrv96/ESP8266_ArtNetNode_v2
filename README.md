# ESP8266_ArtNetNode_v2 Relevant Forks

This branch backups and describe all the relevant forks of this project found.

## [casesolved-co-uk](https://github.com/casesolved-co-uk/ESP8266_ArtNetNode_v2)

Changes:

- Cleanup and refactor
- Debug log visible from website
- Memory usage logging
- Website code stored as SPIFFS static files and served by ESPAsyncWebServer
- Board config for single DMX port (ONE_PORT) over ajax for dynamic web page
- Website: Dynamically remove Port B, add status and max LED brightnesses
- FASTLED library for all LED driving including options for supported non-clocked LED types
- Reimplement FX library for FastLED
- Support ESP8266 BSP v2.5.2
- Update to ArduinoJson v6
- Use json file as settings store instead of struct for sharing with website
- Fix compile warnings
- Redirect debug/exception logging to debug log & remove disable from DMX lib
- Dump restart exception in format for exception decoder
- Move to external DMX library & fix high-watermark bug
- Add UART level invert feature

## [deagenstroup](https://github.com/deagenstroup/ESP8266_ArtNetNode_2022)

Changes:

- Cleanup
- Work with WS2812B led strips instead of WS2812

## [tinic](https://github.com/tinic/ESP32_ArtNetNode)

Changes:

- Cleanup and refactor
- Replace ESP8266 with ESP32

## [JonasArnold](https://github.com/JonasArnold/EthernetDmxNode_esp8266)

Main Changes:

- Cleanup and refactor
- Debug logging
- Add Travis CI
- Implement Multicast and Unicast sACN. In the original version by mtongnz only Multicast is implemented.

Additional goals (maybe not implemented):

- Use normal RGB LED for the Status LED.
- Use normal LEDs for the DMX activity LEDs.
- Beautify the code, make it more structured and clear.
- I will not work with the WS2812 part of the node so it is possible I will remove this.
- New modern user interface.

## [scamiv](https://github.com/scamiv/ESP8266_ArtNetNode_v2)

This fork aims to improve stability: <https://github.com/mtongnz/ESP8266_ArtNetNode_v2/pull/92>

Changes:

- WS2812 no longer flicker
- ESP8266WebServer leaked memory(open connections) gets reclaimed.
- Reboot on low heap.
- Universe 10 now ouputs raw pwm values to pins defined in pwmports[].
- Default dmxA/B pins changed to D1/D2 to avoid boot flicker.
- Improved heap usage.

>Compile options:
>
>- CPU Frequency needs to be 160mhz, SDK 2.4.1 recommeded.
>- Set lwIP to "V2 Lower Memory"
>- Dont forget to upload SPIFFS (click on "ESP8266 Sketch Data Upload") Requires: <https://github.com/esp8266/arduino-esp8266fs-plugin>
>- you can get an extra 4kb of heap by moving g_cont to sys area, this may kill timer callbacks and gdb as a side effekt <https://github.com/esp8266/Arduino/pull/4553/commits/17bf98c01cd6ffc0cd1a1c48ade28ed0a01ffad0>
>
>those options should give you about 45kB of heap.

## [ammuller](https://github.com/ammuller/ESP8266_ArtNetNode_v2) (at first look to be avoided)

This fork aims to improve stability: <https://github.com/mtongnz/ESP8266_ArtNetNode_v2/pull/66>

## Bonus: extra useful info on original version from [markusb](https://github.com/markusb/ESP8266_ArtNetNode_V2)

### ESP Pins used (including NodeMCU & Wemos)

This is the pins used by the ArtNetNode firmware. The NodeMCU & Wemos/Lolin boards use strange numbering that doesn't match the ESP8266 numbering. They are show also

Here are the main hookups needed:

| ESP8266 GPIO | NodeMCU & Wemos | Purpose |
|--------------|-----------------|---------|
| GPIO1  | TX | DMX_TX_A / WS2812 data A |
| GPIO2  | D4 | DMX_TX_B / WS2812 data B |
| GPIO3  | RX | DMX_RX (for A & B) |
| GPIO5  | D1 | DMX_DIR_A |
| GPIO16 | D0 | DMX_DIR_B |
| GPIO12 | D6 | LED_A 3 x WS2812 for status display |
| GPIO14 | D5 | Button for reset to factory defaults |

### Node status display

If a string of three WS2812 LEDs are connected to LED_A then they show the status of the device.

| LED   | Display       | Color |
|-------|---------------|-------|
| LED 1 | Channel A     | Cyan: DMX input sent to Artnet |
|       |               | Blue: Artnet input sent to DMX (DMX mode) |
|       |               | Green: Artnet input sent to WS2812 (WS2812 mode) |
| LED 2 | Channel B     | Cyan: DMX input sent to Artnet |
|       |               | Blue: Artnet input sent to DMX (DMX mode) |
|       |               | Green: Artnet input sent to WS2812 (WS2812 mode) |
| LED 3 | Device status | Pink: Startup |
|       |               | Green flashing: Hartbeat |
|       |               | Red flashing: Error |

For the node status display three WS2812 LEDs need to be connected to pin LED_A.
