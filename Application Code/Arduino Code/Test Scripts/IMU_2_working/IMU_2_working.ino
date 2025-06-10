#include "ICM_20948.h"  // Include the SparkFun ICM-20948 library
#include <Wire.h>

#define SERIAL_PORT Serial
#define WIRE_PORT Wire      // First I2C bus
#define WIRE_PORT2 Wire1    // Second I2C bus

#define AD0_VAL 0     // The value of the last bit of the I2C address (0 or 1)

// First IMU pins
#define SDA_1 21
#define SCL_1 22

// Second IMU pins
#define SDA_2 15
#define SCL_2 27

// Create two ICM_20948_I2C objects
ICM_20948_I2C myICM1;  // First IMU
ICM_20948_I2C myICM2;  // Second IMU

void setup() {
  SERIAL_PORT.begin(115200);  // Start the serial console
  
  // Initialize first I2C bus
  WIRE_PORT.begin(SDA_1, SCL_1);
  WIRE_PORT.setClock(400000);  // Set I2C clock to 400kHz
  
  // Initialize second I2C bus
  WIRE_PORT2.begin(SDA_2, SCL_2);
  WIRE_PORT2.setClock(400000);
  
  // Initialize the first IMU
  SERIAL_PORT.println(F("Initializing first IMU..."));
  bool initialized1 = false;
  while (!initialized1) {
    myICM1.begin(WIRE_PORT, AD0_VAL);
    if (myICM1.status != ICM_20948_Stat_Ok) {
      SERIAL_PORT.println(F("First IMU initialization failed. Trying again..."));
      delay(500);  // Wait before retrying
    } else {
      initialized1 = true;
      SERIAL_PORT.println(F("First IMU connected!"));
    }
  }
  
  // Initialize the second IMU
  SERIAL_PORT.println(F("Initializing second IMU..."));
  bool initialized2 = false;
  while (!initialized2) {
    myICM2.begin(WIRE_PORT2, AD0_VAL);
    if (myICM2.status != ICM_20948_Stat_Ok) {
      SERIAL_PORT.println(F("Second IMU initialization failed. Trying again..."));
      delay(500);  // Wait before retrying
    } else {
      initialized2 = true;
      SERIAL_PORT.println(F("Second IMU connected!"));
    }
  }
  
  // Configure first IMU
  configureDMP(myICM1, "IMU1");
  
  // Configure second IMU
  configureDMP(myICM2, "IMU2");
}

// Function to configure DMP for an IMU
bool configureDMP(ICM_20948_I2C &imu, const char* name) {
  bool success = true;
  
  // Initialize the DMP (Digital Motion Processor)
  success &= (imu.initializeDMP() == ICM_20948_Stat_Ok);
  
  // Enable the 9-axis orientation sensor (quaternion output)
  success &= (imu.enableDMPSensor(INV_ICM20948_SENSOR_ORIENTATION) == ICM_20948_Stat_Ok);
  
  // Set the DMP output data rate to the maximum
  success &= (imu.setDMPODRrate(DMP_ODR_Reg_Quat9, 0) == ICM_20948_Stat_Ok);
  
  // Enable the FIFO and DMP
  success &= (imu.enableFIFO() == ICM_20948_Stat_Ok);
  success &= (imu.enableDMP() == ICM_20948_Stat_Ok);
  
  // Reset the DMP and FIFO
  success &= (imu.resetDMP() == ICM_20948_Stat_Ok);
  success &= (imu.resetFIFO() == ICM_20948_Stat_Ok);
  
  // Check if everything initialized successfully
  if (success) {
    SERIAL_PORT.print(F("DMP enabled successfully for "));
    SERIAL_PORT.println(name);
  } else {
    SERIAL_PORT.print(F("Failed to enable DMP for "));
    SERIAL_PORT.println(name);
  }
  
  return success;
}

void loop() {
  // Process data from first IMU
  processIMUData(myICM1, "IMU1");
  
  // Process data from second IMU
  processIMUData(myICM2, "IMU2");
  
  // Small delay to prevent flooding the serial monitor
  delay(10);
}

void processIMUData(ICM_20948_I2C &imu, const char* name) {
  icm_20948_DMP_data_t data;
  imu.readDMPdataFromFIFO(&data);
  
  if ((imu.status == ICM_20948_Stat_Ok) || (imu.status == ICM_20948_Stat_FIFOMoreDataAvail)) {
    if ((data.header & DMP_header_bitmap_Quat9) > 0) {
      // Convert quaternion to +/- 1
      double q1 = ((double)data.Quat9.Data.Q1) / 1073741824.0;  // Q1 (x)
      double q2 = ((double)data.Quat9.Data.Q2) / 1073741824.0;  // Q2 (y)
      double q3 = ((double)data.Quat9.Data.Q3) / 1073741824.0;  // Q3 (z)
      double q0 = sqrt(1.0 - ((q1 * q1) + (q2 * q2) + (q3 * q3)));  // Q0 (w)
      
      // Calculate Euler angles in radians
      double roll = atan2(2.0 * (q0 * q1 + q2 * q3), 1.0 - 2.0 * (q1 * q1 + q2 * q2));
      double pitch = asin(2.0 * (q0 * q2 - q3 * q1));
      double yaw = atan2(2.0 * (q0 * q3 + q1 * q2), 1.0 - 2.0 * (q2 * q2 + q3 * q3));
      
      // Convert radians to degrees
      roll *= 180.0 / PI;
      pitch *= 180.0 / PI;
      yaw *= 180.0 / PI;
      
      // Print out the results with clear separation based on which IMU
      if (strcmp(name, "IMU1") == 0) {
        Serial.print("Roll1: ");
        Serial.print(roll, 2);
        Serial.print(" Pitch1: ");
        Serial.print(pitch, 2);
        Serial.print(" Yaw1: ");
        Serial.println(yaw, 2);
      } 
      else if (strcmp(name, "IMU2") == 0) {
        Serial.print("Roll2: ");
        Serial.print(roll, 2);
        Serial.print(" Pitch2: ");
        Serial.print(pitch, 2);
        Serial.print(" Yaw2: ");
        Serial.println(yaw, 2);
      }
    }
  }
}
