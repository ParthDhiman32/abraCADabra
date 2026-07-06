#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <Wire.h>
#include "paj7620.h"

/*
  ST7735 "Now Playing" Display + Gesture Sensor
  ----------------------------------------------------------------
  Now-playing screen (track/artist/progress/volume + status bar)
  driven over Serial, plus PAJ7620 gesture detection.

  Playback data comes in over Serial as a single line:
    TRACK:<track>|<artist>|<posSec>|<durSec>|<volume0to100>|<playing0or1>
  Status data (optional) as:
    STAT:<HH:MM>|<batteryPct>|<linkUp0or1>

  Example lines a PC-side Python script (e.g. using pycaw / winrt media
  session APIs) could send every second:
    TRACK:Blinding Lights|The Weeknd|87|200|72|1
    STAT:14:32|91|1

  Library: Adafruit_GFX + Adafruit_ST7735 (install both via Library Manager)
*/

// Display / sensor pins
#define TFT_CS    D4
#define TFT_DC    D3
#define TFT_RST   D2
#define TFT_MOSI  D1
#define TFT_SCLK  D0
#define I2C_SDA   D5
#define I2C_SCL   D6

#define GES_REACTION_TIME  100
#define GES_QUIT_TIME      500

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// ---- Colors (565) ----
#define BG        ST7735_BLACK
#define FG        ST7735_WHITE
#define ACCENT    0x07FF   // cyan-ish
#define DIM       0x39C7   // gray
#define BAR_BG    0x2104   // dark gray
#define BAR_FILL  ST7735_GREEN

// ---- Playback state ----
struct PlaybackState {
  String track    = "No Track";
  String artist   = "--";
  int    posSec   = 0;
  int    durSec   = 0;
  int    volume   = 0;
  bool   playing  = false;
} pb;

// ---- Status bar state ----
struct StatusState {
  String clock   = "--:--";
  int    battery = 100;
  bool   linkUp  = false;
} sysStat;

// Track previous drawn values so we only redraw what changed (avoids flicker)
String lastTrack = "", lastArtist = "", lastClock = "";
int lastPos = -1, lastDur = -1, lastVolume = -1, lastBattery = -1;
bool lastPlaying = true, lastLink = true;
bool firstDraw = true;

unsigned long lastSerialByte = 0;
String serialBuf = "";

void readSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      parseLine(serialBuf);
      serialBuf = "";
    } else if (c != '\r') {
      serialBuf += c;
    }
  }
}

void parseLine(String line) {
  line.trim();
  if (line.startsWith("TRACK:")) {
    String data = line.substring(6);
    // fields: track|artist|pos|dur|vol|playing
    String fields[6];
    int idx = 0;
    int start = 0;
    for (int i = 0; i <= data.length() && idx < 6; i++) {
      if (i == data.length() || data.charAt(i) == '|') {
        fields[idx++] = data.substring(start, i);
        start = i + 1;
      }
    }
    if (idx >= 6) {
      pb.track   = fields[0];
      pb.artist  = fields[1];
      pb.posSec  = fields[2].toInt();
      pb.durSec  = fields[3].toInt();
      pb.volume  = fields[4].toInt();
      pb.playing = fields[5].toInt() == 1;
      drawScreen();
    }
  } else if (line.startsWith("STAT:")) {
    String data = line.substring(5);
    String fields[3];
    int idx = 0;
    int start = 0;
    for (int i = 0; i <= data.length() && idx < 3; i++) {
      if (i == data.length() || data.charAt(i) == '|') {
        fields[idx++] = data.substring(start, i);
        start = i + 1;
      }
    }
    if (idx >= 3) {
      sysStat.clock   = fields[0];
      sysStat.battery = fields[1].toInt();
      sysStat.linkUp  = fields[2].toInt() == 1;
      drawScreen();
    }
  }
}

// ---- Layout constants ----
const int SCR_W = 128, SCR_H = 160;
const int STATUS_Y = 4;
const int TRACK_Y  = 40;
const int ARTIST_Y = 58;
const int BAR_Y    = 90;
const int BAR_H    = 8;
const int TIME_Y   = 104;
const int VOL_Y    = 124;
const int ICON_Y   = 140;

void drawStaticLayout() {
  tft.drawFastHLine(0, 20, SCR_W, DIM);
  tft.drawFastHLine(0, 118, SCR_W, DIM);
}

String fmtTime(int totalSec) {
  int m = totalSec / 60;
  int s = totalSec % 60;
  char buf[8];
  sprintf(buf, "%d:%02d", m, s);
  return String(buf);
}

void drawScreen() {
  // --- Status bar: clock (left), battery (right) ---
  if (sysStat.clock != lastClock || firstDraw) {
    tft.fillRect(0, STATUS_Y, 60, 12, BG);
    tft.setTextColor(FG, BG);
    tft.setTextSize(1);
    tft.setCursor(2, STATUS_Y);
    tft.print(sysStat.clock);
    lastClock = sysStat.clock;
  }
  if (sysStat.battery != lastBattery || firstDraw) {
    tft.fillRect(90, STATUS_Y, 38, 12, BG);
    tft.setTextColor(sysStat.battery < 20 ? ST7735_RED : FG, BG);
    tft.setCursor(90, STATUS_Y);
    tft.print(sysStat.battery);
    tft.print("%");
    lastBattery = sysStat.battery;
  }
  if (sysStat.linkUp != lastLink || firstDraw) {
    tft.fillCircle(122, 8, 3, sysStat.linkUp ? ACCENT : ST7735_RED);
    lastLink = sysStat.linkUp;
  }

  // --- Track name ---
  if (pb.track != lastTrack || firstDraw) {
    tft.fillRect(0, TRACK_Y - 4, SCR_W, 16, BG);
    tft.setTextColor(FG, BG);
    tft.setTextSize(1);
    String t = pb.track;
    if (t.length() > 20) t = t.substring(0, 20);
    tft.setCursor((SCR_W - t.length() * 6) / 2, TRACK_Y);
    tft.print(t);
    lastTrack = pb.track;
  }

  // --- Artist name ---
  if (pb.artist != lastArtist || firstDraw) {
    tft.fillRect(0, ARTIST_Y - 4, SCR_W, 12, BG);
    tft.setTextColor(DIM, BG);
    String a = pb.artist;
    if (a.length() > 22) a = a.substring(0, 22);
    tft.setCursor((SCR_W - a.length() * 6) / 2, ARTIST_Y);
    tft.print(a);
    lastArtist = pb.artist;
  }

  // --- Progress bar ---
  if (pb.posSec != lastPos || pb.durSec != lastDur || firstDraw) {
    int barW = SCR_W - 16;
    int fillW = pb.durSec > 0 ? (int)((float)pb.posSec / pb.durSec * barW) : 0;
    tft.drawRect(8, BAR_Y, barW, BAR_H, DIM);
    tft.fillRect(9, BAR_Y + 1, barW - 2, BAR_H - 2, BAR_BG);
    if (fillW > 2) tft.fillRect(9, BAR_Y + 1, fillW - 2, BAR_H - 2, BAR_FILL);

    // time labels
    tft.fillRect(0, TIME_Y, SCR_W, 10, BG);
    tft.setTextColor(DIM, BG);
    tft.setCursor(8, TIME_Y);
    tft.print(fmtTime(pb.posSec));
    String dstr = fmtTime(pb.durSec);
    tft.setCursor(SCR_W - 8 - dstr.length() * 6, TIME_Y);
    tft.print(dstr);

    lastPos = pb.posSec;
    lastDur = pb.durSec;
  }

  // --- Play/pause glyph ---
  if (pb.playing != lastPlaying || firstDraw) {
    tft.fillRect(SCR_W / 2 - 6, ICON_Y, 12, 12, BG);
    if (pb.playing) {
      tft.fillTriangle(SCR_W / 2 - 4, ICON_Y, SCR_W / 2 - 4, ICON_Y + 12,
                        SCR_W / 2 + 6, ICON_Y + 6, ACCENT);
    } else {
      tft.fillRect(SCR_W / 2 - 5, ICON_Y, 4, 12, ACCENT);
      tft.fillRect(SCR_W / 2 + 2, ICON_Y, 4, 12, ACCENT);
    }
    lastPlaying = pb.playing;
  }

  // --- Volume bar ---
  if (pb.volume != lastVolume || firstDraw) {
    int volBarW = 60;
    int volX = (SCR_W - volBarW) / 2;
    tft.fillRect(volX, VOL_Y, volBarW, 6, BG);
    tft.drawRect(volX, VOL_Y, volBarW, 6, DIM);
    int fillW = (int)(pb.volume / 100.0 * (volBarW - 2));
    if (fillW > 0) tft.fillRect(volX + 1, VOL_Y + 1, fillW, 4, ACCENT);
    lastVolume = pb.volume;
  }

  firstDraw = false;
}

void setup() {
  Wire.begin(I2C_SDA, I2C_SCL);
  Serial.begin(115200);

  uint8_t error = paj7620Init();

  if (error) {
    Serial.print("INIT ERROR,CODE:");
    Serial.println(error);
  } else {
    Serial.println("INIT OK");
  }
  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);
  tft.fillScreen(BG);
  drawStaticLayout();
}

void loop() {
  readSerial();

  // Local fallback: if PC isn't sending updates, simulate progress so the
  // screen still looks alive on the bench.
  static unsigned long lastTick = 0;
  if (millis() - lastTick > 1000) {
    lastTick = millis();
    if (pb.playing && pb.durSec > 0 && pb.posSec < pb.durSec) {
      pb.posSec++;
    }
    drawScreen();
  }

  uint8_t data = 0, data1 = 0, error;
  error = paj7620ReadReg(0x43, 1, &data);

  if (!error) {
    switch (data) {
      case GES_RIGHT_FLAG:
				delay(GES_REACTION_TIME);
				paj7620ReadReg(0x43, 1, &data);
				if(data == GES_LEFT_FLAG) 
				{
					Serial.println("Right-Left");
				}
				else if(data == GES_FORWARD_FLAG) 
				{
					Serial.println("Forward");
					delay(GES_QUIT_TIME);
				}
				else if(data == GES_BACKWARD_FLAG) 
				{
					Serial.println("Backward");
					delay(GES_QUIT_TIME);
				}
				else
				{
					Serial.println("Right");
				}          
				break;
			case GES_LEFT_FLAG:
				delay(GES_REACTION_TIME);
				paj7620ReadReg(0x43, 1, &data);
				if(data == GES_RIGHT_FLAG) 
				{
					Serial.println("Left-Right");
				}
				else if(data == GES_FORWARD_FLAG) 
				{
					Serial.println("Forward");
					delay(GES_QUIT_TIME);
				}
				else if(data == GES_BACKWARD_FLAG) 
				{
					Serial.println("Backward");
					delay(GES_QUIT_TIME);
				}
				else
				{
					Serial.println("Left");
				}          
				break;
				break;
			case GES_UP_FLAG:
				delay(GES_REACTION_TIME);
				paj7620ReadReg(0x43, 1, &data);
				if(data == GES_DOWN_FLAG) 
				{
					Serial.println("Up-Down");
				}
				else if(data == GES_FORWARD_FLAG) 
				{
					Serial.println("Forward");
					delay(GES_QUIT_TIME);
				}
				else if(data == GES_BACKWARD_FLAG) 
				{
					Serial.println("Backward");
					delay(GES_QUIT_TIME);
				}
				else
				{
					Serial.println("Up");
				}
				break;
			case GES_DOWN_FLAG:
				delay(GES_REACTION_TIME);
				paj7620ReadReg(0x43, 1, &data);
				if(data == GES_UP_FLAG) 
				{
					Serial.println("Down-Up");
				}
				else if(data == GES_FORWARD_FLAG) 
				{
					Serial.println("Forward");
					delay(GES_QUIT_TIME);
				}
				else if(data == GES_BACKWARD_FLAG) 
				{
					Serial.println("Backward");
					delay(GES_QUIT_TIME);
				}
				else
				{
					Serial.println("Down");
				}
				break;
			case GES_FORWARD_FLAG:
				delay(GES_REACTION_TIME);
				paj7620ReadReg(0x43, 1, &data);
				if(data == GES_BACKWARD_FLAG) 
				{
					Serial.println("Forward-Backward");
					delay(GES_QUIT_TIME);
				}
				else
				{
					Serial.println("Forward");
					delay(GES_QUIT_TIME);
				}
				break;
			case GES_BACKWARD_FLAG:		  
				delay(GES_REACTION_TIME);
				paj7620ReadReg(0x43, 1, &data);
				if(data == GES_FORWARD_FLAG) 
				{
					Serial.println("Backward-Forward");
					delay(GES_QUIT_TIME);
				}
				else
				{
					Serial.println("Backward");
					delay(GES_QUIT_TIME);
				}
				break;
			case GES_CLOCKWISE_FLAG:
				Serial.println("Clockwise");
				break;
			case GES_COUNT_CLOCKWISE_FLAG:
				Serial.println("anti-clockwise");
				break;  
			default:
				paj7620ReadReg(0x44, 1, &data1);
				if (data1 == GES_WAVE_FLAG) 
				{
					Serial.println("wave");
				}
				break;
		}
	}
	delay(100);
}