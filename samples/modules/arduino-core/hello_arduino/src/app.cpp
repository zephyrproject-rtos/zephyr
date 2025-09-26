/*
 * Copyright (c) 2025 John Doe
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <Arduino.h>
#include "zephyrSerial.h"


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200); // dummy as of now, need to study and refer https://docs.zephyrproject.org/latest/hardware/peripherals/uart.html
}
void loop() {
  char c = 'D';
  size_t ret1;
  size_t ret2;
  ret1 = Serial.print(c);
  ret2 = Serial.println("Hello, World!");
  printk("Sizes: %d %d\n", ret1, ret2);
  Serial.println();
  ret1 = Serial.print("My letter is: ");
  ret2 = Serial.println(c);
  printk("Sizes: %d %d\n", ret1, ret2);
  Serial.println();
  char myString[] = "Will it print?";
  ret1 = Serial.println(myString);
  printk("Size: %d \n\n\n", ret1);
  delay(1000); // 1 second delay
}
