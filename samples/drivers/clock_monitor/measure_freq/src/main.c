/*
 * SPDX-FileCopyrightText: 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Sample: one-shot frequency measurement through the clock_monitor API.
 *
 * Targets whichever instance the board overlay aliases to
 * `clock-meter`. The sample configures MEASURE mode with a
 * CONFIG_SAMPLE_WINDOW_NS window and a callback that gives a
 * semaphore, starts the measurement with clock_monitor_start(), waits
 * for completion with an application-chosen timeout, and prints the
 * measured value in Hz. On the happy path no stop() is needed: MEASURE
 * auto-disarms after one completion (repeating sampling would call
 * start() again from the callback). stop() is only called to abort an
 * in-flight measurement on timeout. Which physical clock is measured
 * depends on the overlay's alias target and the binding's `clocks`
 * phandle.
 */

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_monitor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(sample, LOG_LEVEL_INF);

#define MEASURE_DEV DEVICE_DT_GET(DT_ALIAS(clock_meter))

/* Enable the clocks a board declares in its clock-monitor-required-clocks node. */
#if DT_HAS_COMPAT_STATUS_OKAY(clock_monitor_required_clocks)

#define REQUIRED_CLOCK_ON(node_id, prop, idx)                                  \
	do {                                                                   \
		const struct device *_clk =                                    \
			DEVICE_DT_GET(DT_CLOCKS_CTLR_BY_IDX(node_id, idx));    \
		if (device_is_ready(_clk)) {                                   \
			(void)clock_control_on(_clk,                           \
				(clock_control_subsys_t)(uintptr_t)            \
				DT_CLOCKS_CELL_BY_IDX(node_id, idx, name));    \
		}                                                              \
	} while (0)

static void enable_required_clocks(void)
{
	DT_FOREACH_PROP_ELEM_SEP(
		DT_COMPAT_GET_ANY_STATUS_OKAY(clock_monitor_required_clocks),
		clocks, REQUIRED_CLOCK_ON, (;));
}

#else

static void enable_required_clocks(void) {}

#endif

static K_SEM_DEFINE(measure_done, 0, 1);

static void measure_cb(const struct device *dev,
		       const struct clock_monitor_event_data *evt,
		       void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

	if ((evt->events & (CLOCK_MONITOR_EVT_MEASURE_DONE |
			    CLOCK_MONITOR_EVT_CLOCK_LOST)) != 0U) {
		k_sem_give(&measure_done);
	}
}

int main(void)
{
	const struct device *dev = MEASURE_DEV;

	if (!device_is_ready(dev)) {
		LOG_ERR("%s is not ready", dev->name);
		return -ENODEV;
	}

	/* Ensure any board-declared source clocks are running before configuring. */
	enable_required_clocks();

	struct clock_monitor_config cfg = {
		.mode = CLOCK_MONITOR_MODE_MEASURE,
		.measure = {
			.window_ns = CONFIG_SAMPLE_WINDOW_NS,
		},
		.callback = measure_cb,
	};

	int ret = clock_monitor_configure(dev, &cfg);

	if (ret != 0) {
		LOG_ERR("configure(%s) failed: %d", dev->name, ret);
		return ret;
	}

	LOG_INF("%s configured in MEASURE mode, window = %u ns",
		dev->name, (uint32_t)CONFIG_SAMPLE_WINDOW_NS);

	ret = clock_monitor_start(dev);
	if (ret != 0) {
		LOG_ERR("start(%s) failed: %d", dev->name, ret);
		return ret;
	}

	if (k_sem_take(&measure_done, K_MSEC(50)) != 0) {
		/* Timeout ownership lies with the application: abort the
		 * in-flight measurement explicitly.
		 */
		(void)clock_monitor_stop(dev);
		LOG_WRN("measurement timed out");
		return -EAGAIN;
	}

	uint32_t hz = 0U;

	ret = clock_monitor_get_rate(dev, &hz);
	if (ret == 0) {
		LOG_INF("Measured frequency = %u Hz", hz);
	} else if (ret == -EIO) {
		LOG_WRN("monitored clock lost (FMTO-class event)");
	} else {
		LOG_ERR("get_rate() failed: %d", ret);
	}

	return ret;
}
