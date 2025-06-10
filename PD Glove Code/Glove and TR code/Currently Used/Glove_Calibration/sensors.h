#ifndef SENSORS_H
#define SENSORS_H

// Define sensor pins
const int contactPin = 14;  // Pin for contact sensor
const int forcePin1 = 25;   // Pin for force sensor 1
const int forcePin2 = 26;   // Pin for force sensor 2
const int analogPin4 = 4;   // Pin for analog sensor (tremor and bradykinesia)

// Initialize the physical sensors
void initSensors() {
  // Set up pins
  pinMode(contactPin, INPUT);
  pinMode(forcePin1, INPUT);
  pinMode(forcePin2, INPUT);
  pinMode(analogPin4, INPUT);
  
  // Set ADC resolution to 12-bit (0-4095)
  analogReadResolution(12);
  
  Serial.println("Sensor pins initialized with 12-bit resolution");
}

// Read all sensors based on the selected mode and return raw values in meaningful units
// Mode 1: Tremor - analog pin 4 for value1, accelerometer Y and Z for value2/value3
// Mode 2: Bradykinesia - analog pin 4 for value1, IMU angles for value2/value3
// Mode 3: Stiffness - normalized force (0-1) and IMU angle in degrees
void readSensors(int mode, float &value1, float &value2, float &value3) {
  // These IMU functions are defined in imu_handler.h
  extern float roll1, pitch1, yaw1, roll2, pitch2, yaw2;
  extern float accelX1, accelY1, accelZ1, accelX2, accelY2, accelZ2;
  
  switch (mode) {
    case 1: // Tremor - analog pin 4 for value1, accelerometer for value2/value3
      value1 = analogRead(analogPin4);      // 12-bit analog read (0-4095)
      value2 = accelY2 / 8192.0;          // Accelerometer Y in g
      value3 = accelZ2 / 8192.0;          // Accelerometer Z in g
      break;
      
    case 2: // Bradykinesia - analog pin 4 for value1, IMU angles for value2/value3
      value1 = analogRead(analogPin4);      // 12-bit analog read (0-4095)
      value2 = roll1;                      // Angle in degrees
      value3 = roll2;                      // Angle in degrees
      break;
      
    case 3: // Stiffness
      value1 = analogRead(forcePin1) / 4095.0; // Normalized 0.0-1.0
      value2 = analogRead(forcePin2) / 4095.0;
      value3 = roll2;                          // Angle in degrees
      break;
      
    default:
      value1 = 0;
      value2 = 0;
      value3 = 0;
      break;
  }
}

#endif // SENSORS_H
