#pragma once

#define TFT_CS     7
#define TFT_RST    10
#define TFT_DC     6
#define TFT_MOSI   9
#define TFT_SCLK   8

#define I2C_SDA    4
#define I2C_SCL    5

// Colors (RGB565)
#define COLOR_BG        0x10A2 // Dark grey/black
#define COLOR_ACCENT    0x051D // Spotify-esque Green
#define COLOR_TEXT      0xFFFF
#define COLOR_CARD      0x2124

enum AppMode {
    MODE_CAD,
    MODE_MUSIC
};