/*
 * Copyright (c) 2024 VCI Development
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(mrt_sample, LOG_LEVEL_INF);

#define MRT_NODE DT_CHILD(DT_NODELABEL(mrt0), ch0)
#define MRT_CHANNEL1_NODE DT_CHILD(DT_NODELABEL(mrt0), ch1)

static volatile bool timer_expired = false;

static void mrt_timer_handler(const struct device *dev, void *user_data)
{
	timer_expired = true;
	LOG_INF("MRT timer expired!");
}

int main(void)
{
	const struct device *mrt_dev;
	const struct device *mrt_ch1_dev;
	struct counter_top_cfg top_cfg;
	uint32_t freq;
	int ret;

	LOG_INF("LPC54S018 MRT (Multi-Rate Timer) Sample");

	/* Get MRT channel 0 device */
	mrt_dev = DEVICE_DT_GET(MRT_NODE);
	if (!device_is_ready(mrt_dev)) {
		LOG_ERR("MRT channel 0 device not ready");
		return -ENODEV;
	}

	/* Get MRT channel 1 device */
	mrt_ch1_dev = DEVICE_DT_GET(MRT_CHANNEL1_NODE);
	if (!device_is_ready(mrt_ch1_dev)) {
		LOG_ERR("MRT channel 1 device not ready");
		return -ENODEV;
	}

	/* Get timer frequency */
	freq = counter_get_frequency(mrt_dev);
	LOG_INF("MRT frequency: %u Hz", freq);

	/* Channel 0: Configure for 1 second timeout */
	top_cfg.ticks = freq; /* 1 second */
	top_cfg.callback = mrt_timer_handler;
	top_cfg.user_data = NULL;
	top_cfg.flags = 0;

	ret = counter_set_top_value(mrt_dev, &top_cfg);
	if (ret) {
		LOG_ERR("Failed to set top value: %d", ret);
		return ret;
	}

	/* Start channel 0 */
	ret = counter_start(mrt_dev);
	if (ret) {
		LOG_ERR("Failed to start counter: %d", ret);
		return ret;
	}

	LOG_INF("MRT channel 0 started with 1 second period");

	/* Channel 1: Configure for 500ms timeout */
	top_cfg.ticks = freq / 2; /* 500ms */
	top_cfg.callback = NULL; /* No callback for this channel */
	top_cfg.user_data = NULL;
	top_cfg.flags = 0;

	ret = counter_set_top_value(mrt_ch1_dev, &top_cfg);
	if (ret) {
		LOG_ERR("Failed to set channel 1 top value: %d", ret);
		return ret;
	}

	ret = counter_start(mrt_ch1_dev);
	if (ret) {
		LOG_ERR("Failed to start channel 1: %d", ret);
		return ret;
	}

	LOG_INF("MRT channel 1 started with 500ms period");

	/* Main loop */
	while (1) {
		if (timer_expired) {
			timer_expired = false;
			LOG_INF("Timer expired, restarting...");
			
			/* Restart the timer */
			counter_start(mrt_dev);
		}

		/* Check channel 1 value periodically */
		uint32_t ticks;
		counter_get_value(mrt_ch1_dev, &ticks);
		LOG_INF("Channel 1 counter value: %u", ticks);

		k_sleep(K_MSEC(250));
	}

	return 0;
}