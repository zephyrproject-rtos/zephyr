/*
 * Copyright (c) 2025 John Doe
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Blink inbuilt LED example */

#include <Arduino.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS 1000

void setup() { pinMode(LED_BUILTIN, OUTPUT); }

void loop() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(SLEEP_TIME_MS);
  digitalWrite(LED_BUILTIN, LOW);
  delay(SLEEP_TIME_MS);
}
