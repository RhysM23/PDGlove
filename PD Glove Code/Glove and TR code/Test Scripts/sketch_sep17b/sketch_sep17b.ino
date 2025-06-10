#include <ICM_20948.h>
#include <Wire.h>

#define SERIAL_PORT Serial
#define WIRE_PORT Wire
#define AD0_VAL 1

ICM_20948_I2C myICM;

void setup() {
  SERIAL_PORT.begin(115200);
  WIRE_PORT.begin();
  WIRE_PORT.setClock(400000);

  if (myICM.begin(WIRE_PORT, AD0_VAL) != ICM_20948_Stat_Ok) {
    SERIAL_PORT.println("Failed to initialize sensor.");
    while (1);
  }

  bool success = true;
  success &= (myICM.enableDMPSensor(INV_ICM20948_SENSOR_ACCELEROMETER) == ICM_20948_Stat_Ok);
  success &= (myICM.enableFIFO() == ICM_20948_Stat_Ok);
  success &= (myICM.enableDMP() == ICM_20948_Stat_Ok);
  success &= (myICM.resetDMP() == ICM_20948_Stat_Ok);
  success &= (myICM.resetFIFO() == ICM_20948_Stat_Ok);

  if (!success) {
    SERIAL_PORT.println("Failed to configure FIFO or DMP.");
    while (1);
  }

  SERIAL_PORT.println("FIFO and DMP enabled and reset.");
}

void loop() {
  icm_20948_DMP_data_t data;
  ICM_20948_Status_e status = myICM.readDMPdataFromFIFO(&data);

  if (status == ICM_20948_Stat_Ok) {
    SERIAL_PORT.println("Data read from FIFO.");
    // Process your data here
  } else if (status == ICM_20948_Stat_FIFONoDataAvail) {
    SERIAL_PORT.println("No FIFO Data Available.");
  } else {
    SERIAL_PORT.print("Error reading FIFO: ");
    SERIAL_PORT.println(status);
  }

  delay(100); // Adjust delay as needed
}
