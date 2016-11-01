/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <zephyr.h>
#include <device.h>
#include <gpio.h>

/**
 * the demo assumes use of nucleo_f103rb board, adjust defines below
 * to fit your board
 */

/* we're going to use PB8 and PB5 */
#define PORT "GPIOB"
/* PB5 */
#define LED1 5
/* PB8 */
#define LED2 8

#define SLEEP_TIME	500

void main(void)
{
	int cnt = 0;
	struct device *gpiob;

	gpiob = device_get_binding(PORT);

	gpio_pin_configure(gpiob, LED1, GPIO_DIR_OUT);
	gpio_pin_configure(gpiob, LED2, GPIO_DIR_OUT);

	while (1) {
		gpio_pin_write(gpiob, LED1, cnt % 2);
		gpio_pin_write(gpiob, LED2, (cnt + 1) % 2);
		k_sleep(SLEEP_TIME);
		cnt++;
	}
}
