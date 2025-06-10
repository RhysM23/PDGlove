#ifndef SAMPLING_H
#define SAMPLING_H

#include "data_storage.h"
#include "sensors.h"
#include "esp_timer.h"

// Sampling configuration
const int SAMPLING_RATE_HZ = 100;  // 100 Hz sampling rate
const int SAMPLING_PERIOD_US = 1000000 / SAMPLING_RATE_HZ;  // Period in microseconds
const int MEASUREMENT_DURATION_MS = 10500;  // 10 second measurement

// Variables for sampling
volatile uint32_t measurementStartMicros = 0;  // Absolute start time
esp_timer_handle_t samplingTimer;

// Measurement mode from the received command
volatile int measurementMode = 0;

// Function to be called by the timer - perform sampling directly in the interrupt
void IRAM_ATTR onSampleTimer(void* arg) {
  // Get current time
  uint32_t now_micros = micros();
  uint32_t elapsed_ms = (now_micros - measurementStartMicros) / 1000;
  
  // Read raw sensor values - now supporting 5 values
  float value1, value2, value3, value4, value5;
  readSensors(measurementMode, value1, value2, value3, value4, value5);
  
  // Store the values directly
  storeDataPoint(elapsed_ms, measurementMode, value1, value2, value3, value4, value5);
}

// Initialize the sampling timer
bool initSampling() {
    esp_timer_create_args_t timerConfig = {};
    timerConfig.callback = &onSampleTimer;
    timerConfig.name = "sampling_timer";
    
    esp_err_t result = esp_timer_create(&timerConfig, &samplingTimer);
    if (result != ESP_OK) {
        Serial.println("Failed to create sampling timer");
        return false;
    }
    
    Serial.print("Sampling initialized at ");
    Serial.print(SAMPLING_RATE_HZ);
    Serial.println(" Hz");
    return true;
}

// Start sampling with the specified measurement mode
void startSampling(int mode) {
    // Store the measurement mode
    measurementMode = mode;
    
    // Reset data storage
    resetDataStorage();
    
    // Record the absolute start time
    measurementStartMicros = micros();
    
    // Start the timer with the specified period
    esp_timer_start_periodic(samplingTimer, SAMPLING_PERIOD_US);
    
    Serial.print("Sampling started at ");
    Serial.print(SAMPLING_RATE_HZ);
    Serial.println(" Hz");
}

// Stop sampling
void stopSampling() {
    esp_timer_stop(samplingTimer);
    Serial.println("Sampling stopped");
}

// Check if measurement duration has elapsed
bool isMeasurementComplete(uint32_t startTime) {
    return (millis() - startTime) >= MEASUREMENT_DURATION_MS;
}

#endif // SAMPLING_H