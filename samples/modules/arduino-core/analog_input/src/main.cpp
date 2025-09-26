/*
 * Copyright (c) 2022 TOKITA Hiroshi <tokita.hiroshi@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <Arduino.h>

const int analog_input = A0;    // select the input pin for the potentiometer
const int ledPin = LED_BUILTIN;      // select the pin for the LED
const float wait_factor = 1.f;

void setup() {
  pinMode(ledPin, OUTPUT);
}

void loop() {
  int value = 0;

  value = analogRead(analog_input);

  /* Blinks slowly when the input voltage is high */

  digitalWrite(ledPin, HIGH);
  delay(value * wait_factor);

  digitalWrite(ledPin, LOW);
  delay(value * wait_factor);
}
