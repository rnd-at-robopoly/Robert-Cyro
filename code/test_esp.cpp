// Code to test ESP32 with PlatformIO extension. Makes the blue led toggle on
// and off when touching pin 4

#include <Arduino.h>

const int LED_PIN = 2; // Onboard blue led
bool ledState = false;
bool wasTouched = false;

const int TOUCH_THRESHOLD = 600;

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  delay(1000);
  Serial.println("Tap pin P4 to toggle the blue led on or off");
}

void loop() {
  int touchValue = touchRead(T0); // Reads GPIO 4 (P4)
  bool isTouched = (touchValue < TOUCH_THRESHOLD);

  if (isTouched && !wasTouched) {
    ledState = !ledState; // Toggle on off
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);

    Serial.printf("finger detected! (Reading: %d) -> led is now %s\n",
                  touchValue, ledState ? "on" : "off");
  }

  wasTouched = isTouched;
  delay(50);
}