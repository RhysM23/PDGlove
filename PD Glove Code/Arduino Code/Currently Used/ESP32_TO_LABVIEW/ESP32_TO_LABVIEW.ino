#ifdef ESP32
#include "SPIFFS.h" // ESP32 only
#endif

#include "SPI.h"
#include <TFT_eSPI.h>
TFT_eSPI tft = TFT_eSPI();

#include <TFT_eWidget.h>

#include "OpenFontRender.h"

OpenFontRender ofr_heading;
OpenFontRender ofr_text;
OpenFontRender ofr_value;
OpenFontRender ofr_tft;

#include "NotoSans_Bold.h"
#define TTF_FONT NotoSans_Bold

TFT_eSprite heading = TFT_eSprite(&tft);
TFT_eSprite text = TFT_eSprite(&tft);
TFT_eSprite value = TFT_eSprite(&tft);

GraphWidget sensor1_graph = GraphWidget(&tft);
TraceWidget sensor1_trace = TraceWidget(&sensor1_graph);

GraphWidget sensor2_graph = GraphWidget(&tft);
TraceWidget sensor2_trace = TraceWidget(&sensor2_graph);

GraphWidget sensor3_graph = GraphWidget(&tft);
TraceWidget sensor3_trace = TraceWidget(&sensor3_graph);

#include <TinyPICO.h>
TinyPICO tp = TinyPICO();

#include "WiFi.h"
#include <esp_now.h>
uint8_t broadcastAddress[] = {0x64, 0xB7, 0x08, 0x90, 0x41, 0x7C};
esp_now_peer_info_t peerInfo;

int iti_number = 0;
int max_iti_number = 0;
int LED_progress = 0;

// UPDATED: Message structure with 5 values for stiffness support
typedef struct struct_message { 
    bool command;
    int mode;        // Measurement mode (1=Tremor, 2=Bradykinesia, 3=Stiffness)
    int index; 
    int max_index; 
    uint32_t time_ms; // Time in milliseconds
    float value1;     // Mode-specific value 1
    float value2;     // Mode-specific value 2
    float value3;     // Mode-specific value 3
    float value4;     // Mode-specific value 4 (for stiffness mode)
    float value5;     // Mode-specific value 5 (for stiffness mode)
} struct_message;

struct_message gloveData;

// UPDATED: Arrays for storing measurement data including 5 values
int Array_index[1300];
uint32_t Array_time_ms[1300];
int Array_mode[1300];     // Store the measurement mode
float Array_value1[1300]; // Mode-specific value 1
float Array_value2[1300]; // Mode-specific value 2
float Array_value3[1300]; // Mode-specific value 3
float Array_value4[1300]; // Mode-specific value 4 (for stiffness)
float Array_value5[1300]; // Mode-specific value 5 (for stiffness)


#define BACKGROUND   0x29AA
#define GREENish     0x47A9
#define BLUEish      0x2E5C
#define PINKish      0xF16F
#define YELLOWish    0xFEE9
#define WHITE        0xFFFF

char message[5];
unsigned int message_pos = 0;

const char *headingsNames[] = {
  "LabVIEW", "CONTACT", "ANGLE", "FORCE",
  "ACCELEROM."
};

uint16_t headingsColors[5] = {TFT_GREY, BLUEish, GREENish, YELLOWish, PINKish};

const int VERT_START = 26;
const int VERTICAL_STEP = 23;
int VERT_POS;

bool WAITING_FOR_COMMAND = true;
bool RECEIVING_GLOVE_DATA = false;
bool RECEIVED_ALL = false;
bool DRAW_GLOVE_DATA = false;
bool SEND_GLOVE_DATA = false;

int prevESPNOW_Progress = -1;

// UPDATED: Callback function to handle 5 values
void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  RECEIVING_GLOVE_DATA = true;
  if (RECEIVED_ALL == false) {
    memcpy(&gloveData, incomingData, sizeof(gloveData));
    
    Array_index[gloveData.index] = gloveData.index;
    max_iti_number = gloveData.max_index-1;
    Array_time_ms[gloveData.index] = gloveData.time_ms;
    Array_mode[gloveData.index] = gloveData.mode;
    Array_value1[gloveData.index] = gloveData.value1;
    Array_value2[gloveData.index] = gloveData.value2;
    Array_value3[gloveData.index] = gloveData.value3;
    Array_value4[gloveData.index] = gloveData.value4; // Store 4th value
    Array_value5[gloveData.index] = gloveData.value5; // Store 5th value

    if (gloveData.index == 0) {
      PRINT_TEXT("Receiving data...", BLUEish);
    }
    
    int Progress = round((gloveData.index)*100 / max_iti_number);  
    
    if (prevESPNOW_Progress != Progress) {
        char intvalue[7];
        itoa(Progress, intvalue, 10);
        strcat(intvalue, " %%");
        PRINT_VALUE(intvalue, BLUEish);
        prevESPNOW_Progress = Progress;
      }
    
    if (gloveData.index == max_iti_number) {
      prevESPNOW_Progress = -1;
      RECEIVED_ALL = true;
      RECEIVING_GLOVE_DATA = false;
      WAITING_FOR_COMMAND = true;
      DRAW_GLOVE_DATA = true;
      PRINT_TEXT("Drawing graphs...", WHITE);
    }
  }
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void setup() {
  Serial.begin(115200);

  if (!SPIFFS.begin()) {
    Serial.println("SPIFFS initialisation failed!");
    while (1) yield(); // Stay here twiddling thumbs waiting
  }
  Serial.println("\r\nInitialisation done.");

  tft.begin();
  tft.setTextColor(0xFFFF, 0x0000);
  tft.fillScreen(BACKGROUND);

  ofr_heading.setDrawer(heading);
  ofr_heading.loadFont(TTF_FONT, sizeof(TTF_FONT));
  ofr_heading.setAlignment(Align::TopLeft);
  ofr_heading.setFontSize(36);

  ofr_value.setDrawer(value);
  ofr_value.loadFont(TTF_FONT, sizeof(TTF_FONT));
  ofr_value.setAlignment(Align::TopLeft);
  ofr_value.setFontSize(36);
  
  ofr_text.setDrawer(text);
  ofr_text.loadFont(TTF_FONT, sizeof(TTF_FONT));
  ofr_text.setAlignment(Align::TopLeft);
  ofr_text.setFontSize(36);

  ofr_tft.setDrawer(tft);
  ofr_tft.loadFont(TTF_FONT, sizeof(TTF_FONT));

  void *out0 = heading.createSprite(240, 40);
  if (out0 == NULL) Serial.println("Could not allocate memory for heading sprite!");

  void *out1 = text.createSprite(240, VERTICAL_STEP);
  if (out1 == NULL) Serial.println("Could not allocate memory for value sprite!");

  void *out2 = value.createSprite(240, VERTICAL_STEP);
  if (out1 == NULL) Serial.println("Could not allocate memory for value sprite!");
  
  delay(500);

  drawBmp("/logoram.bmp", 0, 0);

  tft.fillScreen(BACKGROUND);
  PRINT_HEADING(0);

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
  Serial.println("Error initializing ESP-NOW");
  return;
  }
  esp_now_register_recv_cb(OnDataRecv);
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
      
  // UPDATED: Initialize all data arrays including the new ones
  for (int i=0; i<1300; i++) {
    Array_index[i] = 0;
    Array_time_ms[i] = 0;
    Array_mode[i] = 0;
    Array_value1[i] = 0;
    Array_value2[i] = 0;
    Array_value3[i] = 0;
    Array_value4[i] = 0; // Initialize 4th value array
    Array_value5[i] = 0; // Initialize 5th value array
  }
  
  VERT_POS = 2*VERTICAL_STEP - 3;
  PRINT_TEXT("WiFi activated", GREENish);    
  PRINT_TEXT("Serial activated", GREENish);     
  PRINT_TEXT("Waiting for LabVIEW...", WHITE);
}

void loop() {
if (WAITING_FOR_COMMAND == true) {
  if (Serial.available() > 0) {
    // read the incoming byte:
    char inByte = Serial.read();

    if (inByte != '\n' && (message_pos < 5))
    {
      message[message_pos] = inByte;
      message_pos++;
      tp.DotStar_SetPixelColor( 0, 0, 255 );
    } else
    {
      message[message_pos] = '\0';
      
      if (strcmp(message, "TREM") == 0) {
        PRINT_TEXT("-> Tremor", YELLOWish);
        gloveData.command = true;
        gloveData.mode = 1;
        gloveData.index = 1;
      }

      if (strcmp(message, "BRAD") == 0) {
        PRINT_TEXT("-> Bradykinesia", YELLOWish);
        gloveData.command = true;
        gloveData.mode = 2;
        gloveData.index = 2;
      }

      if (strcmp(message, "STIF") == 0) {
        PRINT_TEXT("-> Stiffness (5 sensors)", YELLOWish);
        gloveData.command = true;
        gloveData.mode = 3;
        gloveData.index = 3;
      }
      
      // Initialize all values
      gloveData.max_index = 0;
      gloveData.time_ms = 0;
      gloveData.value1 = 0;
      gloveData.value2 = 0;
      gloveData.value3 = 0;
      gloveData.value4 = 0; // Initialize 4th value
      gloveData.value5 = 0; // Initialize 5th value
      
      message_pos = 0;
      
      esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &gloveData, sizeof(gloveData));
      
      if (result == ESP_OK) {
        Serial.println("Sent Successfully");
        PRINT_TEXT("Command sent to glove", BLUEish);
        PRINT_TEXT("Waiting for glove data", WHITE);
        WAITING_FOR_COMMAND = false;
      }
      else {
        Serial.println("Error sending the data");
        PRINT_TEXT("Failed !", PINKish);
        PRINT_TEXT("Waiting for LabVIEW...", WHITE);
      }
    }
  } else {
      if (LED_progress <= 225) {
        tp.DotStar_SetPixelColor(0, 15+LED_progress, 0);
        } else {
        tp.DotStar_SetPixelColor(0, 465-LED_progress, 0);
        }
        LED_progress += 5;
        if (LED_progress >= 455) LED_progress = 0;    
  }
  delay(20);
}

if (RECEIVING_GLOVE_DATA == true && RECEIVED_ALL == false) {
    if (LED_progress == 0) {
        tp.DotStar_SetPixelColor(0, 0, 255);
      }
      if (LED_progress == 5) {
        tp.DotStar_Clear();
      }
      LED_progress++;
      if (LED_progress >= 10) {
        LED_progress = 0;
      }
  }
  delay(10);
  
if (RECEIVING_GLOVE_DATA == false && RECEIVED_ALL == true) {
  RECEIVED_ALL = false;
  }

if (DRAW_GLOVE_DATA == true) {
  delay(1000);
  tft.fillScreen(BACKGROUND);
  PRINT_HEADING(0); 

  tft.setTextColor(TFT_WHITE, BACKGROUND); 
  tft.setTextSize(1);
  
  // Get the measurement mode from the first data point
  int mode = Array_mode[0];
  
  // Set appropriate scales based on mode (display only shows first 3 values)
  switch(mode) {
    case 1: // Tremor (accelerometer)
      sensor1_graph.createGraph(200, 60, tft.color565(23, 37, 60));
      sensor1_graph.setGraphScale(0.0, 10.0, -2.0, 2.0); // -2g to 2g
      sensor1_graph.setGraphGrid(0.0, 1.0, -1.0, 0.5, tft.color565(73, 77, 109));
      sensor1_graph.drawGraph(30, 64);
      
      tft.setTextDatum(MR_DATUM); // Middle right text datum
      tft.drawFloat(2.0, 1, sensor1_graph.getPointX(0.0), sensor1_graph.getPointY(2.0));
      tft.drawFloat(0.0, 1, sensor1_graph.getPointX(0.0), sensor1_graph.getPointY(0.0));
      tft.drawFloat(-2.0, 1, sensor1_graph.getPointX(0.0), sensor1_graph.getPointY(-2.0));
      
      sensor2_graph.createGraph(200, 60, tft.color565(23, 37, 60));
      sensor2_graph.setGraphScale(0.0, 10.0, -2.0, 2.0);
      sensor2_graph.setGraphGrid(0.0, 1.0, -1.0, 0.5, tft.color565(73, 77, 109));
      sensor2_graph.drawGraph(30, 154);
      
      tft.setTextDatum(MR_DATUM);
      tft.drawFloat(2.0, 1, sensor2_graph.getPointX(0.0), sensor2_graph.getPointY(2.0));
      tft.drawFloat(0.0, 1, sensor2_graph.getPointX(0.0), sensor2_graph.getPointY(0.0));
      tft.drawFloat(-2.0, 1, sensor2_graph.getPointX(0.0), sensor2_graph.getPointY(-2.0));
      
      sensor3_graph.createGraph(200, 60, tft.color565(23, 37, 60));
      sensor3_graph.setGraphScale(0.0, 10.0, -2.0, 2.0);
      sensor3_graph.setGraphGrid(0.0, 1.0, -1.0, 0.5, tft.color565(73, 77, 109));
      sensor3_graph.drawGraph(30, 244);
      
      tft.setTextDatum(MR_DATUM);
      tft.drawFloat(2.0, 1, sensor3_graph.getPointX(0.0), sensor3_graph.getPointY(2.0));
      tft.drawFloat(0.0, 1, sensor3_graph.getPointX(0.0), sensor3_graph.getPointY(0.0));
      tft.drawFloat(-2.0, 1, sensor3_graph.getPointX(0.0), sensor3_graph.getPointY(-2.0));
      break;
      
    case 2: // Bradykinesia
      sensor1_graph.createGraph(200, 60, tft.color565(23, 37, 60));
      sensor1_graph.setGraphScale(0.0, 10.0, 0.0, 1.0); // Boolean contact (0-1)
      sensor1_graph.setGraphGrid(0.0, 1.0, 0.0, 0.5, tft.color565(73, 77, 109));
      sensor1_graph.drawGraph(30, 64);
      
      tft.setTextDatum(MR_DATUM);
      tft.drawFloat(1.0, 1, sensor1_graph.getPointX(0.0), sensor1_graph.getPointY(1.0));
      tft.drawFloat(0.5, 1, sensor1_graph.getPointX(0.0), sensor1_graph.getPointY(0.5));
      tft.drawFloat(0.0, 1, sensor1_graph.getPointX(0.0), sensor1_graph.getPointY(0.0));
      
      sensor2_graph.createGraph(200, 60, tft.color565(23, 37, 60));
      sensor2_graph.setGraphScale(0.0, 10.0, -180, 180); // Angle in degrees
      sensor2_graph.setGraphGrid(0.0, 1.0, -90, 90, tft.color565(73, 77, 109));
      sensor2_graph.drawGraph(30, 154);
      
      tft.setTextDatum(MR_DATUM);
      tft.drawNumber(180, sensor2_graph.getPointX(0.0), sensor2_graph.getPointY(180));
      tft.drawNumber(0, sensor2_graph.getPointX(0.0), sensor2_graph.getPointY(0));
      tft.drawNumber(-180, sensor2_graph.getPointX(0.0), sensor2_graph.getPointY(-180));
      
      sensor3_graph.createGraph(200, 60, tft.color565(23, 37, 60));
      sensor3_graph.setGraphScale(0.0, 10.0, -180, 180);
      sensor3_graph.setGraphGrid(0.0, 1.0, -90, 90, tft.color565(73, 77, 109));
      sensor3_graph.drawGraph(30, 244);
      
      tft.setTextDatum(MR_DATUM);
      tft.drawNumber(180, sensor3_graph.getPointX(0.0), sensor3_graph.getPointY(180));
      tft.drawNumber(0, sensor3_graph.getPointX(0.0), sensor3_graph.getPointY(0));
      tft.drawNumber(-180, sensor3_graph.getPointX(0.0), sensor3_graph.getPointY(-180));
      break;
      
    case 3: // Stiffness - Display force sensor 1, force sensor 2, and angle (values 1, 2, 3)
      sensor1_graph.createGraph(200, 60, tft.color565(23, 37, 60));
      sensor1_graph.setGraphScale(0.0, 10.0, 0.0, 1.0); // Force normalized 0-1
      sensor1_graph.setGraphGrid(0.0, 1.0, 0.0, 0.25, tft.color565(73, 77, 109));
      sensor1_graph.drawGraph(30, 64);
      
      tft.setTextDatum(MR_DATUM);
      tft.drawFloat(1.0, 1, sensor1_graph.getPointX(0.0), sensor1_graph.getPointY(1.0));
      tft.drawFloat(0.5, 1, sensor1_graph.getPointX(0.0), sensor1_graph.getPointY(0.5));
      tft.drawFloat(0.0, 1, sensor1_graph.getPointX(0.0), sensor1_graph.getPointY(0.0));
      
      sensor2_graph.createGraph(200, 60, tft.color565(23, 37, 60));
      sensor2_graph.setGraphScale(0.0, 10.0, 0.0, 1.0);
      sensor2_graph.setGraphGrid(0.0, 1.0, 0.0, 0.25, tft.color565(73, 77, 109));
      sensor2_graph.drawGraph(30, 154);
      
      tft.setTextDatum(MR_DATUM);
      tft.drawFloat(1.0, 1, sensor2_graph.getPointX(0.0), sensor2_graph.getPointY(1.0));
      tft.drawFloat(0.5, 1, sensor2_graph.getPointX(0.0), sensor2_graph.getPointY(0.5));
      tft.drawFloat(0.0, 1, sensor2_graph.getPointX(0.0), sensor2_graph.getPointY(0.0));
      
      sensor3_graph.createGraph(200, 60, tft.color565(23, 37, 60));
      sensor3_graph.setGraphScale(0.0, 10.0, -180, 180); // Angle in degrees
      sensor3_graph.setGraphGrid(0.0, 1.0, -90, 90, tft.color565(73, 77, 109));
      sensor3_graph.drawGraph(30, 244);
      
      tft.setTextDatum(MR_DATUM);
      tft.drawNumber(180, sensor3_graph.getPointX(0.0), sensor3_graph.getPointY(180));
      tft.drawNumber(0, sensor3_graph.getPointX(0.0), sensor3_graph.getPointY(0));
      tft.drawNumber(-180, sensor3_graph.getPointX(0.0), sensor3_graph.getPointY(-180));
      break;
      
    default: // Default case with original scales
      sensor1_graph.createGraph(200, 60, tft.color565(23, 37, 60));
      sensor1_graph.setGraphScale(0.0, 10.0, 0, 4095);
      sensor1_graph.setGraphGrid(0.0, 1.0, 0.0, 1000.0, tft.color565(73, 77, 109));
      sensor1_graph.drawGraph(30, 64);
      
      tft.setTextDatum(MR_DATUM);
      tft.drawNumber(4095, sensor1_graph.getPointX(0.0), sensor1_graph.getPointY(4095.0));
      tft.drawNumber(2048, sensor1_graph.getPointX(0.0), sensor1_graph.getPointY(2048.0));
      tft.drawNumber(0, sensor1_graph.getPointX(0.0), sensor1_graph.getPointY(0.0));

      sensor2_graph.createGraph(200, 60, tft.color565(23, 37, 60));
      sensor2_graph.setGraphScale(0.0, 10.0, 0, 4095);
      sensor2_graph.setGraphGrid(0.0, 1.0, 0.0, 1000.0, tft.color565(73, 77, 109));
      sensor2_graph.drawGraph(30, 154);
      
      tft.setTextDatum(MR_DATUM);
      tft.drawNumber(4095, sensor2_graph.getPointX(0.0), sensor2_graph.getPointY(4095.0));
      tft.drawNumber(2048, sensor2_graph.getPointX(0.0), sensor2_graph.getPointY(2048.0));
      tft.drawNumber(0, sensor2_graph.getPointX(0.0), sensor2_graph.getPointY(0.0));
      
      sensor3_graph.createGraph(200, 60, tft.color565(23, 37, 60));
      sensor3_graph.setGraphScale(0.0, 10.0, 0, 4095);
      sensor3_graph.setGraphGrid(0.0, 1.0, 0.0, 1000.0, tft.color565(73, 77, 109));
      sensor3_graph.drawGraph(30, 244);
      
      tft.setTextDatum(MR_DATUM);
      tft.drawNumber(4095, sensor3_graph.getPointX(0.0), sensor3_graph.getPointY(4095.0));
      tft.drawNumber(2048, sensor3_graph.getPointX(0.0), sensor3_graph.getPointY(2048.0));
      tft.drawNumber(0, sensor3_graph.getPointX(0.0), sensor3_graph.getPointY(0.0));
  }

  // X-axis time labels (common for all modes)
  tft.setTextDatum(TC_DATUM); // Top center text datum
  tft.drawNumber(0, sensor3_graph.getPointX(0.0), sensor3_graph.getPointY(-5.0) + 3);
  tft.drawNumber(5, sensor3_graph.getPointX(5.0), sensor3_graph.getPointY(-5.0) + 3);
  tft.drawNumber(10, sensor3_graph.getPointX(10.0), sensor3_graph.getPointY(-5.0) + 3);

  sensor1_trace.startTrace(GREENish);
  sensor2_trace.startTrace(BLUEish);
  sensor3_trace.startTrace(PINKish);  

  for (int i=0; i<max_iti_number; i++) {
    float time_s = float(Array_time_ms[i])/1000.00;
    sensor1_trace.addPoint(time_s, Array_value1[i]);
    sensor2_trace.addPoint(time_s, Array_value2[i]);
    sensor3_trace.addPoint(time_s, Array_value3[i]);    
  }

  DRAW_GLOVE_DATA = false;
  delay(3000);
  tft.fillScreen(BACKGROUND);
  PRINT_HEADING(0);
  VERT_POS = 2*VERTICAL_STEP - 3;      
  PRINT_TEXT("Sending to PC...", BLUEish); 

  SEND_GLOVE_DATA = true;
  }

  // UPDATED: Send data to LabVIEW with support for 5 values
  if (SEND_GLOVE_DATA == true) {
    for (int i=0; i<max_iti_number; i++) {
      int Progress = round((i+1)*100 / max_iti_number);  
    
      if (prevESPNOW_Progress != Progress) {
        char intvalue[7];
        itoa(Progress, intvalue, 10);
        strcat(intvalue, " %%");
        PRINT_VALUE(intvalue, BLUEish);
        prevESPNOW_Progress = Progress;
      }
      
      char buf[128]; // Increased buffer size for 5 float values
      int mode = Array_mode[i];
      
      // Format data string for LabVIEW based on mode
      if (mode == 3) {
        // Stiffness mode: send all 5 values
        sprintf(buf, "DATA%dx%dx%dx%.3fx%.3fx%.3fx%.3fx%.3f", 
                Array_index[i], 
                max_iti_number, 
                Array_time_ms[i], 
                Array_value1[i], 
                Array_value2[i], 
                Array_value3[i],
                Array_value4[i], 
                Array_value5[i]);
      } else {
        // Tremor and Bradykinesia modes: send 3 values for backward compatibility
        sprintf(buf, "DATA%dx%dx%dx%.3fx%.3fx%.3f", 
                Array_index[i], 
                max_iti_number, 
                Array_time_ms[i], 
                Array_value1[i], 
                Array_value2[i], 
                Array_value3[i]);
      }
              
      Serial.println(buf);
    }
    SEND_GLOVE_DATA = false;
    prevESPNOW_Progress = -1;
    WAITING_FOR_COMMAND = true;
    PRINT_TEXT("Data sent !", GREENish);
    PRINT_TEXT("Waiting for LabVIEW...", WHITE);
   }
}

void PRINT_HEADING(int Screen) {

  uint16_t Dom_Color;

  Dom_Color = headingsColors[Screen];

  heading.fillSprite(Dom_Color);
  heading.fillRect(0, 36, 240, 4, TFT_WHITE);

  ofr_heading.setFontSize(64);
  ofr_heading.setCursor(120, 8);
  ofr_heading.setAlignment(Align::MiddleCenter);
  ofr_heading.setFontColor(TFT_WHITE, Dom_Color);
  ofr_heading.printf(headingsNames[Screen]);

  heading.pushSprite(0, 0);
}

void PRINT_TEXT(const char *TextToPrint, uint16_t textColor) {
  text.fillSprite(BACKGROUND);
  ofr_text.setCursor(0, -1); 
  ofr_text.setFontColor(textColor, BACKGROUND);
  ofr_text.printf(TextToPrint);  
  text.pushSprite(6,VERT_POS);
  VERT_POS = VERT_POS + VERTICAL_STEP;       
}

void PRINT_VALUE(const char *ValueToPrint, uint16_t valueColor) {
  value.fillSprite(BACKGROUND);
  ofr_value.setCursor(0, -1); 
  ofr_value.setFontColor(valueColor, BACKGROUND);
  ofr_value.printf(ValueToPrint);  
  value.pushSprite(170,VERT_POS-VERTICAL_STEP);  
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
