#ifndef DATA_STORAGE_H
#define DATA_STORAGE_H

// Data structure for communication
typedef struct struct_message {
    bool command;
    int mode;        // Measurement mode (1=Tremor, 2=Bradykinesia, 3=Stiffness)
    int index;
    int max_index;
    uint32_t time_ms;
    float value1;    // Meaning depends on mode
    float value2;    // Meaning depends on mode
    float value3;    // Meaning depends on mode
} struct_message;

// Global data object
struct_message gloveData;

// Arrays for storing measurement data
const int MAX_DATA_POINTS = 1300;
int Array_index[MAX_DATA_POINTS];
uint32_t Array_time_ms[MAX_DATA_POINTS];
int Array_mode[MAX_DATA_POINTS];
float Array_value1[MAX_DATA_POINTS];
float Array_value2[MAX_DATA_POINTS];
float Array_value3[MAX_DATA_POINTS];

// Counter variables
int iti_number = 0;        // Current iteration number
int max_iti_number = 0;    // Maximum iteration number

// Initialize all data storage arrays
void initDataStorage() {
  // Reset all data arrays
  for (int i = 0; i < MAX_DATA_POINTS; i++) {
    Array_index[i] = 0;
    Array_time_ms[i] = 0;
    Array_mode[i] = 0;
    Array_value1[i] = 0;
    Array_value2[i] = 0;
    Array_value3[i] = 0;    
  }
  
  // Reset counters
  iti_number = 0;
  max_iti_number = 0;
  
  Serial.println("Data storage initialized");
}

// Store a new data point
void storeDataPoint(uint32_t time_ms, int mode, float value1, float value2, float value3) {
  if (iti_number < MAX_DATA_POINTS) {
    Array_index[iti_number] = iti_number;
    Array_time_ms[iti_number] = time_ms;
    Array_mode[iti_number] = mode;
    Array_value1[iti_number] = value1;
    Array_value2[iti_number] = value2;
    Array_value3[iti_number] = value3;
    
    max_iti_number = iti_number;
    iti_number++;
  } else {
    Serial.println("Warning: Maximum data points reached!");
  }
}

// Calculate the measurement frequency in Hz
int calculateFrequency() {
  if (max_iti_number > 0) {
    // Calculate frequency based on sample count and timespan (10 seconds)
    return (max_iti_number) / 10;
  }
  return 0;
}

// Reset the data storage for a new measurement
void resetDataStorage() {
  iti_number = 0;
}

#endif // DATA_STORAGE_H