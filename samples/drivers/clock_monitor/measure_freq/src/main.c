/*
 * Copyright (c) 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Sample: periodic frequency measurement through the clock_monitor
 * API.
 *
 * Targets whichever instance the board overlay aliases to
 * `clock-meter`. The sample configures METER mode with a 1 ms
 * window, then loops once per second calling clock_monitor_measure()
 * and logging the result in Hz. Which physical clock is measured
 * depends on the overlay's alias target and the binding's `clocks`
 * phandle.
 */

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/clock_monitor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(sample, LOG_LEVEL_INF);

#define METER DEVICE_DT_GET(DT_ALIAS(clock_meter))

int main(void)
{
	const struct device *dev = METER;

	if (!device_is_ready(dev)) {
		LOG_ERR("%s is not ready", dev->name);
		return -ENODEV;
	}

	/* Sanity-check that this instance actually supports METER mode. */
	const struct clock_monitor_capabilities *caps = clock_monitor_caps(dev);

	if (!(caps->supported_modes & BIT(CLOCK_MONITOR_MODE_METER))) {
		LOG_ERR("%s does not support METER mode", dev->name);
		return -ENOTSUP;
	}

	struct clock_monitor_config cfg = {
		.mode = CLOCK_MONITOR_MODE_METER,
		.meter = {
			.window_ns = CONFIG_SAMPLE_WINDOW_NS,
		},
	};

	int ret = clock_monitor_configure(dev, &cfg);

	if (ret != 0) {
		LOG_ERR("configure(%s) failed: %d", dev->name, ret);
		return ret;
	}

	LOG_INF("%s configured in METER mode, window = %u ns",
		dev->name, (uint32_t)CONFIG_SAMPLE_WINDOW_NS);

	while (1) {
		uint32_t hz = 0U;

		ret = clock_monitor_measure(dev, &hz, K_MSEC(50));
		if (ret == 0) {
			LOG_INF("Measured frequency = %u Hz", hz);
		} else if (ret == -EAGAIN) {
			LOG_WRN("measurement timed out");
		} else if (ret == -EIO) {
			LOG_WRN("monitored clock lost (FMTO-class event)");
		} else {
			LOG_ERR("measure() failed: %d", ret);
		}

		k_sleep(K_SECONDS(1));
	}

	return 0;
}
