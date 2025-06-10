#include "ICM_20948.h" // Include the SparkFun ICM-20948 library

#define SERIAL_PORT Serial
#define WIRE_PORT Wire
#define AD0_VAL 0
#define SDA_1 21  // SDA pin for first Wire (as specified)
#define SCL_1 22

ICM_20948_I2C myICM; // Create an ICM_20948_I2C object

void setup() {
  SERIAL_PORT.begin(115200); // Start the serial console

  // Initialize I2C communication
  WIRE_PORT.begin();
  WIRE_PORT.setClock(400000); // Set I2C clock to 400kHz for faster communication

  // Initialize the IMU
  bool initialized = false;
  while (!initialized) {
    myICM.begin(WIRE_PORT, AD0_VAL);

    if (myICM.status != ICM_20948_Stat_Ok) {
      SERIAL_PORT.println(F("Initialization failed. Trying again..."));
      delay(500); // Wait before retrying
    } else {
      initialized = true;
    }
  }

  SERIAL_PORT.println(F("Device connected!"));

  // Initialize the DMP (Digital Motion Processor)
  bool success = true;
  success &= (myICM.initializeDMP() == ICM_20948_Stat_Ok);

  // Enable the 9-axis orientation sensor (quaternion output)
  success &= (myICM.enableDMPSensor(INV_ICM20948_SENSOR_ORIENTATION) == ICM_20948_Stat_Ok);

  // Set the DMP output data rate to the maximum
  success &= (myICM.setDMPODRrate(DMP_ODR_Reg_Quat9, 0) == ICM_20948_Stat_Ok);

  // Enable the FIFO and DMP
  success &= (myICM.enableFIFO() == ICM_20948_Stat_Ok);
  success &= (myICM.enableDMP() == ICM_20948_Stat_Ok);
  
  // Reset the DMP and FIFO
  success &= (myICM.resetDMP() == ICM_20948_Stat_Ok);
  success &= (myICM.resetFIFO() == ICM_20948_Stat_Ok);

  // Check if everything initialized successfully
  if (success) {
    SERIAL_PORT.println(F("DMP enabled successfully!"));
  } else {
    SERIAL_PORT.println(F("Failed to enable DMP!"));
    while (1);
  }
}

void loop() {

  icm_20948_DMP_data_t data;
  myICM.readDMPdataFromFIFO(&data);

  if ((myICM.status == ICM_20948_Stat_Ok) || (myICM.status == ICM_20948_Stat_FIFOMoreDataAvail)) {
    if ((data.header & DMP_header_bitmap_Quat9) > 0) {
      // Convert quaternion to +/- 1
      double q1 = ((double)data.Quat9.Data.Q1) / 1073741824.0; // Q1 (x)
      double q2 = ((double)data.Quat9.Data.Q2) / 1073741824.0; // Q2 (y)
      double q3 = ((double)data.Quat9.Data.Q3) / 1073741824.0; // Q3 (z)
      double q0 = sqrt(1.0 - ((q1 * q1) + (q2 * q2) + (q3 * q3))); // Q0 (w)

      // Calculate Euler angles in radians
      double roll = atan2(2.0 * (q0 * q1 + q2 * q3), 1.0 - 2.0 * (q1 * q1 + q2 * q2));
      double pitch = asin(2.0 * (q0 * q2 - q3 * q1));
      double yaw = atan2(2.0 * (q0 * q3 + q1 * q2), 1.0 - 2.0 * (q2 * q2 + q3 * q3));

      // Convert radians to degrees
      roll *= 180.0 / PI;
      pitch *= 180.0 / PI;
      yaw *= 180.0 / PI;

      // Print out the results
      Serial.print("Roll: ");
      Serial.print(roll, 2);
      Serial.print(" Pitch: ");
      Serial.print(pitch, 2);
      Serial.print(" Yaw: "); 
      Serial.println(yaw, 2);
    }
  }

  if (myICM.status != ICM_20948_Stat_FIFOMoreDataAvail) {
    delay(10); // Small delay to prevent flooding the serial monitor
  }
}
