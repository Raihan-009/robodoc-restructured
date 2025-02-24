// NodeMCU Health Monitoring System
// Integrates MLX90614 temperature sensor, MAX30100 pulse oximeter, 
// LCD display, and IFTTT notifications

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include <MAX30100_PulseOximeter.h>
#include <LiquidCrystal_I2C.h>

// WiFi and IFTTT Configuration
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* IFTTT_KEY = "YOUR_IFTTT_KEY";
const char* IFTTT_EVENT = "health_alert";

// Initialize sensors and display
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
PulseOximeter pox;
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Set I2C address to 0x27

// Health thresholds
const float TEMP_THRESHOLD_HIGH = 37.5;
const float TEMP_THRESHOLD_LOW = 35.0;
const int SPO2_THRESHOLD = 95;

// Timing variables
unsigned long lastReadTime = 0;
const long READ_INTERVAL = 2000;  // Read every 2 seconds
unsigned long lastAlertTime = 0;
const long ALERT_COOLDOWN = 300000;  // 5 minutes between alerts

// Status variables
struct HealthData {
  float temperature;
  float heartRate;
  int spO2;
  bool isValid;
} currentHealth;

void setup() {
  // Initialize Serial for debugging
  Serial.begin(115200);
  
  // Initialize I2C
  Wire.begin();
  
  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("Initializing...");
  
  // Initialize MLX90614
  if (!mlx.begin()) {
    Serial.println("Error connecting to MLX sensor");
    handleError("MLX Error");
  }
  
  // Initialize MAX30100
  if (!pox.begin()) {
    Serial.println("Error connecting to MAX30100");
    handleError("MAX30100 Error");
  }
  
  // Configure MAX30100
  pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
  
  // Connect to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  
  // System ready indication
  lcd.clear();
  lcd.print("System Ready");
  delay(2000);
}

void loop() {
  // Update pulse oximeter
  pox.update();
  
  // Read sensors at intervals
  if (millis() - lastReadTime >= READ_INTERVAL) {
    // Read temperature
    float objTemp = mlx.readObjectTempC();
    
    // Get SpO2 and heart rate
    int spO2 = pox.getSpO2();
    float heartRate = pox.getHeartRate();
    
    // Update health data
    currentHealth.temperature = objTemp;
    currentHealth.spO2 = spO2;
    currentHealth.heartRate = heartRate;
    currentHealth.isValid = (spO2 > 0 && heartRate > 0);
    
    // Display readings
    displayReadings();
    
    // Check for alerts
    checkAlerts();
    
    lastReadTime = millis();
  }
}

void displayReadings() {
  lcd.clear();
  
  // First row: Temperature
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(currentHealth.temperature, 1);
  lcd.print("C");
  
  // Second row: SpO2 and HR
  lcd.setCursor(0, 1);
  lcd.print("SpO2:");
  lcd.print(currentHealth.spO2);
  lcd.print("% HR:");
  lcd.print((int)currentHealth.heartRate);
}

void checkAlerts() {
  bool shouldAlert = false;
  String alertMessage = "Health Alert: ";
  
  // Check temperature
  if (currentHealth.temperature > TEMP_THRESHOLD_HIGH) {
    alertMessage += "High temp " + String(currentHealth.temperature, 1) + "C. ";
    shouldAlert = true;
  }
  else if (currentHealth.temperature < TEMP_THRESHOLD_LOW) {
    alertMessage += "Low temp " + String(currentHealth.temperature, 1) + "C. ";
    shouldAlert = true;
  }
  
  // Check SpO2
  if (currentHealth.isValid && currentHealth.spO2 < SPO2_THRESHOLD) {
    alertMessage += "Low SpO2 " + String(currentHealth.spO2) + "%. ";
    shouldAlert = true;
  }
  
  // Send alert if needed and cooldown period has passed
  if (shouldAlert && (millis() - lastAlertTime >= ALERT_COOLDOWN)) {
    sendIFTTTAlert(alertMessage);
    lastAlertTime = millis();
  }
}

void sendIFTTTAlert(String message) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    
    // Construct IFTTT webhook URL
    String url = "http://maker.ifttt.com/trigger/";
    url += IFTTT_EVENT;
    url += "/with/key/";
    url += IFTTT_KEY;
    
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    
    // Construct JSON payload
    String jsonData = "{\"value1\":\"" + message + "\"}";
    
    // Send POST request
    int httpCode = http.POST(jsonData);
    
    if (httpCode > 0) {
      Serial.println("Alert sent successfully");
    } else {
      Serial.println("Error sending alert");
    }
    
    http.end();
  }
}

void handleError(const char* error) {
  lcd.clear();
  lcd.print("Error:");
  lcd.setCursor(0, 1);
  lcd.print(error);
  
  // Log error
  Serial.println(error);
  
  // Optional: Send error notification via IFTTT
  String errorMsg = "System Error: " + String(error);
  sendIFTTTAlert(errorMsg);
  
  delay(2000);
}