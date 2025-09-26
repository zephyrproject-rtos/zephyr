/*
 * Copyright (c) 2022 TOKITA Hiroshi <tokita.hiroshi@fujitsu.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <Arduino.h>

void setup() {
  Serial.begin(115200);
}

void loop() {
}

void serialEvent() {
  while(Serial.available()) {
    Serial.print((char)Serial.read());
  }
}
