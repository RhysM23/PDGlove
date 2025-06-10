/****************************************************************
 * Example8_DMP_RawAccel.ino
 * ICM 20948 Arduino Library Demo
 * Initialize the DMP based on the TDK InvenSense ICM20948_eMD_nucleo_1.0 example-icm20948
 * Paul Clark, April 25th, 2021
 * Based on original code by:
 * Owen Lyke @ SparkFun Electronics
 * Original Creation Date: April 17 2019
 * 
 * ** This example is based on InvenSense's _confidential_ Application Note "Programming Sequence for DMP Hardware Functions".
 * ** We are grateful to InvenSense for sharing this with us.
 * 
 * ** Important note: by default the DMP functionality is disabled in the library
 * ** as the DMP firmware takes up 14301 Bytes of program memory.
 * ** To use the DMP, you will need to:
 * ** Edit ICM_20948_C.h
 * ** Uncomment line 29: #define ICM_20948_USE_DMP
 * ** Save changes
 * ** If you are using Windows, you can find ICM_20948_C.h in:
 * ** Documents\Arduino\libraries\SparkFun_ICM-20948_ArduinoLibrary\src\util
 *
 * Please see License.md for the license information.
 *
 * Distributed as-is; no warranty is given.
 ***************************************************************/
#include "ICM_20948.h" // Click here to get the library: http://librarymanager/All#SparkFun_ICM_20948_IMU

//#define USE_SPI       // Uncomment this to use SPI

#define SERIAL_PORT Serial

#define SPI_PORT SPI // Your desired SPI port.       Used only when "USE_SPI" is defined
#define CS_PIN 2     // Which pin you connect CS to. Used only when "USE_SPI" is defined

#define WIRE_PORT Wire // Your desired Wire port.      Used when "USE_SPI" is not defined
// The value of the last bit of the I2C address.
// On the SparkFun 9DoF IMU breakout the default is 1, and when the ADR jumper is closed the value becomes 0
#define AD0_VAL 1

#ifdef USE_SPI
ICM_20948_SPI myICM; // If using SPI create an ICM_20948_SPI object
#else
ICM_20948_I2C myICM; // Otherwise create an ICM_20948_I2C object
#endif

// Variables for averaging
const int numSamples = 100; // Number of samples to collect
float sumAccelX = 0, sumAccelY = 0, sumAccelZ = 0;
int sampleCount = 0;

// Offset variables
float offsetX = 45;
float offsetY = 2;
float offsetZ = 250;

// Position tracking variables
float posX = 0, posY = 0, posZ = 0;
float velX = 0, velY = 0, velZ = 0;
unsigned long lastUpdateTime = 0;

void setup()
{
  SERIAL_PORT.begin(115200); // Start the serial console
  SERIAL_PORT.println(F("ICM-20948 Example"));

  delay(100);

  while (SERIAL_PORT.available()) // Make sure the serial RX buffer is empty
    SERIAL_PORT.read();

  SERIAL_PORT.println(F("Press any key to continue..."));

  while (!SERIAL_PORT.available()) // Wait for the user to press a key (send any serial character)
    ;

#ifdef USE_SPI
  SPI_PORT.begin();
#else
  WIRE_PORT.begin();
  WIRE_PORT.setClock(400000);
#endif

  bool initialized = false;
  while (!initialized)
  {
    // Initialize the ICM-20948
#ifdef USE_SPI
    myICM.begin(CS_PIN, SPI_PORT);
#else
    myICM.begin(WIRE_PORT, AD0_VAL);
#endif

    SERIAL_PORT.print(F("Initialization of the sensor returned: "));
    SERIAL_PORT.println(myICM.statusString());
    if (myICM.status != ICM_20948_Stat_Ok)
    {
      SERIAL_PORT.println(F("Trying again..."));
      delay(500);
    }
    else
    {
      initialized = true;
    }
  }

  SERIAL_PORT.println(F("Device connected!"));

  bool success = true;

  // Initialize the DMP
  success &= (myICM.initializeDMP() == ICM_20948_Stat_Ok);

  // Enable the DMP accelerometer
  success &= (myICM.enableDMPSensor(INV_ICM20948_SENSOR_ACCELEROMETER) == ICM_20948_Stat_Ok);

  // Configuring DMP to output data at multiple ODRs:
  success &= (myICM.setDMPODRrate(DMP_ODR_Reg_Accel, 0) == ICM_20948_Stat_Ok); // Set to the maximum

  // Enable the FIFO
  success &= (myICM.enableFIFO() == ICM_20948_Stat_Ok);

  // Enable the DMP
  success &= (myICM.enableDMP() == ICM_20948_Stat_Ok);

  // Reset DMP
  success &= (myICM.resetDMP() == ICM_20948_Stat_Ok);

  // Reset FIFO
  success &= (myICM.resetFIFO() == ICM_20948_Stat_Ok);

  // Check success
  if (success)
  {
    SERIAL_PORT.println(F("DMP enabled!"));
  }
  else
  {
    SERIAL_PORT.println(F("Enable DMP failed!"));
    SERIAL_PORT.println(F("Please check that you have uncommented line 29 (#define ICM_20948_USE_DMP) in ICM_20948_C.h..."));
    while (1)
      ; // Do nothing more
  }

  // Initialize time
  lastUpdateTime = millis();
}

void loop()
{
  // Read any DMP data waiting in the FIFO
  icm_20948_DMP_data_t data;
  myICM.readDMPdataFromFIFO(&data);

  if ((myICM.status == ICM_20948_Stat_Ok) || (myICM.status == ICM_20948_Stat_FIFOMoreDataAvail))
  {
    if ((data.header & DMP_header_bitmap_Accel) > 0)
    {
      float acc_x = (float)data.Raw_Accel.Data.X - offsetX; // Apply offset
      float acc_y = (float)data.Raw_Accel.Data.Y - offsetY;
      float acc_z = (float)data.Raw_Accel.Data.Z - offsetZ;

      // Calculate time difference
      unsigned long currentTime = millis();
      float dt = (currentTime - lastUpdateTime) / 1000.0; // Convert ms to seconds

      // Calculate acceleration magnitude
      float accMagnitude = (sqrt(acc_x * acc_x + acc_y * acc_y + acc_z * acc_z)/8192)-1;
      ;

      // Update velocity and position
      velX += acc_x * dt;
      velY += acc_y * dt;
      velZ += acc_z * dt;

      posX += velX * dt;
      posY += velY * dt;
      posZ += velZ * dt;

      // Print position, velocity, and acceleration magnitude
//      SERIAL_PORT.print(F("Position X: "));
//      SERIAL_PORT.print(posX);
//      SERIAL_PORT.print(F(" Y: "));
//      SERIAL_PORT.print(posY);
//      SERIAL_PORT.print(F(" Z: "));
//      SERIAL_PORT.print(posZ);
//      SERIAL_PORT.print(F(" Vel X: "));
//      SERIAL_PORT.print(velX);
//      SERIAL_PORT.print(F(" Y: "));
//      SERIAL_PORT.print(velY);
//      SERIAL_PORT.print(F(" Z: "));
//      SERIAL_PORT.print(velZ);
      SERIAL_PORT.print(F(" Acceleration Magnitude: "));
      SERIAL_PORT.println(accMagnitude);

      // Update last update time
      lastUpdateTime = currentTime;
    }
  }

  if (myICM.status != ICM_20948_Stat_FIFOMoreDataAvail)
  {
    delay(1);
  }
}
