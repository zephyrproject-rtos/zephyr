/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/toolchain/common.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   1000

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

/* Inline Function to display the PC register --> proove where the application is running */
ALWAYS_INLINE __STATIC_INLINE uint32_t __get_PC(void)
{
	register uint32_t result;

	__ASM volatile ("MOV %0, PC\n" : "=r" (result));
	return result;
}

int main(void)
{
	int ret;

	printk("Hello World! from external flash  %s\n", CONFIG_BOARD);
	printf("--> PC at 0x%x\n", __get_PC());

	if (!gpio_is_ready_dt(&led0)) {
		return -1;
	}

	ret = gpio_pin_configure_dt(&led0, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return -1;
	}

	while (1) {
		ret = gpio_pin_toggle_dt(&led0);
		if (ret < 0) {
			return -1;
		}

		k_msleep(SLEEP_TIME_MS);
	}
	return 0;
}
