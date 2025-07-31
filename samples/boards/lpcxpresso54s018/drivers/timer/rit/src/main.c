/*
 * Copyright (c) 2024 VCI Development
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/timer/lpc_rit.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(rit_sample, LOG_LEVEL_INF);

#define RIT_DEVICE_NODE DT_NODELABEL(rit)

static volatile uint32_t timer_count = 0;

static void rit_timer_callback(const struct device *dev, void *user_data)
{
	timer_count++;
	LOG_INF("RIT timer expired! Count: %u", timer_count);
}

int main(void)
{
	const struct device *rit_dev;
	struct rit_timer_cfg cfg;
	uint32_t freq;
	int ret;

	LOG_INF("LPC54S018 RIT (Repetitive Interrupt Timer) Sample");

	/* Get RIT device */
	rit_dev = DEVICE_DT_GET(RIT_DEVICE_NODE);
	if (!device_is_ready(rit_dev)) {
		LOG_ERR("RIT device not ready");
		return -ENODEV;
	}

	/* Get timer frequency */
	freq = rit_get_frequency(rit_dev);
	LOG_INF("RIT frequency: %u Hz", freq);

	/* Configure for 1 second period with auto-clear */
	cfg.period = freq; /* 1 second */
	cfg.callback = rit_timer_callback;
	cfg.user_data = NULL;
	cfg.auto_clear = true;
	cfg.run_in_debug = false;

	ret = rit_configure(rit_dev, &cfg);
	if (ret) {
		LOG_ERR("Failed to configure RIT: %d", ret);
		return ret;
	}

	/* Start timer */
	ret = rit_start(rit_dev);
	if (ret) {
		LOG_ERR("Failed to start RIT: %d", ret);
		return ret;
	}

	LOG_INF("RIT started with 1 second period");

	/* Demonstrate mask functionality */
	k_sleep(K_SECONDS(3));
	LOG_INF("Setting mask to skip every other bit...");
	
	/* Set mask to 0x555555555555 (alternating bits) */
	rit_set_mask(rit_dev, 0x555555555555ULL);
	
	/* Main loop */
	while (1) {
		uint64_t current_value;
		
		/* Get current timer value */
		rit_get_value(rit_dev, &current_value);
		LOG_INF("Current timer value: 0x%llx", current_value);
		
		/* After 10 interrupts, change period */
		if (timer_count == 10) {
			LOG_INF("Changing period to 500ms...");
			rit_stop(rit_dev);
			
			cfg.period = freq / 2; /* 500ms */
			rit_configure(rit_dev, &cfg);
			rit_start(rit_dev);
			
			timer_count++; /* Avoid reconfiguring */
		}
		
		k_sleep(K_MSEC(250));
	}

	return 0;
}