#include <Wire.h>
#include <RevEng_PAJ7620.h>
#include "Config.h"
#include "DisplayManager.h"

RevEng_PAJ7620 sensor = RevEng_PAJ7620();
DisplayManager display;
AppMode currentMode = MODE_CAD;

unsigned long lastGestureTime = 0;
String inputString = "";
bool stringComplete = false;

void setup() {
    Serial.begin(115200);
    Wire.begin(I2C_SDA, I2C_SCL);
    
    display.begin();
    
    if (!sensor.begin()) {
        Serial.println("SYS:SENSOR_ERROR");
        while(1) { delay(100); }
    }
    Serial.println("SYS:READY");
}

void processSerialCommand(String cmd) {
    if (cmd.startsWith("SONG:")) {
        int split = cmd.indexOf('|');
        if (split > -1) {
            String title = cmd.substring(5, split);
            String artist = cmd.substring(split + 1);
            display.setSongData(title, artist);
        }
    } else if (cmd.startsWith("VOL:")) {
        display.setVolume(cmd.substring(4).toInt());
    }
}

void loop() {
    // 1. Handle Serial Data from Python
    while (Serial.available()) {
        char inChar = (char)Serial.read();
        if (inChar == '\n') {
            processSerialCommand(inputString);
            inputString = "";
        } else if (inChar != '\r') {
            inputString += inChar;
        }
    }

    // 2. Handle Gestures
    Gesture gesture;
    if (sensor.readGesture(gesture)) {
        if (millis() - lastGestureTime > 500) { // Debounce
            lastGestureTime = millis();
            String gName = "";

            switch (gesture) {
                case GES_CLOCKWISE:
                    currentMode = MODE_MUSIC;
                    display.setMode(currentMode);
                    gName = "MODE_SWITCH";
                    Serial.println("MODE:MUSIC");
                    break;
                case GES_ANTICLOCKWISE:
                    currentMode = MODE_CAD;
                    display.setMode(currentMode);
                    gName = "MODE_SWITCH";
                    Serial.println("MODE:CAD");
                    break;
                case GES_UP: gName = "UP"; break;
                case GES_DOWN: gName = "DOWN"; break;
                case GES_LEFT: gName = "LEFT"; break;
                case GES_RIGHT: gName = "RIGHT"; break;
                case GES_FORWARD: gName = "FORWARD"; break;
                case GES_BACKWARD: gName = "BACKWARD"; break;
                case GES_WAVE: gName = "WAVE"; break;
                default: break;
            }

            if (gName != "" && gName != "MODE_SWITCH") {
                display.setLastGesture(gName);
                Serial.print("GESTURE:");
                Serial.println(gName);
            }
        }
    }

    // 3. Update Display Animation Loop (approx 30fps)
    display.update();
    delay(33); 
}