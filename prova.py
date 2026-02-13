import socket

BRIGH_MIN = bytes.fromhex('7e 04 01 00 ff ff ff 00 ef')
BRIGH_MAX = bytes.fromhex('7e 04 01 64 ff ff ff 00 ef')

SPEED_MIN = bytes.fromhex('7e 04 02 00 ff ff ff 00 ef')
SPEED_MAX = bytes.fromhex('7e 04 02 64 ff ff ff 00 ef')

OFF    = bytes.fromhex('7e 04 04 01 ff ff ff 00 ef')
ON     = bytes.fromhex('7e 04 04 00 ff ff ff 00 ef')

WHITE  = bytes.fromhex('7e 05 03 86 03 ff ff 00 ef')
YELLOW = bytes.fromhex('7e 05 03 84 03 ff ff 00 ef')
UV     = bytes.fromhex('7e 05 03 85 03 ff ff 00 ef')
RED    = bytes.fromhex('7e 05 03 80 03 ff ff 00 ef')
GREEN  = bytes.fromhex('7e 05 03 82 03 ff ff 00 ef')
BLUE   = bytes.fromhex('7e 05 03 81 03 ff ff 00 ef')
CYAN   = bytes.fromhex('7e 05 03 83 03 ff ff 00 ef')
# EFFECTS = {
#     'RGB Cycle': 0x87
#     'Color Cycle': 0x88
#     'RGB Fade': 0x89
#     'Color Fade': 0x8a
#     'Color Strobe': 0x95
#     'Red Pulse': 0x8b
#     'Green Pulse': 0x8c
#     'Blue Pulse': 0x8d
#     'White Pulse': 0x91
#     'Amber Pulse': 0x8e
#     'Purple Pulse': 0x90
#     'Cyan Pulse': 0x8f
#     'Red Green Fade': 0x92
#     'Red Blue Fade': 0x93
#     'Green Blue Fade': 0x94
#     'Red Strobe': 0x96
#     'Green Strobe': 0x97
#     'Blue Strobe': 0x98
#     'White Strobe': 0x9c
#     'Purple Strobe': 0x9b
#     'Cyan Strobe': 0x9a
# }

MONOCROME_MAX = bytes.fromhex('7e 05 05 01 64 ff ff 00 ef')
MONOCROME_MIN = bytes.fromhex('7e 05 05 01 00 ff ff 00 ef')

SOUND_MODE1 = bytes.fromhex('7e 06 01 32 01 ff ff 00 ef')
SOUND_MODE2 = bytes.fromhex('7e 06 01 32 02 ff ff 00 ef')
SOUND_MODE3 = bytes.fromhex('7e 06 01 32 03 ff ff 00 ef')
SOUND_MODE4 = bytes.fromhex('7e 06 01 32 05 ff ff 00 ef')
SOUND_MIN   = bytes.fromhex('7e 06 01 00 04 ff ff 00 ef')
SOUND_MAX   = bytes.fromhex('7e 06 01 64 04 ff ff 00 ef')

TEMPERATURE_WARM = bytes.fromhex('7e 06 05 02 64 00 ff 08 ef')
TEMPERATURE_COLD = bytes.fromhex('7e 06 05 02 00 64 ff 08 ef')

RGBWYU = bytes.fromhex('7e 07 05 03 c8 03 ff 5a 00 fc 00 ef') # R from byte4 (c8)


with socket.socket() as s:
    s.connect(('192.168.2.3', 5000))
    fx = bytearray(RED)
    print(fx.hex(' '))
    fx[3] = int(input("inserire numero "), 16)
    print(fx.hex(' '))
    s.send(fx)
