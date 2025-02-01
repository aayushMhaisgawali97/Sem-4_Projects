// Define pins

#define SOUND_SENSOR_PIN 2 // The sound sensor digital output is connected to digital pin 2

#define LED_STRIP_PIN 9 // The LED strip positive terminal is connected to digital pin 9



void setup() {

Serial.begin(9600); // Initialize serial communication

pinMode(SOUND_SENSOR_PIN, INPUT); // Set the sound sensor as an input

pinMode(LED_STRIP_PIN, OUTPUT); // Set the LED strip control pin as an output

digitalWrite(LED_STRIP_PIN, HIGH); // Ensure the LED strip is off by default

}



void loop() {

// Check the sound sensor for high or low signals

int soundDetected = digitalRead(SOUND_SENSOR_PIN);



// If sound is detected (high signal)

if (soundDetected == HIGH) {

Serial.println("Sound detected! LED strip blinking...");

// Blink the LED strip for 2 seconds

for (int i = 0; i < 10; i++) { // Blink 10 times, 200ms per blink

digitalWrite(LED_STRIP_PIN, LOW); // Turn on the LED strip

delay(100); // Wait for 100ms

digitalWrite(LED_STRIP_PIN, HIGH); // Turn off the LED strip

delay(100); // Wait for 100ms

}

} ;