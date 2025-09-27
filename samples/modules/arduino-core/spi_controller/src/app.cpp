/*
 * Copyright (c) 2024 Ayush Singh <ayush@beagleboard.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "SPI.h"
#include <Arduino.h>

#define CHIPSELECT 3

static uint8_t data = 0;

void setup() {
  SPI.begin();
  pinMode(CHIPSELECT, OUTPUT);
  digitalWrite(CHIPSELECT, HIGH);
}

void loop() {
  SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
  digitalWrite(CHIPSELECT, LOW);
  SPI.transfer(data++);
  digitalWrite(CHIPSELECT, HIGH);
  SPI.endTransaction();
  delay(1000);
}
