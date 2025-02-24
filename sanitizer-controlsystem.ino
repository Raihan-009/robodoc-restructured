// Arduino Automatic Hand Sanitizer
// Uses HC-SR04 ultrasonic sensor and 12V DC pump motor

// Pin Definitions
const int TRIG_PIN = 9;    // Ultrasonic sensor trigger pin
const int ECHO_PIN = 10;   // Ultrasonic sensor echo pin
const int PUMP_PIN = 7;    // Pump motor control pin (via relay)
const int LED_PIN = 13;    // Status LED pin

// Constants
const int DISTANCE_THRESHOLD = 10;  // Distance threshold in cm
const int SPRAY_DURATION = 1000;    // Spray duration in milliseconds
const int COOLDOWN_TIME = 2000;     // Time between sprays in milliseconds

// Variables
unsigned long lastSprayTime = 0;    // Last spray timestamp
bool isSystemActive = false;        // System status flag

void setup() {
  // Initialize Serial Monitor
  Serial.begin(9600);
  
  // Configure pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  
  // Ensure pump is off at start
  digitalWrite(PUMP_PIN, LOW);
  
  // System startup indication
  digitalWrite(LED_PIN, HIGH);
  delay(1000);
  digitalWrite(LED_PIN, LOW);
  
  Serial.println("Sanitizer System Ready!");
}

void loop() {
  // Get current distance reading
  int distance = measureDistance();
  
  // Print distance for debugging
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");
  
  // Check if hand is detected and system is ready
  if (distance <= DISTANCE_THRESHOLD && 
      (millis() - lastSprayTime) > COOLDOWN_TIME) {
    activatePump();
  }
  
  // Small delay to prevent excessive readings
  delay(100);
}

int measureDistance() {
  // Clear trigger pin
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  
  // Send ultrasonic pulse
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  // Measure echo duration
  long duration = pulseIn(ECHO_PIN, HIGH);
  
  // Calculate distance
  // Speed of sound = 343 m/s = 0.0343 cm/µs
  // Distance = (duration × 0.0343) / 2
  int distance = duration * 0.0343 / 2;
  
  return distance;
}

void activatePump() {
  // Update last spray time
  lastSprayTime = millis();
  
  // Activate pump and LED
  digitalWrite(LED_PIN, HIGH);
  digitalWrite(PUMP_PIN, HIGH);
  
  // Keep pump on for spray duration
  delay(SPRAY_DURATION);
  
  // Turn off pump and LED
  digitalWrite(PUMP_PIN, LOW);
  digitalWrite(LED_PIN, LOW);
  
  // Log activation
  Serial.println("Sanitizer dispensed!");
  
  // Small delay to ensure pump fully stops
  delay(100);
}

// System status check function
bool checkSystemStatus() {
  // Basic system check (can be expanded)
  bool sensorWorking = measureDistance() >= 0;
  bool pumpResponding = true; // Add actual pump check if needed
  
  return sensorWorking && pumpResponding;
}

// Error handling function
void handleError(const char* error) {
  Serial.print("ERROR: ");
  Serial.println(error);
  
  // Visual error indication
  for(int i = 0; i < 3; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(200);
    digitalWrite(LED_PIN, LOW);
    delay(200);
  }
}