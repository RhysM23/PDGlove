#ifndef SENSORS_H
#define SENSORS_H

// Define sensor pins - updated with additional force sensors
const int contactPin = 14;  // Pin for contact sensor
const int forcePin1 = 25;   // Pin for force sensor 1
const int forcePin2 = 26;   // Pin for force sensor 2
const int forcePin3 = 32;   // Pin for force sensor 3 (new)
const int forcePin4 = 33;   // Pin for force sensor 4 (new)

// Initialize the physical sensors
void initSensors() {
  // Set up pins
  pinMode(contactPin, INPUT);
  pinMode(forcePin1, INPUT);
  pinMode(forcePin2, INPUT);
  pinMode(forcePin3, INPUT);  // New force sensor
  pinMode(forcePin4, INPUT);  // New force sensor
  
  Serial.println("Sensor pins initialized (including new force sensors on pins 32 & 33)");
}

// Read all sensors based on the selected mode and return raw values in meaningful units
// Mode 1: Tremor - accelerometer data in g
// Mode 2: Bradykinesia - contact state (0/1) and IMU angles in degrees
// Mode 3: Stiffness - normalized force (0-1) from 4 sensors and IMU angle in degrees
void readSensors(int mode, float &value1, float &value2, float &value3, float &value4, float &value5) {
  // These IMU functions are defined in imu_handler.h
  extern float roll1, pitch1, yaw1, roll2, pitch2, yaw2;
  extern float accelX1, accelY1, accelZ1, accelX2, accelY2, accelZ2;
  
  switch (mode) {
    case 1: // Tremor - accelerometer data in g
      // Convert raw accelerometer data to g (assuming ±2g range)
      value1 = accelX2 / 16384.0; 
      value2 = accelY2 / 16384.0;
      value3 = accelZ2 / 16384.0;
      value4 = 0; // Not used for tremor
      value5 = 0; // Not used for tremor
      break;
      
    case 2: // Bradykinesia
      value1 = analogRead(contactPin); // Boolean contact (0 or 1)
      value2 = roll1;                   // Angle in degrees
      value3 = roll2;                   // Angle in degrees
      value4 = 0; // Not used for bradykinesia
      value5 = 0; // Not used for bradykinesia
      break;
      
    case 3: // Stiffness - now with 4 force sensors + 1 angle
      value1 = analogRead(forcePin1); // Normalized 0.0-1.0
      value2 = analogRead(forcePin2); // Normalized 0.0-1.0
      value3 = roll2;                          // Angle in degrees
      value4 = analogRead(forcePin3);  // Normalized 0.0-1.0 (new)
      value5 = analogRead(forcePin4);  // Normalized 0.0-1.0 (new)
      break;
      
    default:
      value1 = 0;
      value2 = 0;
      value3 = 0;
      value4 = 0;
      value5 = 0;
      break;
  }
}

// Overloaded version for backward compatibility (3 values)
void readSensors(int mode, float &value1, float &value2, float &value3) {
  float value4, value5;
  readSensors(mode, value1, value2, value3, value4, value5);
}

#endif // SENSORS_H