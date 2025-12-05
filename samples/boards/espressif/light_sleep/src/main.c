/*
 * Copyright (c) 2022-2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/device.h>
#include <esp_sleep.h>
#include <esp_timer.h>

/* Add an extra delay when sleeping to make sure that light sleep
 * is chosen.
 */
#define LIGHT_SLP_EXTRA_DELAY (50000UL)

void dev_enable_wakeup(const struct device *dev)
{
	if (!device_is_ready(dev)) {
		printk("%s: device not ready\n", dev->name);
	}

	if (pm_device_wakeup_is_capable(dev) && !pm_device_wakeup_enable(dev, true)) {
		printk("%s: could not enable wakeup source\n", dev->name);
	}
}

int main(void)
{
	const struct device *const dev_rtc = DEVICE_DT_GET(DT_NODELABEL(rtc_timer));

	dev_enable_wakeup(dev_rtc);

	while (true) {
		printk("Entering light sleep\n");
		/* To make sure the complete line is printed before entering sleep mode,
		 * need to wait until UART TX FIFO is empty
		 */
		k_busy_wait(10000);

		/* Get timestamp before entering sleep */
		int64_t t_before_ms = k_uptime_get();

		/* Sleep triggers the idle thread, which makes the pm subsystem select some
		 * pre-defined power state. Light sleep is used here because there is enough
		 * time to consider it, energy-wise, worthy.
		 */
		k_sleep(K_USEC(DT_PROP(DT_NODELABEL(light_sleep), min_residency_us) +
					LIGHT_SLP_EXTRA_DELAY));
		/* Execution continues here after wakeup */
		/* Get timestamp after waking up from sleep */
		int64_t t_after_ms = k_uptime_get();

		/* Determine wake up reason */
		const char *wakeup_reason;

		if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER) {
			wakeup_reason = "timer";
		} else {
			wakeup_reason = "other";
		}

		printk("Returned from light sleep, reason: %s, t=%lld ms, slept for %lld ms\n",
				wakeup_reason, t_after_ms, (t_after_ms - t_before_ms));
	}

	return 0;
}
