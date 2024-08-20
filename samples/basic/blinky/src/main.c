/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <gpio.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   1000

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
int main(void)
{
	gpio_function_en(GPIO_PB1);
	gpio_output_en(GPIO_PB1);

	while (1) {
		gpio_toggle(GPIO_PB1);
		k_msleep(SLEEP_TIME_MS);
	}
	return 0;
}
