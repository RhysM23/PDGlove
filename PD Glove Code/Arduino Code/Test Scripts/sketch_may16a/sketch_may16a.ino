// Force Sensor Calibration Sketch
// Reads analog values from force sensors and outputs to Serial

// Define force sensor pins (using same pins as in your main code)
const int forcePin1 = 25;   // Force sensor 1
const int forcePin2 = 26;   // Force sensor 2

// Variables to track min/max values for calibration
int minForce1 = 4095;
int maxForce1 = 0;
int minForce2 = 4095;
int maxForce2 = 0;

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  while (!Serial) {
    ; // Wait for serial port to connect
  }
  
  Serial.println("Force Sensor Calibration");
  Serial.println("------------------------");
  Serial.println("Apply different pressures to calibrate your sensors");
  Serial.println("Format: Time(ms), Force1, Force2, Min1, Max1, Min2, Max2");
  
  // Set pin modes
  pinMode(forcePin1, INPUT);
  pinMode(forcePin2, INPUT);
  
  // Wait a moment to let things stabilize
  delay(1000);
}

void loop() {
  // Read the force sensor values
  int force1 = analogRead(forcePin1);
  int force2 = analogRead(forcePin2);
  
  // Update min/max values
  if (force1 < minForce1 && force1 > 0) minForce1 = force1;
  if (force1 > maxForce1) maxForce1 = force1;
  if (force2 < minForce2 && force2 > 0) minForce2 = force2;
  if (force2 > maxForce2) maxForce2 = force2;
  
  // Print the values
  Serial.print(millis());
  Serial.print(",\t");
  Serial.print(force1);
  Serial.print(",\t");
  Serial.print(force2);
  Serial.print(",\t");
  Serial.print(minForce1);
  Serial.print(",\t");
  Serial.print(maxForce1);
  Serial.print(",\t");
  Serial.print(minForce2);
  Serial.print(",\t");
  Serial.println(maxForce2);
  
  // Short delay between readings
  delay(100);
}
