#ifdef ESP32
  #include "SPIFFS.h" // ESP32 only
#endif

#include "SPI.h"
#include <TFT_eSPI.h>   // Hardware-specific library
TFT_eSPI tft = TFT_eSPI();         // Invoke custom library

#include <TFT_eWidget.h>    

#include "OpenFontRender.h"
OpenFontRender ofr_tft;

#include "NotoSans_Bold.h"
#define TTF_FONT NotoSans_Bold

GraphWidget contact_graph = GraphWidget(&tft);
TraceWidget contact_trace = TraceWidget(&contact_graph);

GraphWidget angle_graph = GraphWidget(&tft);
TraceWidget angle_trace = TraceWidget(&angle_graph);

GraphWidget accel_graph = GraphWidget(&tft);
TraceWidget accel_trace = TraceWidget(&accel_graph);

#include <TinyPICO.h>
TinyPICO tp = TinyPICO();

#include "WiFi.h"
#include <esp_now.h>

typedef struct struct_message {
    int index;
    int max_index;
    float time_x;
    int contact;
    int angle;
    float accel;
} struct_message;

int Array_index[1300];
float Array_time_x[1300];
int Array_contact[1300];
int Array_angle[1300];
float Array_accel[1300];

struct_message gloveData;

esp_now_peer_info_t peerInfo;

int iti_number = 0;
int max_iti_number = 0;
int LED_progress = 0;

const int analogPin = 15;

#define BACKGROUND   0x29AA
#define GREENish     0x47A9
#define BLUEish      0x2E5C
#define PINKish      0xF16F
#define YELLOWish    0xFEE9

int previous_interface = 11;

bool finished = false;

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  if (finished == false) {
    memcpy(&gloveData, incomingData, sizeof(gloveData));
    
    Array_index[gloveData.index] = gloveData.index;
    max_iti_number = gloveData.max_index-1;
    Array_time_x[gloveData.index] = gloveData.time_x;
    Array_contact[gloveData.index] = gloveData.contact;
    Array_angle[gloveData.index] = gloveData.angle;
    Array_accel[gloveData.index] = gloveData.accel;
    /*
      ofr_tft.setFontSize(36);
      ofr_tft.setCursor(26, 130); // Middle of box
      ofr_tft.setAlignment(Align::MiddleLeft);
      ofr_tft.setFontColor(TFT_WHITE, BACKGROUND);
      ofr_tft.printf("Receiving data...");
    */
    if (LED_progress == 0) {
        tp.DotStar_SetPixelColor(0, 0, 255);
      }
      if (LED_progress == 5) {
        tp.DotStar_Clear();
      }
      LED_progress++;
      if (LED_progress == 10) {
        LED_progress = 0;
      }
  
    if (gloveData.index == max_iti_number) {

      finished = true;
      tp.DotStar_Clear();
    }
  }
  
}

void setup()
{
  Serial.begin(115200);

  if (!SPIFFS.begin()) {
    Serial.println("SPIFFS initialisation failed!");
    while (1) yield(); // Stay here twiddling thumbs waiting
  }
  Serial.println("\r\nInitialisation done.");

  WiFi.mode(WIFI_STA);
  //WiFi.mode(WIFI_MODE_STA);
  //Serial.println(WiFi.macAddress());
  
  if (esp_now_init() != ESP_OK) {
  Serial.println("Error initializing ESP-NOW");
  return;
  }

  esp_now_register_recv_cb(OnDataRecv);
  
  tft.begin();


  ofr_tft.setDrawer(tft);
  ofr_tft.loadFont(TTF_FONT, sizeof(TTF_FONT));  
  delay(500);

  // Time recorded for test purposes
  uint32_t t = millis();

  drawBmp("/logoram.bmp", 0, 0);
  
  // How much time did rendering take (ESP8266 80MHz 271ms, 160MHz 157ms, ESP32 SPI 120ms, 8bit parallel 105ms
  t = millis() - t;
  Serial.print(t); Serial.println(" ms");

  // Wait before drawing again
  delay(1000);
  
  tft.setTextColor(TFT_WHITE, BACKGROUND); 
  tft.fillScreen(BACKGROUND);
  DrawTitle();
  
  for (int i=0; i<1300; i++) {
    Array_index[i] = 0;
    Array_time_x[i] = 0.00;
    Array_contact[i] = 0;
    Array_angle[i] = 0;
    Array_accel[i] = 0.00;    
  }
  
}

void loop()
{
if (finished == true) {


      tft.fillScreen(BACKGROUND);
      DrawTitle();
      ofr_tft.setFontSize(36);
      ofr_tft.setCursor(26, 59); // Middle of box
      ofr_tft.setAlignment(Align::MiddleLeft);
      ofr_tft.setFontColor(GREENish, BACKGROUND);
      ofr_tft.printf("Contact"); 
    
      ofr_tft.setCursor(26, 130); // Middle of box
      ofr_tft.setAlignment(Align::MiddleLeft);
      ofr_tft.setFontColor(BLUEish, BACKGROUND);
      ofr_tft.printf("Angle"); 
    
      ofr_tft.setCursor(26, 222); // Middle of box
      ofr_tft.setAlignment(Align::MiddleLeft);
      ofr_tft.setFontColor(PINKish, BACKGROUND);
      ofr_tft.printf("Acceleration"); 
      
      tft.setTextColor(TFT_WHITE, BACKGROUND); 
      tft.setTextSize(1); 
    
      contact_graph.createGraph(200, 36, tft.color565(23, 37, 60));
      contact_graph.setGraphScale(0.0, 10.0, 0.0, 1.0);
      contact_graph.setGraphGrid(0.0, 1.0, 0.0, 0.5, tft.color565(73, 77, 109));
      contact_graph.drawGraph(20, 76);
      
      tft.setTextDatum(MR_DATUM); // Middle right text datum
      tft.drawNumber(0, contact_graph.getPointX(0.0), contact_graph.getPointY(0.0));
      tft.drawNumber(1, contact_graph.getPointX(0.0), contact_graph.getPointY(1.0));
    
      angle_graph.createGraph(200, 60, tft.color565(23, 37, 60));
      angle_graph.setGraphScale(0.0, 10.0, -10.0, 100.0);
      angle_graph.setGraphGrid(0.0, 1.0, 0.0, 45.0, tft.color565(73, 77, 109));
      angle_graph.drawGraph(20, 146);
      
      tft.setTextDatum(MR_DATUM); // Middle right text datum
      tft.drawNumber(0, angle_graph.getPointX(0.0), angle_graph.getPointY(0.0));
      tft.drawNumber(45, angle_graph.getPointX(0.0), angle_graph.getPointY(45.0));
      tft.drawNumber(90, angle_graph.getPointX(0.0), angle_graph.getPointY(90.0));
      
      accel_graph.createGraph(200, 60, tft.color565(23, 37, 60));
      accel_graph.setGraphScale(0.0, 10.0, -5.0, 5.0);
      accel_graph.setGraphGrid(0.0, 1.0, -3.0, 3.0, tft.color565(73, 77, 109));
      accel_graph.drawGraph(20, 240);
      
      tft.setTextDatum(MR_DATUM); // Middle right text datum
      tft.drawNumber(-3, accel_graph.getPointX(0.0), accel_graph.getPointY(-3.0));
      tft.drawNumber(0, accel_graph.getPointX(0.0), accel_graph.getPointY(0.0));
      tft.drawNumber(3, accel_graph.getPointX(0.0), accel_graph.getPointY(3.0));
    
        // Draw the x axis scale
      tft.setTextDatum(TC_DATUM); // Top centre text datum
      tft.drawNumber(0, accel_graph.getPointX(0.0), accel_graph.getPointY(-5.0) + 3);
      tft.drawNumber(5, accel_graph.getPointX(5.0), accel_graph.getPointY(-5.0) + 3);
      tft.drawNumber(10, accel_graph.getPointX(10.0), accel_graph.getPointY(-5.0) + 3);
    
      contact_trace.startTrace(GREENish);
      angle_trace.startTrace(BLUEish);
      accel_trace.startTrace(PINKish);

  for (int i=0; i<max_iti_number; i++) {
    contact_trace.addPoint(Array_time_x[i], Array_contact[i]);
    angle_trace.addPoint(Array_time_x[i], Array_angle[i]);
    accel_trace.addPoint(Array_time_x[i], Array_accel[i]);    
  }
  
  finished = false;
}

  
  /*
  //int inputValue = analogRead(analogPin);
  int interface;

  //interface = map(inputValue, 0, 4095, 0, 4);
  interface = 1;
  
  tft.setCursor(0, 300);
  tft.setTextColor(TFT_WHITE, BACKGROUND); 
  tft.setTextSize(1); 
  tft.print("testing...");


  tp.DotStar_SetBrightness(20);

  if (interface != previous_interface) {
  DrawTitle(interface);
  }

  delay(3000);
  */
}


void DrawTitle() {
  
  uint16_t Dom_Color;

  String Text;

  Dom_Color = BLUEish;  // pinkish
  tp.DotStar_SetPixelColor( 0, 255, 0 );

  tft.fillScreen(BACKGROUND);
  tft.fillRect(0,0,240,40, Dom_Color);
  tft.fillRect(0,40,240,4, TFT_WHITE);

  ofr_tft.setFontSize(64);
  ofr_tft.setCursor(120, 8); // Middle of box
  ofr_tft.setAlignment(Align::MiddleCenter);
  ofr_tft.setFontColor(TFT_WHITE, Dom_Color);
  ofr_tft.printf("RESULTS"); 
}

void drawBmp(const char *filename, int16_t x, int16_t y) {

  if ((x >= tft.width()) || (y >= tft.height())) return;

  fs::File bmpFS;

  // Open requested file on SD card
  bmpFS = SPIFFS.open(filename, "r");

  if (!bmpFS)
  {
    Serial.print("File not found");
    return;
  }

  uint32_t seekOffset;
  uint16_t w, h, row, col;
  uint8_t  r, g, b;

  uint32_t startTime = millis();

  if (read16(bmpFS) == 0x4D42)
  {
    read32(bmpFS);
    read32(bmpFS);
    seekOffset = read32(bmpFS);
    read32(bmpFS);
    w = read32(bmpFS);
    h = read32(bmpFS);

    if ((read16(bmpFS) == 1) && (read16(bmpFS) == 24) && (read32(bmpFS) == 0))
    {
      y += h - 1;

      bool oldSwapBytes = tft.getSwapBytes();
      tft.setSwapBytes(true);
      bmpFS.seek(seekOffset);

      uint16_t padding = (4 - ((w * 3) & 3)) & 3;
      uint8_t lineBuffer[w * 3 + padding];

      for (row = 0; row < h; row++) {
        
        bmpFS.read(lineBuffer, sizeof(lineBuffer));
        uint8_t*  bptr = lineBuffer;
        uint16_t* tptr = (uint16_t*)lineBuffer;
        // Convert 24 to 16-bit colours
        for (uint16_t col = 0; col < w; col++)
        {
          b = *bptr++;
          g = *bptr++;
          r = *bptr++;
          *tptr++ = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
        }

        // Push the pixel row to screen, pushImage will crop the line if needed
        // y is decremented as the BMP image is drawn bottom up
        tft.pushImage(x, y--, w, 1, (uint16_t*)lineBuffer);
      }
      tft.setSwapBytes(oldSwapBytes);
      Serial.print("Loaded in "); Serial.print(millis() - startTime);
      Serial.println(" ms");
    }
    else Serial.println("BMP format not recognized.");
  }
  bmpFS.close();
}

// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.

uint16_t read16(fs::File &f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(fs::File &f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}
