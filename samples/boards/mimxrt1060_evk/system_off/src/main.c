/*
 * Copyright (c) 2022 Whisper.ai
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/policy.h>

#define BUSY_WAIT_S 2U
#define SLEEP_S 2U
#define SOFT_OFF_S 10U

#define SW0_NODE DT_ALIAS(sw0)
#if !DT_NODE_HAS_STATUS(SW0_NODE, okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif

#define SNVS_RTC_NODE DT_NODELABEL(snvs_rtc)
#if !DT_NODE_HAS_STATUS(SNVS_RTC_NODE, okay)
#error "Unsupported board: snvs_rtc node is not enabled"
#endif

#define SNVS_LP_RTC_ALARM_ID 1

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios, { 0 });
static const struct device *const snvs_rtc_dev = DEVICE_DT_GET(SNVS_RTC_NODE);

void main(void)
{
	printk("\n%s system off demo\n", CONFIG_BOARD);

	if (!device_is_ready(button.port)) {
		printk("Error: button device %s is not ready\n", button.port->name);
		return;
	}

	/* Configure to generate PORT event (wakeup) on button press. */
	/* No external pull-up on the user button, so configure one here */
	int ret = gpio_pin_configure_dt(&button, GPIO_INPUT | GPIO_PULL_UP);

	if (ret != 0) {
		printk("Error %d: failed to configure %s pin %d\n", ret, button.port->name,
		       button.pin);
		return;
	}

	ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_LEVEL_LOW);
	if (ret != 0) {
		printk("Error %d: failed to configure interrupt on %s pin %d\n", ret,
		       button.port->name, button.pin);
		return;
	}

	printk("Busy-wait %u s\n", BUSY_WAIT_S);
	k_busy_wait(BUSY_WAIT_S * USEC_PER_SEC);

	printk("Sleep %u s\n", SLEEP_S);
	k_sleep(K_SECONDS(SLEEP_S));

	uint32_t sleep_ticks = counter_us_to_ticks(snvs_rtc_dev, SOFT_OFF_S * 1000ULL * 1000ULL);

	if (sleep_ticks == 0) {
		/* counter_us_to_ticks will round down the number of ticks to the nearest int. */
		/* Ensure at least one tick is used in the RTC */
		sleep_ticks++;
	}
	const struct counter_alarm_cfg alarm_cfg = {
		.ticks = sleep_ticks,
		.flags = 0,
	};

	ret = counter_set_channel_alarm(snvs_rtc_dev, SNVS_LP_RTC_ALARM_ID, &alarm_cfg);
	if (ret != 0) {
		printk("Could not rtc alarm.\n");
		return;
	}
	printk("RTC Alarm set for %llu seconds to wake from soft-off.\n",
	       counter_ticks_to_us(snvs_rtc_dev, alarm_cfg.ticks) / (1000ULL * 1000ULL));
	printk("Entering system off; press %s to restart sooner\n", button.port->name);

	pm_state_force(0u, &(struct pm_state_info){ PM_STATE_SOFT_OFF, 0, 0 });

	/* Now we need to go sleep. This will let the idle thread runs and
	 * the pm subsystem will use the forced state. To confirm that the
	 * forced state is used, lets set the same timeout used previously.
	 */
	k_sleep(K_SECONDS(SLEEP_S));

	printk("ERROR: System off failed\n");
	while (true) {
		/* spin to avoid fall-off behavior */
	}
}
