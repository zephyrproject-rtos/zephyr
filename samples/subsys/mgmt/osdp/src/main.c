/*
 * Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/gpio.h>
#include <mgmt/osdp.h>

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)

#if DT_NODE_HAS_STATUS(LED0_NODE, okay)
#define LED0    DT_GPIO_LABEL(LED0_NODE, gpios)
#define PIN     DT_GPIO_PIN(LED0_NODE, gpios)
#define FLAGS   DT_GPIO_FLAGS(LED0_NODE, gpios)
#else
#error "BOARD does not define a debug LED"
#define LED0    ""
#define PIN     0
#define FLAGS   0
#endif

#define SLEEP_TIME_MS                  10

int cmd_handler(struct osdp_cmd *p)
{
	printk("App received command %d\n", p->id);
	return 0;
}

void main(void)
{
	int ret, led_state;
	uint32_t cnt = 0;
	const struct device *dev;
	struct osdp_cmd cmd;

	dev = device_get_binding(LED0);
	if (dev == NULL) {
		printk("Failed to get LED0 binding\n");
		return;
	}

	ret = gpio_pin_configure(dev, PIN, GPIO_OUTPUT_ACTIVE | FLAGS);
	if (ret < 0) {
		printk("Failed to configure gpio pin\n");
		return;
	}

	led_state = 0;
	while (1) {
		if ((cnt & 0x7f) == 0x7f) {
			/* show a sign of life */
			led_state = !led_state;
		}
		if (osdp_pd_get_cmd(&cmd) == 0) {
			cmd_handler(&cmd);
		}
		gpio_pin_set(dev, PIN, led_state);
		k_msleep(SLEEP_TIME_MS);
		cnt++;
	}
}
