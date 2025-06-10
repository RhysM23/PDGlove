#include "imu_handler.h"
#include "esp_now_comm.h"
#include "data_storage.h"
#include "sensors.h"
#include "tinypico_led.h"
#include "sampling.h"

// State variables
bool WAITING_FOR_COMMAND = true;
bool MEASURE = false;
bool SEND_GLOVE_DATA = false;

// Timing variables
uint32_t t_start = 0;
int prevESPNOW_Progress = -1;

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  Serial.println("\r\nSystem starting...");
  
  // Initialize subsystems
  initTinyPicoLED();
  initSensors();
  
  // Initialize IMU sensors
  bool imuInitialized = initIMUSensors();
  if (!imuInitialized) {
    Serial.println("WARNING: IMU initialization incomplete");
  }
  
  // Initialize ESP-NOW
  bool espNowInitialized = initESPNow();
  if (!espNowInitialized) {
    Serial.println("WARNING: ESP-NOW initialization failed");
  }
  
  // Initialize data storage
  initDataStorage();
  
  // Initialize fixed-rate sampling system
  bool samplingInitialized = initSampling();
  if (!samplingInitialized) {
    Serial.println("WARNING: Sampling system initialization failed");
  }
  
  // Set initial timing
  t_start = millis() - 10001;  // Ensure we're ready for a measurement right away
  
  Serial.println("System initialization complete");
  Serial.println("Waiting for command...");
}

void loop() {
  // Always read IMU data when available to keep it updated
  readIMUData();
  
  // State: Waiting for Command
  if (WAITING_FOR_COMMAND) {
    if (RECEIVED_COMMAND) {
      // Command received, process it
      showCommandReceivedIndication();
      
      measurementMode = gloveData.index;  // Using index as mode (1=Tremor, 2=Bradykinesia, 3=Stiffness)
  
      // Display the test type based on the mode
      if (measurementMode == 1) {
        Serial.println("Test: Tremor");
      } else if (measurementMode == 2) {
        Serial.println("Test: Bradykinesia");
      } else if (measurementMode == 3) {
        Serial.println("Test: Stiffness");
      }

      delay(500);
      
      // Transition to measurement state
      WAITING_FOR_COMMAND = false;
      RECEIVED_COMMAND = false;
      
      // Reset counters
      iti_number = 0;
      max_iti_number = 0;
      
      // Turn off WiFi during measurement
      setWiFiMode(false);
      delay(500);
      
      Serial.println("Starting measurement in 3 seconds...");
      
      // Countdown animation
      LED_progress = 0;
      for (int i = 0; i < 61; i++) {
        Serial.print(".");
        if (i % 20 == 0) Serial.println();
        updateCountdownAnimation();
        delay(50);
      }
      resetLED();
      
      // Start the fixed-rate sampling
      startSampling(gloveData.index);
      
      // Record measurement start time
      t_start = millis();
      
      // Set state to measuring

      
      MEASURE = true;
      updateMeasuringAnimation();

      Serial.println("\nMeasurement started with fixed sampling rate!");

    } else {
      // No command received, show waiting animation
      updateWaitingAnimation();
      delay(20);
    }
  }
  
  // State: Measuring
  else if (MEASURE) {
    
        
    // Check if measurement duration has elapsed
    if (isMeasurementComplete(t_start)) {
      // Stop the sampling timer
      stopSampling();
      MEASURE = false;
      
      // Calculate and output frequency
      int actualSamplingRate = calculateFrequency();
      Serial.print("Measurement complete. Actual sampling rate: ");
      Serial.print(actualSamplingRate);
      Serial.print(" Hz (Target: ");
      Serial.print(SAMPLING_RATE_HZ);
      Serial.println(" Hz)");
      
      // Turn WiFi back on for data transmission
      setWiFiMode(true);
      delay(500);
      
      Serial.println("Sending data....");
      delay(100);
      
      // Reset for data sending
      iti_number = 0;
      SEND_GLOVE_DATA = true;
    }
  }
  
    // State: Sending Data
  else if (SEND_GLOVE_DATA) {
    if (iti_number < max_iti_number) {
      // Prepare data packet with our new structure members
      gloveData.index = Array_index[iti_number];
      gloveData.max_index = max_iti_number;
      gloveData.time_ms = Array_time_ms[iti_number];  // Changed from time_x_ms
      gloveData.mode = Array_mode[iti_number];        // Add mode information
      gloveData.value1 = Array_value1[iti_number];    // Changed from sensor1
      gloveData.value2 = Array_value2[iti_number];    // Changed from sensor2
      gloveData.value3 = Array_value3[iti_number];    // Changed from sensor3
      
      // Send the packet
      bool sendSuccess = sendESPNowData();
      
      // Calculate and display progress
      int Progress = round((iti_number + 1) * 100 / max_iti_number);
      if (prevESPNOW_Progress != Progress) {
        Serial.print("Progress: ");
        Serial.print(Progress);
        Serial.println("%");
        prevESPNOW_Progress = Progress;
      }
      
      // Update the sending animation
      updateSendingAnimation(sendSuccess);
      
      // Check if sending is complete
      if (iti_number == max_iti_number - 1) {
        resetLED();
        
        if (!sending_error) {
          Serial.println("Data transmission completed successfully!");
        } else {
          Serial.println("Data transmission failed!");
        }
        
        // Reset for next command
        SEND_GLOVE_DATA = false;
        prevESPNOW_Progress = -1;
        iti_number = -1;
        WAITING_FOR_COMMAND = true;
        RECEIVED_COMMAND = false;
        LED_progress = 0;
        
        Serial.println("Waiting for next command...");
      }
      
      iti_number++;
      delay(5);
    }
  }
}