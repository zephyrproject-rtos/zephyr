/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   1000

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)
#define LED2_NODE DT_ALIAS(led2)

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(LED2_NODE, gpios);

#define __ASM __asm /*!< asm keyword for GNU Compiler */
#define __INLINE inline /*!< inline keyword for GNU Compiler */
#define __STATIC_INLINE static inline

__attribute__((always_inline)) __STATIC_INLINE uint32_t __get_PC(void)
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
	if (!gpio_is_ready_dt(&led1)) {
		return -1;
	}
	if (!gpio_is_ready_dt(&led2)) {
		return -1;
	}

	ret = gpio_pin_configure_dt(&led0, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return -1;
	}
	ret = gpio_pin_configure_dt(&led1, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return -1;
	}
	ret = gpio_pin_configure_dt(&led2, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return -1;
	}

	while (1) {
		ret = gpio_pin_toggle_dt(&led0);
		if (ret < 0) {
			return -1;
		}
		k_msleep(200);
		ret = gpio_pin_toggle_dt(&led1);
		if (ret < 0) {
			return -1;
		}
		k_msleep(300);
		ret = gpio_pin_toggle_dt(&led2);
		if (ret < 0) {
			return -1;
		}

		k_msleep(SLEEP_TIME_MS);
	}
	return 0;
}
