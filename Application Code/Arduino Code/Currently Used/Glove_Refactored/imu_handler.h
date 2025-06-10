#ifndef IMU_HANDLER_H
#define IMU_HANDLER_H

#include "ICM_20948.h"  // Include the SparkFun ICM-20948 library
#include <Wire.h>

// IMU configuration
#define WIRE_PORT Wire      // First I2C bus for IMU1
#define WIRE_PORT2 Wire1    // Second I2C bus for IMU2
#define AD0_VAL 0           // The value of the last bit of the I2C address

// IMU pins
#define SDA_1 21
#define SCL_1 22
#define SDA_2 15
#define SCL_2 27

// Create IMU objects
ICM_20948_I2C myIMU1;  // First IMU
ICM_20948_I2C myIMU2;  // Second IMU

// Variables to store IMU data
float roll1 = 0.0, pitch1 = 0.0, yaw1 = 0.0;
float roll2 = 0.0, pitch2 = 0.0, yaw2 = 0.0;
float accelX1 = 0.0, accelY1 = 0.0, accelZ1 = 0.0;
float accelX2 = 0.0, accelY2 = 0.0, accelZ2 = 0.0;

// Function to configure DMP for an IMU
bool configureDMP(ICM_20948_I2C &imu, const char* name) {
  bool success = true;
  
  // Initialize the DMP (Digital Motion Processor)
  success &= (imu.initializeDMP() == ICM_20948_Stat_Ok);
  
  // Enable the 9-axis orientation sensor (quaternion output)
  success &= (imu.enableDMPSensor(INV_ICM20948_SENSOR_ORIENTATION) == ICM_20948_Stat_Ok);
  
  // Using correct sensor enum for accelerometer
  success &= (imu.enableDMPSensor(INV_ICM20948_SENSOR_RAW_ACCELEROMETER) == ICM_20948_Stat_Ok);
  
  // Set the DMP output data rate to the maximum
  success &= (imu.setDMPODRrate(DMP_ODR_Reg_Quat9, 0) == ICM_20948_Stat_Ok);
  success &= (imu.setDMPODRrate(DMP_ODR_Reg_Accel, 0) == ICM_20948_Stat_Ok);
  
  // Enable the FIFO and DMP
  success &= (imu.enableFIFO() == ICM_20948_Stat_Ok);
  success &= (imu.enableDMP() == ICM_20948_Stat_Ok);
  
  // Reset the DMP and FIFO
  success &= (imu.resetDMP() == ICM_20948_Stat_Ok);
  success &= (imu.resetFIFO() == ICM_20948_Stat_Ok);
  
  // Check if everything initialized successfully
  if (success) {
    Serial.print(F("DMP enabled successfully for "));
    Serial.println(name);
  } else {
    Serial.print(F("Failed to enable DMP for "));
    Serial.println(name);
  }
  
  return success;
}

// Initialize the IMU sensors
bool initIMUSensors() {
  // Initialize first I2C bus
  WIRE_PORT.begin(SDA_1, SCL_1);
  WIRE_PORT.setClock(400000);  // Set I2C clock to 400kHz
  
  // Initialize second I2C bus
  WIRE_PORT2.begin(SDA_2, SCL_2);
  WIRE_PORT2.setClock(400000);
  
  // Initialize the first IMU
  Serial.println("Initializing IMU1...");
  bool initialized1 = false;
  for (int attempts = 0; attempts < 5 && !initialized1; attempts++) {
    myIMU1.begin(WIRE_PORT, AD0_VAL);
    if (myIMU1.status != ICM_20948_Stat_Ok) {
      Serial.println("IMU1 initialization failed, retrying...");
      delay(500);  // Wait before retrying
    } else {
      initialized1 = true;
      Serial.println("IMU1 connected!");
      configureDMP(myIMU1, "IMU1");
    }
  }
  
  // Initialize the second IMU
  Serial.println("Initializing IMU2...");
  bool initialized2 = false;
  for (int attempts = 0; attempts < 5 && !initialized2; attempts++) {
    myIMU2.begin(WIRE_PORT2, AD0_VAL);
    if (myIMU2.status != ICM_20948_Stat_Ok) {
      Serial.println("IMU2 initialization failed, retrying...");
      delay(500);  // Wait before retrying
    } else {
      initialized2 = true;
      Serial.println("IMU2 connected!");
      configureDMP(myIMU2, "IMU2");
    }
  }
  
  return initialized1 && initialized2;
}

// Read IMU data from both IMUs
void readIMUData() {
  // Process data from IMU1
  icm_20948_DMP_data_t data1;
  myIMU1.readDMPdataFromFIFO(&data1);
  
  if ((myIMU1.status == ICM_20948_Stat_Ok) || (myIMU1.status == ICM_20948_Stat_FIFOMoreDataAvail)) {
    // Read quaternion data for orientation
    if ((data1.header & DMP_header_bitmap_Quat9) > 0) {
      // Convert quaternion to +/- 1
      double q1 = ((double)data1.Quat9.Data.Q1) / 1073741824.0;  // Q1 (x)
      double q2 = ((double)data1.Quat9.Data.Q2) / 1073741824.0;  // Q2 (y)
      double q3 = ((double)data1.Quat9.Data.Q3) / 1073741824.0;  // Q3 (z)
      double q0 = sqrt(1.0 - ((q1 * q1) + (q2 * q2) + (q3 * q3)));  // Q0 (w)
      
      // Calculate Euler angles in radians
      roll1 = atan2(2.0 * (q0 * q1 + q2 * q3), 1.0 - 2.0 * (q1 * q1 + q2 * q2));
      pitch1 = asin(2.0 * (q0 * q2 - q3 * q1));
      yaw1 = atan2(2.0 * (q0 * q3 + q1 * q2), 1.0 - 2.0 * (q2 * q2 + q3 * q3));
      
      // Convert radians to degrees
      roll1 *= 180.0 / PI;
      pitch1 *= 180.0 / PI;
      yaw1 *= 180.0 / PI;
    }
    
    // Read accelerometer data
    if ((data1.header & DMP_header_bitmap_Accel) > 0) {
      // Scale accelerometer data
      accelX1 = (float)data1.Raw_Accel.Data.X;
      accelY1 = (float)data1.Raw_Accel.Data.Y;
      accelZ1 = (float)data1.Raw_Accel.Data.Z;
    }
  }
  
  // Process data from IMU2
  icm_20948_DMP_data_t data2;
  myIMU2.readDMPdataFromFIFO(&data2);
  
  if ((myIMU2.status == ICM_20948_Stat_Ok) || (myIMU2.status == ICM_20948_Stat_FIFOMoreDataAvail)) {
    // Read quaternion data for orientation
    if ((data2.header & DMP_header_bitmap_Quat9) > 0) {
      // Convert quaternion to +/- 1
      double q1 = ((double)data2.Quat9.Data.Q1) / 1073741824.0;  // Q1 (x)
      double q2 = ((double)data2.Quat9.Data.Q2) / 1073741824.0;  // Q2 (y)
      double q3 = ((double)data2.Quat9.Data.Q3) / 1073741824.0;  // Q3 (z)
      double q0 = sqrt(1.0 - ((q1 * q1) + (q2 * q2) + (q3 * q3)));  // Q0 (w)
      
      // Calculate Euler angles in radians
      roll2 = atan2(2.0 * (q0 * q1 + q2 * q3), 1.0 - 2.0 * (q1 * q1 + q2 * q2));
      pitch2 = asin(2.0 * (q0 * q2 - q3 * q1));
      yaw2 = atan2(2.0 * (q0 * q3 + q1 * q2), 1.0 - 2.0 * (q2 * q2 + q3 * q3));
      
      // Convert radians to degrees
      roll2 *= 180.0 / PI;
      pitch2 *= 180.0 / PI;
      yaw2 *= 180.0 / PI;
    }
    
    // Read accelerometer data
    if ((data2.header & DMP_header_bitmap_Accel) > 0) {
      // Scale accelerometer data
      accelX2 = (float)data2.Raw_Accel.Data.X;
      accelY2 = (float)data2.Raw_Accel.Data.Y;
      accelZ2 = (float)data2.Raw_Accel.Data.Z;
    }
  }
}

// Convert IMU angle data to the 0-4095 range to match the original analog values
int mapIMUAngleToAnalog(float angle) {
  // Map angle from -180 to 180 to 0-4095
  return map(constrain(angle, -180, 180), -180, 180, 0, 4095);
}

// Convert IMU acceleration data to the 0-4095 range
int mapIMUAccelToAnalog(float accel) {
  // Typical accelerometer range is +/- 2g or +/- 16g depending on settings
  // Map to the same range as analog readings
  return map(constrain(accel, -16000, 16000), -16000, 16000, 0, 4095);
}

#endif // IMU_HANDLER_H