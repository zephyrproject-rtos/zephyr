/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include<zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/flash.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   1000

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)

#define MAIN_BASE_ADDRESS (0x00018000)

#define FLASH_0_NODE DT_NODELABEL(flashctl)


/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
const struct device *const dev = DEVICE_DT_GET(FLASH_0_NODE);
int main(void)
{
	int ret;
	bool led_state = true;
	uint32_t data[] = {0xDEADBEEF, 0xCAFEBABE};
	if (!gpio_is_ready_dt(&led)) {
		return 0;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return 0;
	}
	flash_write(dev, MAIN_BASE_ADDRESS, &data[0], 8);
	data[0] = 0x0;
	data[1] = 0x0;
	flash_read(dev, MAIN_BASE_ADDRESS, &data[0], 8);
	flash_erase(dev, MAIN_BASE_ADDRESS, 1024);
	while (1) {
		ret = gpio_pin_toggle_dt(&led);
		if (ret < 0) {
			return 0;
		}

		led_state = !led_state;
		printf("LED state: %s\n", led_state ? "ON" : "OFF");
		k_msleep(SLEEP_TIME_MS);
	}
	return 0;
}
