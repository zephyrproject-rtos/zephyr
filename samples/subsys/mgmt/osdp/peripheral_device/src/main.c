/*
 * Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/mgmt/osdp.h>

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)

#if !DT_NODE_HAS_STATUS(LED0_NODE, okay)
#error "BOARD does not define a debug LED"
#endif

static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET_OR(LED0_NODE, gpios, {0});

#define SLEEP_TIME_MS                  (20)
#define CNT_PER_SEC                    (1000 / SLEEP_TIME_MS)

int cmd_handler(void *unused, struct osdp_cmd *p)
{
	printk("App received command %d\n", p->id);
	return 0;
}

int main(void)
{
	int ret, led_state;
	uint32_t cnt = 0;

	if (!device_is_ready(led0.port)) {
		printk("LED0 GPIO port %s is not ready\n", led0.port->name);
		return 0;
	}

	ret = gpio_pin_configure_dt(&led0, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		printk("Failed to configure gpio port %s pin %d\n",
		       led0.port->name, led0.pin);
		return 0;
	}

	osdp_pd_set_command_callback(cmd_handler, NULL);

	led_state = 0;
	while (1) {
		if ((cnt & 0x7f) == 0x7f) {
			/* show a sign of life */
			led_state = !led_state;
		}
		gpio_pin_set(led0.port, led0.pin, led_state);
		k_msleep(SLEEP_TIME_MS);
		cnt++;
	}
	return 0;
}
