#include <Wire.h>
#include "ICM_20948.h" 
#include <MadgwickAHRS.h> // Include Madgwick filter library

ICM_20948_I2C myICM;
Madgwick filter; // Create Madgwick filter instance

// Variables for velocity and position estimation
float velX = 0.0, velY = 0.0, velZ = 0.0;
float posX = 0.0, posY = 0.0, posZ = 0.0;

unsigned long prevTime = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // Initialize ICM-20948
  if (myICM.begin(Wire, 0x69) != ICM_20948_Stat_Ok) {
    Serial.println("ICM-20948 initialization failed!");
    while (1);
  }
  Serial.println("ICM-20948 initialized successfully!");

  filter.begin(100); // Initialize the Madgwick filter with an assumed sample rate
}

void loop() {
  if (myICM.dataReady()) {
    myICM.getAGMT();

    unsigned long currentTime = millis();
    float deltaTime = (currentTime - prevTime) / 1000.0; // Convert milliseconds to seconds
    prevTime = currentTime;

    // Get accelerometer and gyroscope readings
    float accX = myICM.accX() * 9.81; // Convert from g to m/s²
    float accY = myICM.accY() * 9.81;
    float accZ = myICM.accZ() * 9.81;
    float gyroX = myICM.gyrX() * (PI / 180.0); // Convert to radians/sec
    float gyroY = myICM.gyrY() * (PI / 180.0);
    float gyroZ = myICM.gyrZ() * (PI / 180.0);

    // Update Madgwick filter with gyroscope and accelerometer data
    filter.updateIMU(gyroX, gyroY, gyroZ, accX, accY, accZ);

    // Get orientation in quaternions
    float roll = filter.getRoll();
    float pitch = filter.getPitch();
    float yaw = filter.getYaw();

    // Rotate accelerometer data to remove gravity component
    float gravX = sin(pitch) * 9.81;
    float gravY = -sin(roll) * cos(pitch) * 9.81;
    float gravZ = cos(roll) * cos(pitch) * 9.81;

    float linearAccX = accX - gravX;
    float linearAccY = accY - gravY;
    float linearAccZ = accZ - gravZ;

    // Estimate velocity by integrating linear acceleration
    velX += linearAccX * deltaTime;
    velY += linearAccY * deltaTime;
    velZ += linearAccZ * deltaTime;

    // Estimate position by integrating velocity
    posX += velX * deltaTime;
    posY += velY * deltaTime;
    posZ += velZ * deltaTime;

    // Print estimated position and orientation
    Serial.print("Pitch: "); Serial.print(pitch);
    Serial.print(" Roll: "); Serial.print(roll);
    Serial.print(" Yaw: "); Serial.print(yaw);
    Serial.print(" Position X: "); Serial.print(posX);
    Serial.print(" Y: "); Serial.print(posY);
    Serial.print(" Z: "); Serial.println(posZ);
  }

  delay(10); // Adjust delay for real-time performance
}
