#include "DisplayManager.h"

DisplayManager::DisplayManager() : 
    tft(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST), 
    canvas(160, 128) {} // Screen is 128x160, rotated landscape for better layout

void DisplayManager::begin() {
    tft.initR(INITR_BLACKTAB);
    tft.setRotation(1); // Landscape
    tft.fillScreen(COLOR_BG);
}

void DisplayManager::setMode(AppMode mode) {
    if (currentMode != mode) {
        currentMode = mode;
        isTransitioning = true;
        modeTransitionY = -128; // Slide in from top
    }
}

void DisplayManager::setSongData(String song, String artist) {
    currentSong = song;
    currentArtist = artist;
}

void DisplayManager::setVolume(int vol) { volume = vol; }
void DisplayManager::setLastGesture(String gesture) { lastGesture = gesture; }

void DisplayManager::drawRoundedCard(int x, int y, int w, int h, uint16_t color) {
    canvas.fillRoundRect(x, y, w, h, 6, color);
}

void DisplayManager::drawProgressBar(int x, int y, int w, int h, int progress) {
    canvas.fillRoundRect(x, y, w, h, h/2, 0x4228); // Background track
    int pW = map(progress, 0, 100, 0, w);
    canvas.fillRoundRect(x, y, pW, h, h/2, COLOR_ACCENT);
}

void DisplayManager::update() {
    canvas.fillScreen(COLOR_BG);

    if (isTransitioning) {
        modeTransitionY += 15; // Animation speed
        if (modeTransitionY >= 0) {
            modeTransitionY = 0;
            isTransitioning = false;
        }
    }

    if (currentMode == MODE_CAD) {
        drawCADUI();
    } else {
        drawMusicUI();
    }

    // Push buffer to screen - ZERO flickering
    tft.drawRGBBitmap(0, 0, canvas.getBuffer(), 160, 128);
}

void DisplayManager::drawCADUI() {
    int yOffset = isTransitioning ? modeTransitionY : 0;
    
    // Header
    canvas.setTextColor(COLOR_ACCENT);
    canvas.setTextSize(1);
    canvas.setCursor(10, 10 + yOffset);
    canvas.print("FUSION 360 MODE");

    // Main Card
    drawRoundedCard(10, 25 + yOffset, 140, 70, COLOR_CARD);
    
    // Icon (Mocked with text for performance)
    canvas.setTextColor(COLOR_TEXT);
    canvas.setTextSize(2);
    canvas.setCursor(45, 45 + yOffset);
    canvas.print("3D CAD");

    // Gesture status
    drawRoundedCard(10, 100 + yOffset, 140, 20, 0x3186);
    canvas.setTextSize(1);
    canvas.setCursor(15, 106 + yOffset);
    canvas.print("ACT: ");
    canvas.print(lastGesture);
}

void DisplayManager::drawMusicUI() {
    int yOffset = isTransitioning ? modeTransitionY : 0;

    // Header
    canvas.setTextColor(COLOR_TEXT);
    canvas.setTextSize(1);
    canvas.setCursor(10, 10 + yOffset);
    canvas.print("NOW PLAYING");

    // Main Card
    drawRoundedCard(10, 25 + yOffset, 140, 70, COLOR_CARD);
    
    // Scrolling Title
    canvas.setTextWrap(false);
    canvas.setTextSize(1);
    canvas.setCursor(15 - scrollOffset, 35 + yOffset);
    canvas.print(currentSong);
    
    if (currentSong.length() > 20) {
        scrollOffset++;
        if (scrollOffset > (currentSong.length() * 6)) scrollOffset = -140;
    } else {
        scrollOffset = 0;
    }

    canvas.setTextColor(0xAD75); // Light grey
    canvas.setCursor(15, 50 + yOffset);
    canvas.print(currentArtist);

    // Controls mock UI
    canvas.setTextColor(COLOR_TEXT);
    canvas.setCursor(20, 75 + yOffset);
    canvas.print("|<   ||   >|");

    // Volume Bar
    canvas.setCursor(10, 105 + yOffset);
    canvas.print("VOL");
    drawProgressBar(35, 105 + yOffset, 115, 8, volume);
}