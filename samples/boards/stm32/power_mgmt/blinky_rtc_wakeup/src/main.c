/*
 * Copyright (c) 2021 SILA Embedded Solutions GmbH <office@embedded-solutions.at>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>
#include <sys/printk.h>
#include <drivers/counter.h>
#include <pm/pm.h>

#define SLEEP_TIME_MS 2000
#define ALARM_CHANNEL_ID 0

void rtc_callback(const struct device *dev, uint8_t chan_id, uint32_t ticks, void *user_data)
{
	printk("RTC callback");
}

void main(void)
{
	const struct device *rtc = device_get_binding(DT_LABEL(DT_NODELABEL(rtc)));
	const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
	int result;
	struct counter_alarm_cfg alarm_config;
	struct pm_state_info standby = {
		.state = PM_STATE_SOFT_OFF,
		.substate_id = 1
	};

	result = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);

	if (result != 0) {
		return;
	}

	alarm_config.callback = rtc_callback;
	alarm_config.ticks = counter_us_to_ticks(rtc, SLEEP_TIME_MS*1000);
	alarm_config.user_data = NULL;
	alarm_config.flags = 0;

	result = counter_start(rtc);

	if (result != 0) {
		return;
	}

	printk("Device ready\n");

	gpio_pin_set(led.port, led.pin, true);
	k_sleep(K_MSEC(SLEEP_TIME_MS));
	gpio_pin_set(led.port, led.pin, false);
	counter_set_channel_alarm(rtc, ALARM_CHANNEL_ID, &alarm_config);
	pm_power_state_force(0, standby);
}
