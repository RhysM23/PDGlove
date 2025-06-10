#include <Wire.h>

#define SDA_PIN 21
#define SCL_PIN 22
#define IMU_ADDR 0x68

// ICM-20948 Register Map (only what we need)
#define ICM20948_WHO_AM_I    0x00    // Should return 0xEA
#define ICM20948_PWR_MGMT_1  0x06    // Power management 1
#define ICM20948_ACCEL_XOUT_H 0x2D   // Accelerometer X-axis high byte
#define ICM20948_BANK_SEL    0x7F    // Bank selection register

// Function to write a byte to a register
void writeRegister(uint8_t bank, uint8_t reg, uint8_t value) {
  // Set the bank
  Wire.beginTransmission(IMU_ADDR);
  Wire.write(ICM20948_BANK_SEL);
  Wire.write(bank << 4); // Shift bank to proper position
  Wire.endTransmission();
  
  // Write the register
  Wire.beginTransmission(IMU_ADDR);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

// Function to read a byte from a register
uint8_t readRegister(uint8_t bank, uint8_t reg) {
  // Set the bank
  Wire.beginTransmission(IMU_ADDR);
  Wire.write(ICM20948_BANK_SEL);
  Wire.write(bank << 4); // Shift bank to proper position
  Wire.endTransmission();
  
  // Request register
  Wire.beginTransmission(IMU_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false); // false = don't send stop condition
  
  Wire.requestFrom(IMU_ADDR, 1);
  if(Wire.available()) {
    return Wire.read();
  }
  return 0;
}

// Function to read multiple bytes
void readRegisters(uint8_t bank, uint8_t reg, uint8_t count, uint8_t *dest) {
  // Set the bank
  Wire.beginTransmission(IMU_ADDR);
  Wire.write(ICM20948_BANK_SEL);
  Wire.write(bank << 4); // Shift bank to proper position
  Wire.endTransmission();
  
  // Request registers
  Wire.beginTransmission(IMU_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false); // false = don't send stop condition
  
  uint8_t i = 0;
  Wire.requestFrom(IMU_ADDR, count);
  while(Wire.available() && i < count) {
    dest[i++] = Wire.read();
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("Direct ICM-20948 Register Access Test");
  
  // Initialize I2C
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(50000); // 50kHz
  
  // Basic device detection
  Wire.beginTransmission(IMU_ADDR);
  byte error = Wire.endTransmission();
  
  if (error != 0) {
    Serial.print("Error connecting to device: ");
    Serial.println(error);
    while(1); // Stop here
  }
  
  Serial.println("Device detected at address 0x68");
  
  // Try to read WHO_AM_I register (Bank 0)
  uint8_t whoami = readRegister(0, ICM20948_WHO_AM_I);
  Serial.print("WHO_AM_I register: 0x");
  Serial.println(whoami, HEX);
  
  if (whoami == 0xEA) {
    Serial.println("Correct device ID detected (0xEA)");
  } else {
    Serial.println("Unexpected device ID! Should be 0xEA");
  }
  
  // Try to wake up the device
  Serial.println("Attempting to reset and wake up the device...");
  writeRegister(0, ICM20948_PWR_MGMT_1, 0x80); // Reset the device
  delay(100);
  writeRegister(0, ICM20948_PWR_MGMT_1, 0x01); // Auto select clock source
  delay(100);
  
  // Check if device is awake
  uint8_t pwr_status = readRegister(0, ICM20948_PWR_MGMT_1);
  Serial.print("Power management status: 0x");
  Serial.println(pwr_status, HEX);
  
  if (pwr_status & 0x40) {
    Serial.println("Device is still in sleep mode");
  } else {
    Serial.println("Device is awake");
  }
  
  Serial.println("Setup complete. Will read accelerometer data...");
}

void loop() {
  // Read accelerometer data
  uint8_t data[6];
  readRegisters(0, ICM20948_ACCEL_XOUT_H, 6, data);
  
  // Convert the data
  int16_t ax = (data[0] << 8) | data[1];
  int16_t ay = (data[2] << 8) | data[3];
  int16_t az = (data[4] << 8) | data[5];
  
  // Display the results
  Serial.println("Accelerometer (raw values):");
  Serial.print("X = ");
  Serial.println(ax);
  Serial.print("Y = ");
  Serial.println(ay);
  Serial.print("Z = ");
  Serial.println(az);
  Serial.println();
  
  delay(1000);
}
