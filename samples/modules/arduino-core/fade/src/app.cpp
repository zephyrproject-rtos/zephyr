/*
 * Copyright (c) 2022 TOKITA Hiroshi <tokita.hiroshi@fujitsu.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <Arduino.h>

const int led = 3;  // PWM output pin.
const int increments = 5;
const int wait_ms = 10;

void setup() {
  /* Pin that use as the PWM output need not be configured by pinMode() */
}

void loop() {
  int value = 0;
  while (value < 256) {
    analogWrite(led, value);
    value += increments;
    delay(wait_ms);
  }

  value = 255;
  while (value >= 0) {
    analogWrite(led, value);
    value -= increments;
    delay(wait_ms);
  }
}
