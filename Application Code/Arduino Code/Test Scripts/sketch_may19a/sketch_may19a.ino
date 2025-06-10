void setup() {
  Serial.begin(9600);
  while (!Serial) delay(10);
  
  // Set ADC resolution to 12-bit (0-4095 range)
  analogReadResolution(12);
}

void loop() {
  // Read 12-bit analog value from pin 4 (returns 0-4095)
  int analogValue = analogRead(4);
  
  Serial.print("12-bit reading from pin 4: ");
  Serial.println(analogValue);
  
  // Convert to voltage with 12-bit precision (3.3V reference)
  float voltage = analogValue * (3.3 / 4095.0);
  Serial.print("Voltage: ");
  Serial.print(voltage, 4);  // 4 decimal places for precision
  Serial.println("V");
  
  delay(500);
}
