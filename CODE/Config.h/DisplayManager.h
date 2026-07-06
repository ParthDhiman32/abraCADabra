#pragma once
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include "Config.h"

class DisplayManager {
private:
    Adafruit_ST7735 tft;
    GFXcanvas16 canvas;
    AppMode currentMode = MODE_CAD;
    
    String currentSong = "Waiting for PC...";
    String currentArtist = "Connect USB";
    int volume = 50;
    String lastGesture = "NONE";
    
    // Animation states
    int scrollOffset = 0;
    int modeTransitionY = 0;
    bool isTransitioning = false;

    void drawCADUI();
    void drawMusicUI();
    void drawRoundedCard(int x, int y, int w, int h, uint16_t color);
    void drawProgressBar(int x, int y, int w, int h, int progress);

public:
    DisplayManager();
    void begin();
    void update();
    void setMode(AppMode mode);
    void setSongData(String song, String artist);
    void setVolume(int vol);
    void setLastGesture(String gesture);
};