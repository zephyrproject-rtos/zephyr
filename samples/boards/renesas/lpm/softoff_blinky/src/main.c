/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/pm/pm.h>
#include <zephyr/device.h>
#include <zephyr/drivers/wuc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/counter.h>
#include "r_lpm.h"

#define LED0_NODE           DT_ALIAS(led0)
#define WAKEUP_COUNTER_NODE DT_NODELABEL(wakeup_counter)
#define WAKEUP_INTERVAL_SEC 1U

#if !DT_NODE_HAS_PROP(DT_PATH(zephyr_user), wakeup_ctrls)
#error "No wakeup-ctrls defined under /zephyr,user"
#endif

static const struct wuc_dt_spec wuc = WUC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct device *const wakeup_counter = DEVICE_DT_GET(WAKEUP_COUNTER_NODE);

static int enable_wakeup_source(void)
{
	int ret = wuc_enable_wakeup_source_dt(&wuc);

	if (ret != 0) {
		printk("Failed to enable wakeup source: %d\n", ret);
	}

	return ret;
}
/* Return true if the LED was toggled, false if not */
static bool led_wakeup_toggle(void)
{
	int new_state;

	if (wuc_check_wakeup_source_triggered_dt(&wuc) == 0) {
		/* Not woken by the expected source, do not toggle LED */
		return false;
	}

	wuc_clear_wakeup_source_triggered_dt(&wuc);

	gpio_pin_configure_dt(&led, GPIO_INPUT);

	new_state = !gpio_pin_get_dt(&led);

	gpio_pin_configure_dt(&led, new_state ? GPIO_OUTPUT_ACTIVE : GPIO_OUTPUT_INACTIVE);

	/* If deep-standby-options includes "io-port-keep", pins are held across
	 * Deep Software Standby reset (IOKEEP=1). Clear IOKEEP after restoring the
	 * desired GPIO configuration so the physical pins follow the new state.
	 */
	R_LPM_IoKeepClear(NULL);

	printk("Woke up from deep standby\n");
	printk("LED: %s\n", new_state ? "ON" : "OFF");

	return true;
}

static int start_wakeup_counter(void)
{
	int ret;
	struct counter_top_cfg top_cfg = {
		.ticks     = MAX(counter_us_to_ticks(wakeup_counter,
				(uint64_t)WAKEUP_INTERVAL_SEC * USEC_PER_SEC), 1U),
		.callback  = NULL,
		.user_data = NULL,
		.flags     = 0,
	};

	if (!device_is_ready(wakeup_counter)) {
		printk("Wakeup counter not ready\n");
		return -ENODEV;
	}

	ret = counter_set_top_value(wakeup_counter, &top_cfg);
	if (ret != 0) {
		printk("Failed to set counter top: %d\n", ret);
		return ret;
	}

	ret = counter_reset(wakeup_counter);
	if (ret != 0 && ret != -ENOSYS) {
		printk("Failed to reset counter: %d\n", ret);
		return ret;
	}

	ret = counter_start(wakeup_counter);
	if (ret != 0) {
		printk("Failed to start counter: %d\n", ret);
		return ret;
	}

	return 0;
}

int main(void)
{
	if (!device_is_ready(wuc.dev)) {
		printk("WUC device not ready: %s\n", wuc.dev->name);
		return 0;
	}

	if (!gpio_is_ready_dt(&led)) {
		printk("LED device not ready\n");
		return -ENODEV;
	}

	if (led_wakeup_toggle() == false) {
		/* Not woken by the expected source */
		gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
		printk("LED: ON\n");
	}

	if (enable_wakeup_source() != 0 ||
	    start_wakeup_counter() != 0) {
		return 0;
	}

	pm_state_force(0u, &(struct pm_state_info){ PM_STATE_SOFT_OFF, 0, 0 });
	k_sleep(K_FOREVER);

	return 0;
}
