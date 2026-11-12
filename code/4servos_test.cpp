#include <Arduino.h>
#include <ESP32Servo.h>

// Pins for the 4 servos
const int NUM_SERVOS = 4;
const int servoPins[NUM_SERVOS] = {13, 14, 27, 26};

Servo servos[NUM_SERVOS];

void setup() {
  Serial.begin(115200);

  for (int i = 0; i < NUM_SERVOS; i++) {
    ESP32PWM::allocateTimer(i);
    servos[i].setPeriodHertz(50);
    servos[i].attach(servoPins[i], 500, 2400);
  }
}

void loop() {
  for (int posDegrees = 0; posDegrees <= 180; posDegrees += 5) {
    for (int i = 0; i < NUM_SERVOS; i++) {
      servos[i].write(posDegrees);
    }
    Serial.println(posDegrees);
    delay(20);
  }

  for (int posDegrees = 180; posDegrees >= 0; posDegrees -= 5) {
    for (int i = 0; i < NUM_SERVOS; i++) {
      servos[i].write(posDegrees);
    }
    Serial.println(posDegrees);
    delay(20);
  }
}
