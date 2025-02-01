#include <Servo.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Adafruit_NeoPixel.h>

// Pin Definitions
const int gasSensorPin = A0;    // Analog pin connected to the MQ2 gas sensor
const int buzzerPin = 8;        // Digital pin connected to the buzzer
const int servoPin = 9;         // Digital pin connected to the servo
const int ledPin = 6;           // Digital pin connected to WS2812B LED strip
const int gasThreshold = 400;   // Threshold value for gas detection (adjust based on your testing)

// OLED Display Settings
#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

// WS2812B LED Strip Settings
#define NUM_LEDS 4
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, ledPin, NEO_GRB + NEO_KHZ800);

Servo myServo;                  // Create servo object to control a servo
bool servoMoved = false;        // Flag to check if the servo has already moved

void setup() {
  pinMode(gasSensorPin, INPUT);
  pinMode(buzzerPin, OUTPUT);
  myServo.attach(servoPin);
  myServo.write(0);             // Initial position of the servo

  // Initialize OLED Display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Initialize OLED with I2C address 0x3C
  display.clearDisplay();
  display.setTextColor(WHITE);

  // Initialize WS2812B LED Strip
  strip.begin();
  strip.show();                 // Initialize all pixels to 'off'
  strip.fill(strip.Color(0, 255, 0)); // Green (SAFE mode)
  strip.show();

  // Display SAFE status initially
  displaySafeStatus();

  Serial.begin(9600);           // Initialize Serial for debugging
}

void loop() {
  int gasLevel = analogRead(gasSensorPin); // Read the gas sensor value
  Serial.println(gasLevel);               // Print gas sensor value for debugging

  if (gasLevel > gasThreshold && !servoMoved) {
    // Gas Detected: Activate Alarm
    activateAlarm();
  } else if (gasLevel <= gasThreshold) {
    // Standby: Display "SAFE"
    if (!servoMoved) {
      displaySafeStatus();
    }
  }

  delay(1000);
}

// Function to activate alarm
void activateAlarm() {
  digitalWrite(buzzerPin, HIGH);
  strip.fill(strip.Color(255, 0, 0));  // Red
  strip.show();

  // Display "Leakage" status on OLED
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 10);
  display.println("Leakage");
  display.display();

  delay(2000);                         // Beep for 2 seconds
  digitalWrite(buzzerPin, LOW);

  myServo.write(90);                   // Rotate servo to 90 degrees
  servoMoved = true;                   // Prevent multiple movements
}

// Function to display "SAFE" status
void displaySafeStatus() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 10);
  display.println("SAFE");
  display.display();

  strip.fill(strip.Color(0, 255, 0)); // Green
  strip.show();
}
