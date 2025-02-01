#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <FastLED.h>

// WiFi credentials
const char* ssid = "WIFI_SSID";
const char* password = "PASSWORD";

//target wake up time
int targetHour = 7;
int targetMinute = 30;

// Timezone offset in seconds (e.g., for IST it's +5:30, so offset is 19800 seconds)
const long utcOffsetInSeconds = 19800;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

// Proximity Sensor Pin
const int proximityPin = D4; // Pin connected to the proximity sensor (Adjust as needed)
bool detectionActive = false;
bool wakeTimeCalculated = false; // Flag to prevent recalculating
unsigned long detectionStartTime = 0;

#define LED_PIN    D1    // Pin connected to the WS2812 data line 
#define NUM_LEDS   8    // Number of WS2812 LEDs in your strip

// Variables to store wake-up time
int calculatedWakeHour = -1;
int calculatedWakeMinute = -1;

// Define the array of leds
CRGB leds[NUM_LEDS];

// Sunrise duration in milliseconds
const unsigned long sunriseDuration = 300000; // 5 minutes in milliseconds
const unsigned long offDelay = 600000;     // 10 minutes in milliseconds

// Define the sunrise color gradient (CRGB values)
const CRGB sunriseColors[] = {
  CRGB(20, 0, 0),     // Dark Red
  CRGB(50, 10, 0),    // Orange-Red
  CRGB(100, 30, 0),   // Orange
  CRGB(150, 70, 10),  // Yellow-Orange
  CRGB(200, 120, 30), // Light Yellow-Orange
  CRGB(255, 180, 80), // Bright Yellow
  CRGB(255, 220, 150),// Very Light Yellow
  CRGB(255, 255, 200) // Almost White
};
const int numColorSteps = sizeof(sunriseColors) / sizeof(sunriseColors[0]);

unsigned long startTime;
bool sunriseComplete = false; // Flag to track if the sunrise is finished
unsigned long sunriseEndTime = 0; // Store the time when the sunrise completed
bool ledOn = true;        // Flag to track if LEDs are on

void calculateWakeTime(int currentHour, int currentMinute, int cycles, int& wakeHour, int& wakeMinute) {
  int fallAsleepTime = 15; // Time to fall asleep in minutes
  int cycleDuration = 90; // Length of one sleep cycle in minutes
  int totalMinutesToAdd = fallAsleepTime + (cycles * cycleDuration);
  int currentTimeInMinutes = (currentHour * 60) + currentMinute;
  int wakeTimeInMinutes = currentTimeInMinutes + totalMinutesToAdd;

  // Handle next day overflow
  if (wakeTimeInMinutes >= 1440) {
    wakeTimeInMinutes -= 1440;
  }

  // Convert back to hours and minutes
  wakeHour = wakeTimeInMinutes / 60;
  wakeMinute = wakeTimeInMinutes % 60;
}

void runSunriseAnimation() {
  unsigned long elapsedTime = millis() - startTime;

  if (elapsedTime <= sunriseDuration) {
    // Calculate the progress (0.0 to 1.0)
    float progress = (float)elapsedTime / sunriseDuration;

    // Calculate the brightness (0 to 255)
    uint8_t brightness = static_cast<uint8_t>(255 * progress);
    FastLED.setBrightness(brightness);

    // Calculate the color based on the progress
    float colorProgress = progress * (numColorSteps - 1);
    int colorIndex = static_cast<int>(colorProgress);
    float colorFraction = colorProgress - colorIndex;

    CRGB color1 = sunriseColors[colorIndex];
    CRGB color2 = sunriseColors[min(colorIndex + 1, numColorSteps - 1)];

    CRGB currentColor = blend(color1, color2, colorFraction * 255); // Blend using FastLED's blend function

    // Set all LEDs to the calculated color
    fill_solid(leds, NUM_LEDS, currentColor);
  } else if (!sunriseComplete) {
    // Sunrise is complete, keep the LEDs at the final color and brightness
    FastLED.setBrightness(255);
    fill_solid(leds, NUM_LEDS, sunriseColors[numColorSteps - 1]);
    sunriseComplete = true; // Set the flag so this block doesn't run repeatedly
    sunriseEndTime = millis(); // Record the time of sunrise completion
  }

  FastLED.show();
}

void turnOnLeds() {
  leds[0] = CRGB::Black;
  for (int i = 1; i < NUM_LEDS; ++i) {
    leds[i] = CRGB(255, 147, 41);
  }
  FastLED.show();
}

void turnOffLeds() {
  FastLED.clear();
  FastLED.show();
}

void setup() {
  Serial.begin(115200);

  // Initialize LEDs
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.clear(); // Initialize all pixels to 'off'
  FastLED.show();

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Initialize NTP client
  timeClient.begin();

  // Initialize proximity sensor
  pinMode(proximityPin, INPUT);

  startTime = millis();
  FastLED.setBrightness(100);
}

void loop() {
  static unsigned long lastNTPCheck = 0;
  const unsigned long ntpCheckInterval = 2 * 60 * 1000; // Check NTP every 2 minutes

  if (ledOn) {
    turnOnLeds();
  } else {
    turnOffLeds();
  }

  // Proximity sensor check
  int proximityState = digitalRead(proximityPin);
  if (proximityState == HIGH) {
    if (!detectionActive) {
      detectionActive = true;
      detectionStartTime = millis();
      FastLED.setBrightness(40);
    } else if (millis() - detectionStartTime >= 30000 && !wakeTimeCalculated) { // 30 seconds and not yet calculated
      wakeTimeCalculated = true;
      ledOn = false;

      // Fetch time and calculate wake-up time
      timeClient.update();
      int currentHour = timeClient.getHours();
      int currentMinute = timeClient.getMinutes();

      // Variables to store calculated wake-up times
      int wakeHour3, wakeMinute3;
      int wakeHour4, wakeMinute4;
      int wakeHour5, wakeMinute5;

      // Calculate wake-up times for 3, 4, and 5 cycles
      calculateWakeTime(currentHour, currentMinute, 3, wakeHour3, wakeMinute3);
      calculateWakeTime(currentHour, currentMinute, 4, wakeHour4, wakeMinute4);
      calculateWakeTime(currentHour, currentMinute, 5, wakeHour5, wakeMinute5);

       // Choose the closest wake-up time to 7:30 AM
      int actualTimeInMinutes = targetHour * 60 + targetMinute;

      // Convert wake-up times to minutes for easy comparison
      int wakeTime3InMinutes = wakeHour3 * 60 + wakeMinute3;
      int wakeTime4InMinutes = wakeHour4 * 60 + wakeMinute4;
      int wakeTime5InMinutes = wakeHour5 * 60 + wakeMinute5;

      // Calculate the difference between each calculated wake-up time and the actual time (7:30 AM)
      int diff3 = abs(wakeTime3InMinutes - actualTimeInMinutes);
      int diff4 = abs(wakeTime4InMinutes - actualTimeInMinutes);
      int diff5 = abs(wakeTime5InMinutes - actualTimeInMinutes);

      // Choose the closest wake-up time
      if (diff3 <= diff4 && diff3 <= diff5) {
        calculatedWakeHour = wakeHour3;
        calculatedWakeMinute = wakeMinute3;
      } else if (diff4 <= diff3 && diff4 <= diff5) {
        calculatedWakeHour = wakeHour4;
        calculatedWakeMinute = wakeMinute4;
      } else {
        calculatedWakeHour = wakeHour5;
        calculatedWakeMinute = wakeMinute5;
      }

      Serial.print("Closest wake-up time set to: ");
      Serial.print(calculatedWakeHour);
      Serial.print(":");
      if (calculatedWakeMinute < 10) Serial.print("0");
      Serial.println(calculatedWakeMinute);
    }
  } else {
    detectionActive = false;
    wakeTimeCalculated = false; // Reset flag when detection stops
    FastLED.setBrightness(100);
  }

  // Check time every 2 minutes
  if (millis() - lastNTPCheck >= ntpCheckInterval) {
    lastNTPCheck = millis();
    timeClient.update();
    int currentHour = timeClient.getHours();
    int currentMinute = timeClient.getMinutes();

    // Check if it's time to wake up or has already passed
    if (calculatedWakeHour != -1 && calculatedWakeMinute != -1) {
      if ((currentHour > calculatedWakeHour) ||
          (currentHour == calculatedWakeHour && currentMinute >= calculatedWakeMinute)) {
        Serial.println("Time to wake up!");
        while (!sunriseComplete) {
          runSunriseAnimation();
          delay(200);
        }
        while (!ledOn) { // Let the lamp stay white for 10 minutes
          if (millis() - sunriseEndTime >= offDelay) {
            ledOn = true;
            FastLED.setBrightness(100);
          }
          delay(1000);
        }
        calculatedWakeHour = -1; // Reset to prevent repeated triggers
        calculatedWakeMinute = -1;
      }
    }
  }

  delay(50);
}
