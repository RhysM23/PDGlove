#ifndef TINYPICO_LED_H
#define TINYPICO_LED_H

#include <TinyPICO.h>

// Create the TinyPICO object
TinyPICO tp = TinyPICO();

// LED animation state
int LED_progress = 0;

// Initialize TinyPICO LED
void initTinyPicoLED() {
  // Initialize LED
  tp.DotStar_Clear();
  LED_progress = 0;
  Serial.println("TinyPICO LED initialized");
}

// Update waiting animation (green pulsing)
void updateWaitingAnimation() {
  if (LED_progress <= 225) {
    tp.DotStar_SetPixelColor(0, 15+LED_progress, 0);
  } else {
    tp.DotStar_SetPixelColor(0, 465-LED_progress, 0);
  }
  LED_progress += 5;
  if (LED_progress == 455) LED_progress = 0;
}

// Update measuring animation (red blinking)
void updateMeasuringAnimation() {
    // Set a static red LED
    tp.DotStar_SetPixelColor(255, 0, 0);
  }

// Show command received indication (blue flash)
void showCommandReceivedIndication() {
  tp.DotStar_SetPixelColor(0, 0, 255);
  delay(200);
  tp.DotStar_Clear();
}

// Update sending animation (blue/red based on success)
void updateSendingAnimation(bool success) {
  int color = success ? 255 : 0;
  
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
}

// Yellow counting animation for countdown
void updateCountdownAnimation() {
  if (LED_progress <= 20) {
    tp.DotStar_SetPixelColor(15+LED_progress*12, 15+LED_progress*12, 0);
    LED_progress++;
  }
  if (LED_progress > 20) {
    LED_progress = 0;
  }
}

// Reset LED
void resetLED() {
  tp.DotStar_Clear();
  LED_progress = 0;
}

#endif // TINYPICO_LED_H