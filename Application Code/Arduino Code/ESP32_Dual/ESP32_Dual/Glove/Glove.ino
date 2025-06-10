#ifdef ESP32
  #include "SPIFFS.h" // ESP32 only
#endif

#include "SPI.h"
#include <TFT_eSPI.h>   // Hardware-specific library
TFT_eSPI tft = TFT_eSPI();         // Invoke custom library

#include <TFT_eWidget.h>    

#include "OpenFontRender.h"
OpenFontRender ofr;
OpenFontRender ofr_tft;

#include "NotoSans_Bold.h"
#define TTF_FONT NotoSans_Bold
TFT_eSprite sprite = TFT_eSprite(&tft);

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

uint8_t broadcastAddress[] = {0x64, 0xB7, 0x08, 0x90, 0x41, 0x7C};

typedef struct struct_message {
    int index;
    int max_index;
    float time_x;
    int contact;
    int angle;
    float accel;
} struct_message;

struct_message gloveData;

int Array_index[1300];
float Array_time_x[1300];
int Array_contact[1300];
int Array_angle[1300];
float Array_accel[1300];

int previousContact = -9;
int previousAngle = -9;
float previousAccel = -9.00;
int previousProgress = -1;

esp_now_peer_info_t peerInfo;

const int contactPin = 14;
const int anglePin = 15;
const int accelPin = 27;

#define BACKGROUND   0x29AA
#define GREENish     0x47A9
#define BLUEish      0x2E5C
#define PINKish      0xF16F
#define YELLOWish    0xFEE9

int iti_number = 0;
int max_iti_number = 0;
int LED_progress = 0;

bool record_status = false;
bool sending_error = false;
uint32_t t_start = 0;


void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void setup()
{
  Serial.begin(115200);

  if (!SPIFFS.begin()) {
    Serial.println("SPIFFS initialisation failed!");
    while (1) yield(); // Stay here twiddling thumbs waiting
  }
  Serial.println("\r\nInitialisation done.");

  tft.begin();
  tft.setTextColor(0xFFFF, 0x0000);
  tft.fillScreen(BACKGROUND);

  ofr.setDrawer(sprite);
  ofr.loadFont(TTF_FONT, sizeof(TTF_FONT));
  
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

  for (int i = 0; i<50; i++)
    {
      tp.DotStar_SetPixelColor( i*5, 0, 0 );
      delay(15);
    }
  for (int i = 0; i<50; i++)
    {
      tp.DotStar_SetPixelColor( 0, 0, i*5 );
      delay(15);
    }
  for (int i = 0; i<50; i++)
    {
      tp.DotStar_SetPixelColor( 0, i*5, 0 );
      delay(15);
    }
  tp.DotStar_SetPixelColor( 0, 255, 0 );

  t_start = millis()-10001;
  
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
  
  uint32_t t_length;
  t_length = millis() - t_start;

  if (t_length >= 10000) {
    if (record_status == false) {
      WiFi.mode(WIFI_OFF);
      delay(100);
      record_status = true;
      iti_number = 0;

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
      //tft.setTextSize(2); 
    
      //tft.setCursor(26, 60);
      //tft.println("Contact:");
      
      //tft.setCursor(26, 130);
      //tft.println("Angle:");
    
      //tft.setCursor(26, 220);
      //tft.println("Z accel.:");
    
      //tp.DotStar_SetPixelColor(255, 0, 0);
    
      // tp.DotStar_Set2tness(20);
    
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

      tp.DotStar_SetPixelColor(255, 255, 0);
      delay(1000);
      tp.DotStar_SetPixelColor(0, 255, 0);
      sending_error = false;
      t_start = millis();
      t_length = 0;
      previousContact = -9;
      previousAngle = -9;
      previousAccel = -9.00;
    } else {
      record_status = false;
      
      WiFi.mode(WIFI_STA);
     
      // Init ESP-NOW
      if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
      }
    
      // Once ESPNow is successfully Init, we will register for Send CB to
      // get the status of Transmitted packet
      esp_now_register_send_cb(OnDataSent);
      
        // Register peer
      memcpy(peerInfo.peer_addr, broadcastAddress, 6);
      peerInfo.channel = 0;  
      peerInfo.encrypt = false;
      
      // Add peer        
      if (esp_now_add_peer(&peerInfo) != ESP_OK){
        Serial.println("Failed to add peer");
        return;
      }
   
      tft.fillScreen(BACKGROUND);
      DrawTitle();

      ofr_tft.setFontSize(36);
      ofr_tft.setCursor(26, 170); // Middle of box
      ofr_tft.setAlignment(Align::MiddleLeft);
      ofr_tft.setFontColor(TFT_WHITE, BACKGROUND);
      int frequency = (iti_number-1)/10;
      ofr_tft.printf("Frequency:  %d Hz", frequency);
      
      ofr_tft.setFontSize(36);
      ofr_tft.setCursor(26, 130); // Middle of box
      ofr_tft.setAlignment(Align::MiddleLeft);
      ofr_tft.setFontColor(TFT_WHITE, BACKGROUND);
      ofr_tft.printf("Sending data...");
      
      //tft.setTextColor(TFT_WHITE, BACKGROUND); 
      //tft.setTextSize(1); 
      //tft.setCursor(0, 0);
      //tft.println(iti_number-1);
      iti_number = 0;
      t_start = millis();
      delay(100);
    }
  }

  if (record_status == true)
  {
    int angleValueRaw = analogRead(anglePin);
    int angleValue = int(double(1870-angleValueRaw)/7.22);
    
    int accelValueRaw = analogRead(accelPin);
    int accelValue = map(accelValueRaw, 0, 4095, 0, 3300);
    float accelValueFloat = (accelValue-1650)/330.0;
    
    int contactValue = digitalRead(contactPin);    

    float time_x = float(t_length)/1000.0;

    if ((previousContact != contactValue) or (previousAngle != angleValue) or (previousAccel != accelValueFloat)) {
      void *out = sprite.createSprite(70, 24);
      if (out == NULL) Serial.println("Could not allocate memory for sprite!");
    } else delay(5);

    if (previousContact != contactValue) {
        sprite.fillSprite(BACKGROUND);
        ofr.setFontSize(36);
        ofr.setCursor(sprite.width()-2, -2); // Middle of box
        ofr.setAlignment(Align::TopRight);
        ofr.setFontColor(TFT_WHITE, BACKGROUND);
        ofr.printf("%d", contactValue); 
        sprite.pushSprite(150,52);
    }
   
    //tft.setTextSize(2); 
  
    //tft.setCursor(140, 60);
    //tft.print(contactValue);
    //tft.println("   ");
    
    contact_trace.addPoint(time_x, contactValue);
    previousContact = contactValue;

    if (previousAngle != angleValue) {
        sprite.fillSprite(BACKGROUND);
        ofr.setCursor(sprite.width()-2, -2); // Middle of box
        ofr.setAlignment(Align::TopRight);
        ofr.setFontColor(TFT_WHITE, BACKGROUND);
        ofr.printf("%d deg", angleValue);
        sprite.pushSprite(150,121);
    }
    
    //tft.setCursor(120, 130);
    //tft.print(angleValue);
    //tft.print(" deg  ");

    angle_trace.addPoint(time_x, angleValue);
    previousAngle = angleValue;
  
    //tft.setCursor(140, 220);
    //tft.print(accelValueFloat);
    //tft.println(" g ");

    if (previousAccel != accelValueFloat) {
        int first_part = abs(accelValueFloat);
        float buffer_part = (abs(accelValueFloat) - first_part)*100.00;
        int second_part = buffer_part;
      
        char* extra_zero = "0";
        if (second_part >= 10) extra_zero="";
        
        char* str = "";
        if (accelValueFloat<0) str = "-";

        sprite.fillSprite(BACKGROUND);
        ofr.setCursor(sprite.width()-2, -2); // Middle of box
        ofr.setAlignment(Align::TopRight);
        ofr.setFontColor(TFT_WHITE, BACKGROUND);
        ofr.printf("%s%d.%s%d g", str, first_part, extra_zero, second_part);
        sprite.pushSprite(150,216);
    }

    
    accel_trace.addPoint(time_x, accelValueFloat);
    previousAccel = accelValueFloat;

    
    Array_index[iti_number] = iti_number;
    Array_time_x[iti_number] = time_x;
    Array_contact[iti_number] = contactValue;
    Array_angle[iti_number] = angleValue;
    Array_accel[iti_number] = accelValueFloat;
    
    max_iti_number = iti_number;
    
    iti_number++;

    delay(3);
    
  }
  
  if (record_status == false) {
    if (iti_number < max_iti_number) {
      gloveData.index = Array_index[iti_number];
      gloveData.max_index = max_iti_number;
      gloveData.time_x = Array_time_x[iti_number];
      gloveData.contact = Array_contact[iti_number];
      gloveData.angle = Array_angle[iti_number];
      gloveData.accel = Array_accel[iti_number]; 
      
      esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &gloveData, sizeof(gloveData));

      int Progress = round(iti_number*100 / max_iti_number);

      if (previousProgress != Progress) {
        void *out = sprite.createSprite(46, 24);
        if (out == NULL) Serial.println("Could not allocate memory for sprite!");
        sprite.fillSprite(BACKGROUND);
        ofr.setFontSize(36);
        ofr.setCursor(sprite.width()-2, -2); // Middle of box
        ofr.setAlignment(Align::TopRight);
        ofr.setFontColor(YELLOWish, BACKGROUND);
        ofr.printf("%d %%", Progress);
        sprite.pushSprite(154,122);
      }
        
      int color = 255;
      
      if (result == ESP_OK) {
        Serial.println("Sent Successfully");
        color = 255;
      }
      else {
        Serial.println("Error sending the data");
        color = 0;
        sending_error = true;
      }
      
      if (LED_progress == 0) {
        tp.DotStar_SetPixelColor(255-color, 0, color);
      }
      if (LED_progress == 5) {
        tp.DotStar_Clear();
      }
      LED_progress++;
      if (LED_progress == 10) {
        LED_progress = 0;
      }

      
      if (iti_number == max_iti_number-1) {
        tp.DotStar_Clear();
        tft.fillRect(0,110,240,40,BACKGROUND);
        ofr_tft.setFontSize(36);
        ofr_tft.setCursor(26, 130); // Middle of box
        ofr_tft.setAlignment(Align::MiddleLeft);
        if (sending_error == false) {
          ofr_tft.setFontColor(GREENish, BACKGROUND);
          ofr_tft.printf("Completed !");
        } else {
          ofr_tft.setFontColor(PINKish, BACKGROUND);
          ofr_tft.printf("Failed !");          
        }

      }
      
    iti_number++;

    // delay(5);
    }
  }

  
}


void DrawTitle() {

  uint16_t Dom_Color;

  String Text;

  Dom_Color = PINKish;  // pinkish
  //tp.DotStar_SetPixelColor( 255, 22, 120 );
  Text = "RAW DATA";   

  tft.fillScreen(BACKGROUND);
  tft.fillRect(0,0,240,40, Dom_Color);
  tft.fillRect(0,40,240,4, TFT_WHITE);

//  tft.setTextColor(TFT_WHITE, Dom_Color);
//  tft.setTextDatum(MC_DATUM);
//  tft.setTextSize(3);
//  tft.drawString(Text,120,26,1);

  ofr_tft.setFontSize(64);
  ofr_tft.setCursor(120, 8); // Middle of box
  ofr_tft.setAlignment(Align::MiddleCenter);
  ofr_tft.setFontColor(TFT_WHITE, Dom_Color);
  ofr_tft.printf("RAW DATA"); 

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
