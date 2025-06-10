#ifdef ESP32
  #include "SPIFFS.h" // ESP32 only
#endif

#include "SPI.h"
#include <TFT_eSPI.h>   // Hardware-specific library
TFT_eSPI tft = TFT_eSPI();         // Invoke custom library

#include <TFT_eWidget.h>    

#include "OpenFontRender.h"
OpenFontRender ofr_attribute;
OpenFontRender ofr_value;
OpenFontRender ofr_tft;

#include "NotoSans_Bold.h"
#define TTF_FONT NotoSans_Bold

TFT_eSprite attribute = TFT_eSprite(&tft);
TFT_eSprite value = TFT_eSprite(&tft);

#include <TinyPICO.h>
TinyPICO tp = TinyPICO();

const int contactPin = 14;
const int anglePin = 27;
const int anglePin2 = 15;
const int accelPin = 4;
const int accelPin2z = 32;
const int accelPin2x = 33;
const int forcePin1 = 25;
const int forcePin2 = 26;
const int GlovePin = 22;

#define BACKGROUND   0x29AA
#define GREENish     0x47A9
#define BLUEish      0x2E5C
#define PINKish      0xF16F
#define YELLOWish    0xFEE9

#define BAT_CHARGE 34
#define BAT_VOLTAGE 35

const float LIPO_MAX = 4.20; //maximum voltage of battery
const float LIPO_MIN = 3.0;

bool USB_CONNECTED = false;
bool CHARGING = false;
bool ENOUGH_POWER = true;
bool GLOVE_ACTIVATED = false;

int VERT_POS;

uint32_t TIME = 0;
int WAIT_MS = 20;
bool BREAK_WAIT = false;
bool REDRAW_INTERFACE = false;

bool CURRENT_STATE[] = {false, true, false, false};
bool PREVIOUS_STATE[] = {false, true, false, true};

float LIPO_VOLTAGE;
int LIPO_CHARGE;

char *sensorsNames[] = {
                     "- Contact:", "- Angle 1:", "- Angle 2:",
                     "- Force 1:", "- Force 2:", "- Acceler. 1:",
                     "- Acceler. 2:", "- Acceler. 3:"
                    };
                    
uint16_t sensorsColors[8] = {BLUEish, GREENish, GREENish, YELLOWish, YELLOWish, PINKish, PINKish, PINKish}; 

void setup()
{
  Serial.begin(115200);

  if (!SPIFFS.begin()) {
    Serial.println("SPIFFS. initialisation failed!");
    while (1) yield();
  }
  Serial.println("\r\nInitialisation done.");

  tft.begin();
  tft.setTextColor(0xFFFF, 0x0000);
  tft.fillScreen(BACKGROUND);

  ofr_attribute.setDrawer(attribute);
  ofr_attribute.loadFont(TTF_FONT, sizeof(TTF_FONT));
  ofr_attribute.setAlignment(Align::TopLeft);
  ofr_attribute.setFontSize(36);
  
  ofr_value.setDrawer(value);
  ofr_value.loadFont(TTF_FONT, sizeof(TTF_FONT));
  ofr_value.setAlignment(Align::TopLeft);
  ofr_value.setFontSize(36);  
    
  ofr_tft.setDrawer(tft);
  ofr_tft.loadFont(TTF_FONT, sizeof(TTF_FONT));  
  
  delay(500);

  void *out1 = attribute.createSprite
  (164, 24);
    if (out1 == NULL) Serial.println("Could not allocate memory for sprite!");

  void *out2 = value.createSprite(70, 24);
    if (out2 == NULL) Serial.println("Could not allocate memory for sprite!");

  uint32_t t = millis();

  drawBmp("/logoram.bmp", 0, 0);
  
  t = millis() - t;
  Serial.print(t); Serial.println(" ms");

  for (int i = 0; i<51; i++)
    {
      tp.DotStar_SetPixelColor( i*5, 0, 0 );
      delay(10);
    }
  for (int i = 0; i<51; i++)
    {
      tp.DotStar_SetPixelColor( 250-i*5, 0, i*5 );
      delay(10);
    }
  for (int i = 0; i<51; i++)
    {
      tp.DotStar_SetPixelColor( 0, i*5, 250-i*5 );
      delay(10);
    }
  for (int i = 0; i<51; i++)
    {
      tp.DotStar_SetPixelColor( 0, 250-i*5, 0 );
      delay(10);
    }
    
  pinMode(GlovePin, INPUT); 
  
  delay(100);
  tft.fillScreen(BACKGROUND);
  DrawTitle();
}

void loop()
{
  USB_CONNECTED = digitalRead(9);
  CHARGING = tp.IsChargingBattery();
  
  if (USB_CONNECTED == false) {
    LIPO_VOLTAGE = tp.GetBatteryVoltage();
    LIPO_CHARGE = ((LIPO_VOLTAGE - LIPO_MIN) / (LIPO_MAX - LIPO_MIN)) * 100; 
    if (LIPO_CHARGE >= 40) {
      ENOUGH_POWER = true; 
    } else {
      ENOUGH_POWER = false;     
    }
  } else {
    ENOUGH_POWER = true;
    tp.DotStar_SetPixelColor(0, 0, 255);
  }

  if (ENOUGH_POWER = true) {
    if (digitalRead(GlovePin) == false) {
      GLOVE_ACTIVATED = false;
    } else {
      GLOVE_ACTIVATED = true;
    }
  } else {
    GLOVE_ACTIVATED = false;
  }

  CURRENT_STATE[0] = USB_CONNECTED;
  CURRENT_STATE[1] = CHARGING;
  CURRENT_STATE[2] = ENOUGH_POWER;
  CURRENT_STATE[3] = GLOVE_ACTIVATED;

  if ((CURRENT_STATE[0] != PREVIOUS_STATE[0]) || (CURRENT_STATE[1] != PREVIOUS_STATE[1]) || (CURRENT_STATE[2] != PREVIOUS_STATE[2]) || (CURRENT_STATE[3] != PREVIOUS_STATE[3])) {
    BREAK_WAIT = true;
    WAIT_MS = 3000;
    REDRAW_INTERFACE = true;
    PREVIOUS_STATE[0] = CURRENT_STATE[0];
    PREVIOUS_STATE[1] = CURRENT_STATE[1];
    PREVIOUS_STATE[2] = CURRENT_STATE[2];
    PREVIOUS_STATE[3] = CURRENT_STATE[3];
  }
  
  if (millis() - TIME > WAIT_MS || BREAK_WAIT == true) {
    BREAK_WAIT = false;
    TIME = millis();
    
    if (REDRAW_INTERFACE == true) {
      Redraw_Interface();
      REDRAW_INTERFACE = false;
    }
    
    if (USB_CONNECTED == false && WAIT_MS == 3000) {
      Update_Battery_Data();
      Battery_to_LED(LIPO_CHARGE);
    }
    
    if (GLOVE_ACTIVATED == true) {
      WAIT_MS = 20;
      Update_Sensor_Data();
    } else {
      WAIT_MS = 3000;
    }
  }
}

void Redraw_Interface() {  
  VERT_POS = 50;
  PRINT_ATTRIBUTE("USB connected:", TFT_WHITE);

  if (USB_CONNECTED == true) {
    PRINT_VALUE("YES", GREENish);
  } else {
    PRINT_VALUE("NO", PINKish);
  }
  
  if (USB_CONNECTED == true) {
    VERT_POS = VERT_POS + 24;
    PRINT_ATTRIBUTE("Charging battery:", TFT_WHITE);
    
    if (CHARGING == true) {
      PRINT_VALUE("YES", GREENish);
    } else {
      PRINT_VALUE("NO", PINKish);
    }
  } else {
    VERT_POS = VERT_POS + 24;
    PRINT_ATTRIBUTE("Battery voltage:", TFT_WHITE);
            
    VERT_POS = VERT_POS + 24;
    PRINT_ATTRIBUTE("Battery charge:", TFT_WHITE); 
  }
  
    VERT_POS = VERT_POS + 24;
    PRINT_ATTRIBUTE("Glove status:", TFT_WHITE);  
     
  if (GLOVE_ACTIVATED == true) {
    PRINT_VALUE("ON", GREENish);
    
    for (int i=0; i<8; i++) {
      VERT_POS = VERT_POS + 24;
      PRINT_ATTRIBUTE(sensorsNames[i],sensorsColors[i]);
    }
  } else {
    PRINT_VALUE("OFF", PINKish);
  }

  attribute.fillSprite(BACKGROUND);
  value.fillSprite(BACKGROUND);
  VERT_POS = VERT_POS + 24;
  for (int i=VERT_POS; i<=320; i+=20) {
    attribute.pushSprite(6,i);  
    value.pushSprite(170,i);  
  }
}

void PRINT_ATTRIBUTE(char *sensorName, uint16_t sensorColor) {
  attribute.fillSprite(BACKGROUND);
  ofr_attribute.setCursor(0, 0); 
  ofr_attribute.setFontColor(sensorColor, BACKGROUND);
  ofr_attribute.printf(sensorName);  
  attribute.pushSprite(6,VERT_POS);     
}

void PRINT_VALUE(char *valueName, uint16_t valueColor) {
  value.fillSprite(BACKGROUND);
  ofr_value.setCursor(0, 0); 
  ofr_value.setFontColor(valueColor, BACKGROUND);
  ofr_value.printf(valueName);  
  value.pushSprite(170,VERT_POS);     
}
      
void Update_Battery_Data() {

  int first_part;
  int second_part;
  int buffer_part;
  char* extra_zero = "0";
  char* str = "";
  
  VERT_POS = 50 + 24;

  
  char voltageText[8];
  dtostrf(LIPO_VOLTAGE, 2, 2, voltageText);
  strcat(voltageText, " V");
  PRINT_VALUE(voltageText, YELLOWish);
          
  VERT_POS = VERT_POS + 24;
  
  value.fillSprite(BACKGROUND);
  ofr_value.setCursor(0, 0);
  ofr_value.printf("%d %%", LIPO_CHARGE);
  value.pushSprite(170,VERT_POS);  
}

void Update_Sensor_Data() {
  if (USB_CONNECTED == true) {
    VERT_POS = 122;
  } else {
    VERT_POS = 146;
  }

  int contactValue = digitalRead(contactPin);   
  value.fillSprite(BACKGROUND);
  ofr_value.setCursor(0, 0);
  ofr_value.setFontColor(TFT_WHITE, BACKGROUND); 
  ofr_value.printf("%d", contactValue); 
  value.pushSprite(170,VERT_POS);

  VERT_POS = VERT_POS + 24;
  
  value.fillSprite(BACKGROUND);
  ofr_value.setCursor(0, 0);
  ofr_value.printf("%d deg", ANGLE_FINGER(1));
  value.pushSprite(170,VERT_POS);

  VERT_POS = VERT_POS + 24;

  value.fillSprite(BACKGROUND);
  ofr_value.setCursor(0, 0);
  ofr_value.printf("%d deg", ANGLE_FINGER(2));
  value.pushSprite(170,VERT_POS);

  VERT_POS = VERT_POS + 24;
  char forceText[8];
  float forceValueFloat = FORCE(1);
  dtostrf(forceValueFloat, 2, 2, forceText);
  strcat(forceText, " N");
  PRINT_VALUE(forceText, TFT_WHITE);
  
  value.fillSprite(BACKGROUND);
  ofr_value.setCursor(0, 0);
  
  //ofr_value.printf("xx.x N");
  //value.pushSprite(170,VERT_POS);

  VERT_POS = VERT_POS + 24;

  value.fillSprite(BACKGROUND);
  ofr_value.setCursor(0, 0);
  
  forceValueFloat = FORCE(2);
  dtostrf(forceValueFloat, 2, 2, forceText);
  strcat(forceText, " N");
  PRINT_VALUE(forceText, TFT_WHITE);
  
  //ofr_value.printf("xx.x N");
  //value.pushSprite(170,VERT_POS);

  VERT_POS = VERT_POS + 24;
  float accelValueFloat = ACCELEROMETER_1(1);
  /*
  int first_part = abs(accelValueFloat);
  float buffer_part = (abs(accelValueFloat) - first_part)*100.00;
  int second_part = buffer_part;
  char* extra_zero = "0";
  if (second_part >= 10) extra_zero="";
  char* str = "";
  if (accelValueFloat<0) str = "-";
  value.fillSprite(BACKGROUND);
  ofr_value.setCursor(0, 0);
  ofr_value.printf("%s%d.%s%d g", str, first_part, extra_zero, second_part);
  value.pushSprite(170,VERT_POS);
  */
  char accelText[8];
  dtostrf(accelValueFloat, 2, 2, accelText);
  strcat(accelText, " g");
  PRINT_VALUE(accelText, TFT_WHITE);


  VERT_POS = VERT_POS + 24;
  accelValueFloat = ACCELEROMETER_1(2);
  
  value.fillSprite(BACKGROUND);
  ofr_value.setCursor(0, 0);

  dtostrf(accelValueFloat, 2, 2, accelText);
  strcat(accelText, " g");
  PRINT_VALUE(accelText, TFT_WHITE);
  value.pushSprite(170,VERT_POS);

  VERT_POS = VERT_POS + 24;
  accelValueFloat = ACCELEROMETER_1(3);
  
  value.fillSprite(BACKGROUND);
  ofr_value.setCursor(0, 0);

  dtostrf(accelValueFloat, 2, 2, accelText);
  strcat(accelText, " g");
  PRINT_VALUE(accelText, TFT_WHITE);
  //value.pushSprite(170,VERT_POS); 
}

void Battery_to_LED(int charge) {
  int r = 0;
  int g = 0;
  int b = 0;
  int i;
  switch (charge) {
    case 0 ... 40 : {
      i = 40 - charge;
      r = 30-i*3/2;
      }
      break;
    case 41 ... 60 : {
      i = 60 - charge;
      r = 60-i*3/2;
      g = 0;
      }
      break;
    case 61 ... 80 : {
      i = 80 - charge;
      r = 60;
      g = 60 - i*3;
      }
      break;  
    case 81 ... 100 : {
      i = 100 - charge;
      r=i*3;
      g = 60;
      }
      break; 
  }
  tp.DotStar_SetPixelColor(r, g, b);
}

int ANGLE_FINGER(int number) {
  int angleValueRaw;
  if (number ==1) {
    angleValueRaw = analogRead(anglePin);
  } else {
    angleValueRaw = analogRead(anglePin2);
  }
  
  int angleValue = int(double(1870-angleValueRaw)/7.22); 
  return angleValue;
}

float ACCELEROMETER_1(int number) {
  int accelValueRaw;

  if (number == 1) {
    accelValueRaw = analogRead(accelPin);
  }
  if (number == 2) {
    accelValueRaw = analogRead(accelPin2z);
  }
  if (number == 3) {
    accelValueRaw = analogRead(accelPin2x);
  }
    
  int accelValue = map(accelValueRaw, 0, 4095, 0, 3300);
  float accelValueFloat = (accelValue-1650)/330.0;
  return accelValueFloat;
}

float FORCE(int number) {
   int forceValueRaw;
   
  if (number == 1) {
    forceValueRaw = analogRead(forcePin1);
  }
  if (number == 2) {
    forceValueRaw = analogRead(forcePin2);
  }
  
  int forceValue = map(forceValueRaw, 0, 4095, 0, 1000);
  float forceValueFloat = (forceValue)/100.0;
  return forceValueFloat;
}

void DrawTitle() {

  uint16_t Dom_Color;

  Dom_Color = TFT_GREY;  // pinkish

  tft.fillScreen(BACKGROUND);
  tft.fillRect(0,0,240,40, Dom_Color);
  tft.fillRect(0,40,240,4, TFT_WHITE);

  ofr_tft.setFontSize(64);
  ofr_tft.setCursor(120, 8); // Middle of box
  ofr_tft.setAlignment(Align::MiddleCenter);
  ofr_tft.setFontColor(TFT_WHITE, Dom_Color);
  ofr_tft.printf("GLOVE STATUS"); 

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
