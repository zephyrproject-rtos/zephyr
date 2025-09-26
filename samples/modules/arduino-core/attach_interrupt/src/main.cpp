/*
 * Copyright (c) 2022 TOKITA Hiroshi <tokita.hiroshi@fujitsu.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <Arduino.h>

const pin_size_t ledPin = LED_BUILTIN;
const pin_size_t interruptPin = 2;
PinStatus state = LOW;

void blink() {
  state = (state == LOW) ? HIGH : LOW;
  digitalWrite(ledPin, state);
}

void setup() {
  pinMode(ledPin, OUTPUT);
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(interruptPin, blink, CHANGE);
}

void loop() {
}
