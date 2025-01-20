#include <M5EPD.h>
#include <memory>
#include <stdexcept>
#include <SdFat.h>
#define SD_FAT_TYPE 1

#include <iostream>
#include <vector>
#include <sstream>
#include <string>
#include <JPEGDEC.h>
#include "filelistdb.h"

#define EINK_REFRESH_DELAY 500
#define REFRESH_TIME 600

// BACKGROUND_BRIGHTNESS fills the border on small images: 0: White... 15: black
#define BACKGROUND_BRIGHTNESS 0

JPEGDEC jpeg;
M5EPD_Canvas canvas(&M5.EPD);
FileListDB db("/index.db", "/");

uint8_t ditherSpace[15360];
int offsetX = 0; // Screen space centering.
int offsetY = 0; // Screen space centering.

File globalFileHandle;

void * myOpen(const char *filename, int32_t *size) {
  globalFileHandle = SD.open(filename);
  *size = globalFileHandle.size();
  return &globalFileHandle;
}

void myClose(void *handle) {
  if (globalFileHandle) globalFileHandle.close();
}

int32_t myRead(JPEGFILE *handle, uint8_t *buffer, int32_t length) {
  if (!globalFileHandle) return 0;
  return globalFileHandle.read(buffer, length);
}

int32_t mySeek(JPEGFILE *handle, int32_t position) {
  if (!globalFileHandle) return 0;
  return globalFileHandle.seek(position);
}

void renderBattery(int level, int xpos, int ypos) {
  uint8_t width = 18;
  uint8_t barwidth = ((width-6)/3);

  canvas.drawLine(xpos + 1, ypos, xpos + width - 4, ypos, 1);
  canvas.drawLine(xpos, ypos + 1, xpos, ypos + 5, 1);
  canvas.drawLine(xpos + 1, ypos + 6, xpos + width - 4, ypos + 6, 1);
  canvas.drawPixel(xpos + width - 3, ypos + 1, 1);
  canvas.drawPixel(xpos + width - 2, ypos + 1, 1);
  canvas.drawLine(xpos + width - 1, ypos + 2, xpos + width - 1, ypos + 4, 1);
  canvas.drawPixel(xpos + width - 2, ypos + 5, 1);
  canvas.drawPixel(xpos + width - 3, ypos + 5, 1);
  canvas.drawPixel(xpos + width - 3, ypos + 6, 1);
  if (level > 90) {
    canvas.fillRect(xpos + 2, ypos + 2, barwidth * 3, 3, 1);
  } else if (level > 75) {
    for (uint8_t i = 0; i < 3; i++) {
      canvas.fillRect(xpos + 2 + (i * barwidth), ypos + 2, barwidth - 1, 3, 1);
    }
  } else if (level > 50) {
    for (uint8_t i = 0; i < 2; i++) {
      canvas.fillRect(xpos + 2 + (i * barwidth), ypos + 2, barwidth - 1, 3, 1);
    }
  } else if (level > 25) {
    canvas.fillRect(xpos + 2, ypos + 2, barwidth - 1, 3, 1);
  }
}

int getBatteryPcnt(){
  uint32_t vol = M5.getBatteryVoltage();
  if (vol < 3300) {
    vol = 3300;
  }
  else if (vol > 4350) {
    vol = 4350;
  }
  float battery = (float)(vol - 3300) / (float)(4350 - 3300);
  if (battery <= 0.01) {
    battery = 0.01;
  }
  if (battery > 1) {
    battery = 1;
  }
  return (int)(battery * 100);
}

void drawMessage(String msg){
    canvas.fillCanvas(BACKGROUND_BRIGHTNESS);
    canvas.setTextSize(3);
    canvas.drawString(msg, 64, 256, 1);
    canvas.pushCanvas(0, 0, UPDATE_MODE_GL16);
}

void drawTempHumidityBattery() {
  canvas.setTextSize(2);
  char batteryBuffer[20];
  int batteryPcnt = getBatteryPcnt();
  sprintf(batteryBuffer, "%d%%", batteryPcnt);
  char statusBuffer[256] = "CHR";

  M5.SHT30.UpdateData();
  float tem = M5.SHT30.GetTemperature();
  float hum = M5.SHT30.GetRelHumidity();

  sprintf(statusBuffer, "%2.1fC %2.1f%%", tem, hum);
  canvas.drawRightString(statusBuffer, 960, 0, 1);
  sprintf(statusBuffer, "%s", batteryBuffer);
  canvas.drawRightString(statusBuffer, 960, 524, 1);
}

int JPEGDraw(JPEGDRAW *pDraw) {
  uint16_t startX = pDraw->x + offsetX;
  uint16_t startY = pDraw->y + offsetY;
  uint16_t width = pDraw->iWidth;
  uint16_t height = pDraw->iHeight;
  uint16_t *pixels = pDraw->pPixels;  // Cache pointer for faster access

  for (int16_t y = 0; y < height; y++) {
    for (int16_t x = 0; x < width; x += 4) {
      uint16_t col = *pixels++;

      // Extract and invert 4 colors from the 16-bit value
      uint8_t colors[4] = {
        0xF - ((col >> 4) & 0xF),
        0xF - (col & 0xF),
        0xF - ((col >> 12) & 0xF),
        0xF - ((col >> 8) & 0xF)
      };

      // Draw 4 pixels
      for (int8_t i = 0; i < 4; i++) {
        canvas.drawPixel(startX + x + i, startY + y, colors[i]);
      }
    }
  }
  return 1;
}

bool loadRandomImage() {
  M5.EPD.Clear(1);
  String fullname = "/";
  fullname += db.getRandomFileName();
  Serial.printf("Filename %s\n", fullname);
  return drawImage((char *) fullname.c_str());
}
 
bool drawImage(char *fileName) {

  uint8_t result = jpeg.open(fileName, myOpen, myClose, myRead, mySeek, JPEGDraw);
  if (result != 1) {
    uint8_t err = jpeg.getLastError();
    switch (err) {
      case 1: Serial.println("Error: JPEG_INVALID_PARAMETER"); break;
      case 2: Serial.println("Error: JPEG_DECODE_ERROR"); break;
      case 3: Serial.println("Error: JPEG_UNSUPPORTED_FEATURE (progressive JPG's not supported)"); break;
      case 4: Serial.println("Error: JPEG_FILE_TOO_SMALL"); delay(5000); break;
      case 5: Serial.println("Error: JPEG_WRONG_MAGIC_NUMBER"); break;
    }
    return false;
  }

  int width = jpeg.getWidth();
  int height = jpeg.getHeight();

  Serial.print("Image size: ");
  Serial.print(width);
  Serial.print(" x ");
  Serial.println(height);

  // Too big? Then scale.
  uint8_t scaling = 0; // Get's or'd with options for the following:
  uint8_t options = 0;
  /*Decoder options
    JPEG_AUTO_ROTATE 1
    JPEG_SCALE_HALF 2
    JPEG_SCALE_QUARTER 4
    JPEG_SCALE_EIGHTH 8
    JPEG_LE_PIXELS 16
    JPEG_EXIF_THUMBNAIL 32
    JPEG_LUMA_ONLY 64
  */
  if (width > 960 || height > 540) {
    // Try half size.
    width >>= 1;
    height >>= 1;
    Serial.print("Trying half size: ");
    Serial.print(width);
    Serial.print(" x ");
    Serial.println(height);
    scaling = JPEG_SCALE_HALF;
    if (width > 960 || height > 540) {
      // Try quarter size.
      width >>= 1;
      height >>= 1;
      scaling = JPEG_SCALE_QUARTER;
      Serial.print("Trying quarter size: ");
      Serial.print(width);
      Serial.print(" x ");
      Serial.println(height);
      if (width > 960 || height > 540) {
        // Try eigth size.
        width >>= 1;
        height >>= 1;
        scaling = JPEG_SCALE_EIGHTH;
        Serial.println("Trying eighth size: ");
        Serial.print(width);
        Serial.print(" x ");
        Serial.println(height);
        if (width > 960 || height > 540) {
          Serial.println("Still too big after scaling attempt, skipping.");
          return false;
        }
      }
    }
    Serial.println("Resized image now fits.");
  }

  // For when this image is smaller than the last - make sure no old image can be seen on the boundaries.
  canvas.fillCanvas(BACKGROUND_BRIGHTNESS);
  offsetX = (960 - width) >> 1;
  offsetY = (540 - height) >> 1;
  jpeg.setPixelType(FOUR_BIT_DITHERED);
  jpeg.decodeDither(ditherSpace, options | scaling);
  jpeg.close();
  drawTempHumidityBattery();
  canvas.pushCanvas(0, 0, UPDATE_MODE_GC16);
  return true;
}

void sleep(int secs){
  // https://cat-in-136.github.io/2022/05/note-m5paper-power-supply-management.html
  M5.disableEPDPower();
  M5.disableEXTPower();
  M5.shutdown(secs);
  Serial.println("Deep sleep failed, using light sleep");
  delay(secs*100);
  ESP.restart();
}

void setup() {
  //setCpuFrequencyMhz(80);
  M5.begin(false,true,true,true,true);
  M5.EPD.SetRotation(0);
  M5.RTC.begin();
  canvas.createCanvas(960, 540);
  Serial.println("Load Image");

  if (!SD.exists(db.getStorageFileName())){
    Serial.printf("\n%s does not exist, creating\n", db.getStorageFileName());
    char buffer[64];
    sprintf(buffer, "Generating %s", db.getStorageFileName());
    drawMessage(buffer);
    delay(EINK_REFRESH_DELAY);
    db.saveFileList();
  }

  while (!loadRandomImage()){
    Serial.println("Error loading image");
    drawMessage("Error loading image");
    delay(EINK_REFRESH_DELAY);
    sleep();
  }
  delay(EINK_REFRESH_DELAY);
  sleep(REFRESH_TIME);
}

void loop() { }
