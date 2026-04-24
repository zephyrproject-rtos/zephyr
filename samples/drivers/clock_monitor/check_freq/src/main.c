/*
 * Copyright (c) 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Sample: out-of-window frequency check through the clock_monitor API.
 *
 * Targets whichever instance the board overlay aliases to
 * `clock-monitor`. The sample configures WINDOW mode (expected
 * frequency, tolerance and window duration all Kconfig-tunable),
 * installs a callback that logs any threshold-crossing or
 * clock-lost events, and periodically polls get_events() from the
 * main loop to cover events that are not currently in the
 * callback's event_mask filter.
 */

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/clock_monitor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(sample, LOG_LEVEL_INF);

#define MONITOR DEVICE_DT_GET(DT_ALIAS(clock_monitor))

/*
 * Tunable parameters come straight from Kconfig
 * (see check_freq/Kconfig). If CONFIG_SAMPLE_EXPECTED_HZ is left at
 * its default of 0, the driver auto-queries the monitored clock's
 * rate via clock_control and uses it as the window centre.
 */

static void fault_cb(const struct device *dev,
		     const struct clock_monitor_event_data *evt,
		     void *user_data)
{
	ARG_UNUSED(user_data);

	if (evt->events & CLOCK_MONITOR_EVT_FREQ_HIGH) {
		LOG_ERR("[%s] monitored clock above upper threshold", dev->name);
	}
	if (evt->events & CLOCK_MONITOR_EVT_FREQ_LOW) {
		LOG_ERR("[%s] monitored clock below lower threshold (or lost)",
			dev->name);
	}
}

int main(void)
{
	const struct device *dev = MONITOR;

	if (!device_is_ready(dev)) {
		LOG_ERR("%s is not ready", dev->name);
		return -ENODEV;
	}

	const struct clock_monitor_capabilities *caps = clock_monitor_caps(dev);

	if (!(caps->supported_modes & BIT(CLOCK_MONITOR_MODE_WINDOW))) {
		LOG_ERR("%s does not support WINDOW mode", dev->name);
		return -ENOTSUP;
	}

	/* Subscribe to every event the back-end actually supports. */
	const uint32_t wanted =
		CLOCK_MONITOR_EVT_FREQ_HIGH |
		CLOCK_MONITOR_EVT_FREQ_LOW;

	struct clock_monitor_config cfg = {
		.mode = CLOCK_MONITOR_MODE_WINDOW,
		.window = {
			.expected_hz   = CONFIG_SAMPLE_EXPECTED_HZ,
			.tolerance_ppm = CONFIG_SAMPLE_TOLERANCE_PPM,
			.window_ns     = CONFIG_SAMPLE_WINDOW_NS,
			.event_mask    = caps->supported_events & wanted,
		},
		.callback  = fault_cb,
	};

	int ret = clock_monitor_configure(dev, &cfg);

	if (ret != 0) {
		LOG_ERR("configure(%s) failed: %d", dev->name, ret);
		return ret;
	}

	ret = clock_monitor_check(dev);
	if (ret != 0) {
		LOG_ERR("start(%s) failed: %d", dev->name, ret);
		return ret;
	}

	LOG_INF("clock check running (expected ~%u Hz, tolerance +/-%u ppm, "
		"window %u ns)",
		(uint32_t)CONFIG_SAMPLE_EXPECTED_HZ,
		(uint32_t)CONFIG_SAMPLE_TOLERANCE_PPM,
		(uint32_t)CONFIG_SAMPLE_WINDOW_NS);

	while (1) {
		k_sleep(K_SECONDS(5));

		/* Drain anything the callback might not have reported
		 * (e.g. events not in the callback's event_mask or
		 * latched while the CPU was busy).
		 */
		uint32_t drained = 0U;

		(void)clock_monitor_get_events(dev, &drained);

		if (drained == 0U) {
			LOG_INF("heartbeat: no events latched");
		} else if (drained & CLOCK_MONITOR_EVT_FREQ_HIGH) {
			LOG_WRN("heartbeat: monitored clock above "
				"upper threshold");
		} else if (drained & CLOCK_MONITOR_EVT_FREQ_LOW) {
			LOG_WRN("heartbeat: monitored clock below "
				"lower threshold (or lost)");
		} else {
			/* No actions taken. */
		}
	}

	return 0;
}
