/*
 * Copyright (c) 2022 Dhruva Gole
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <Arduino.h>

/* size of stack area used by each thread */
#define STACKSIZE 1024

/* scheduling priority used by each thread */
#define PRIORITY 7

void blink0(void)
{
	while (1) {
		digitalWrite(LED_BUILTIN, HIGH);
		delay(100);
		digitalWrite(LED_BUILTIN, LOW);
		delay(100);
	}
}

void blink1(void)
{
	while (1) {
		digitalWrite(D11, HIGH);
		delay(1000);
		digitalWrite(D11, LOW);
		delay(1000);
	}
}

K_THREAD_DEFINE(blink0_id, STACKSIZE, blink0, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(blink1_id, STACKSIZE, blink1, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(blink2_id, STACKSIZE, loop, NULL, NULL, NULL, PRIORITY, 0, 0);

void setup()
{
	pinMode(LED_BUILTIN, OUTPUT);
	pinMode(D11, OUTPUT);
	pinMode(D10, OUTPUT);
}
void loop()
{
		digitalWrite(D10, HIGH);
		delay(300);
		digitalWrite(D10, LOW);
		delay(300);
}
