#include <Wire.h>

// Define the I2C pins for ESP32 Tiny Pico
// Try different combinations if these don't work
#define SDA_PIN 21
#define SCL_PIN 22

void setup() {
  Serial.begin(115200);
  
  // Wait for serial to initialize
  delay(1000);
  
  Serial.println("\nESP32 Tiny Pico I2C Scanner");
  Serial.println("This program scans for I2C devices on the bus");
  
  // Initialize I2C with specified pins
  Wire.begin(SDA_PIN, SCL_PIN);
  
  // Optional: lower the I2C clock speed for more reliability
  Wire.setClock(100000); // 100kHz instead of default 400kHz
}

void loop() {
  byte error, address;
  int deviceCount = 0;
  
  Serial.println("\nScanning I2C bus...");
  
  // The ICM-20948 is typically at address 0x68 or 0x69
  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    
    if (error == 0) {
      Serial.print("Device found at address 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.print(address, HEX);
      
      // Identify common I2C devices
      if (address == 0x68 || address == 0x69) {
        Serial.print(" (Possible ICM-20948 IMU)");
      }
      
      Serial.println();
      deviceCount++;
    } else if (error == 4) {
      Serial.print("Unknown error at address 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.println(address, HEX);
    }
  }
  
  if (deviceCount == 0) {
    Serial.println("No I2C devices found - check wiring or try different pins");
    
    // Additional troubleshooting info
    Serial.println("\nTroubleshooting tips:");
    Serial.println("1. Check that Vcc and GND are connected properly");
    Serial.println("2. Make sure SDA and SCL are not swapped");
    Serial.println("3. Try adding pull-up resistors (4.7k) to SDA and SCL");
    Serial.println("4. Try lower I2C frequency with Wire.setClock(50000)");
    Serial.println("5. Ensure the IMU's AD0 pin matches your address setting (0x68 or 0x69)");
    Serial.println("6. Check if your ESP32 Tiny Pico uses different I2C pins than expected");
  }
  
  Serial.println("\nDone. Waiting 5 seconds before next scan...");
  delay(5000);
}
