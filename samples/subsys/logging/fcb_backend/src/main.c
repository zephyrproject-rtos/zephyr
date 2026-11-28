/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_output.h>
#include <zephyr/pm/device.h>



#include <zephyr/settings/settings.h>


LOG_MODULE_REGISTER(main);

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   5000

#if CONFIG_LED
#include <zephyr/drivers/led.h>
/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
#endif


int main(void)
{
	int ret=-1;
	LOG_INF("Example Started");
	static bool on = false;

	while (1) {
#if CONFIG_LED		
		ret = gpio_pin_toggle_dt(&led);
#else
		ret = 0;
#endif
		LOG_INF("Led %s", (on?"On":"Off"));
		on = !on;
		if (ret < 0) {
			return 0;
		}
		k_sleep( K_MSEC(SLEEP_TIME_MS));
	}

	return 0;
}
