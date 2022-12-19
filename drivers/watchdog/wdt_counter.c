/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/watchdog.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/logging/log_ctrl.h>

#define WDT_CHANNEL_COUNT DT_PROP(DT_WDT_COUNTER, num_channels)
#define DT_WDT_COUNTER DT_COMPAT_GET_ANY_STATUS_OKAY(zephyr_counter_watchdog)

#define WDT_SUPPORTED_CFG_FLAGS (WDT_FLAG_RESET_NONE | WDT_FLAG_RESET_SOC)

extern void sys_arch_reboot(int type);

struct wdt_counter_data {
	wdt_callback_t callback[CONFIG_WDT_COUNTER_CH_COUNT];
	uint32_t timeout[CONFIG_WDT_COUNTER_CH_COUNT];
	uint8_t flags[CONFIG_WDT_COUNTER_CH_COUNT];
	uint8_t alloc_cnt;
};

static struct wdt_counter_data wdt_data;

struct wdt_counter_config {
	const struct device *counter;
};

static int wdt_counter_setup(const struct device *dev, uint8_t options)
{
	const struct wdt_counter_config *config = dev->config;
	const struct device *counter = config->counter;

	if ((options & WDT_OPT_PAUSE_IN_SLEEP) || (options & WDT_OPT_PAUSE_IN_SLEEP)) {
		return -ENOTSUP;
	}

	return counter_start(counter);
}

static int wdt_counter_disable(const struct device *dev)
{
	const struct wdt_counter_config *config = dev->config;
	const struct device *counter = config->counter;

	return counter_stop(counter);
}

static void counter_alarm_callback(const struct device *dev,
				   uint8_t chan_id, uint32_t ticks,
				   void *user_data)
{
	const struct device *wdt_dev = user_data;
	struct wdt_counter_data *data = wdt_dev->data;

	counter_stop(dev);
	if (data->callback[chan_id]) {
		data->callback[chan_id](wdt_dev, chan_id);
	}

	if (data->flags[chan_id] & WDT_FLAG_RESET_SOC) {
		LOG_PANIC();
		sys_arch_reboot(0);
	}
}

static int timeout_set(const struct device *dev, int chan_id, bool cancel)
{
	const struct wdt_counter_config *config = dev->config;
	struct wdt_counter_data *data = dev->data;
	const struct device *counter = config->counter;
	struct counter_alarm_cfg alarm_cfg = {
		.callback = counter_alarm_callback,
		.ticks = data->timeout[chan_id],
		.user_data = (void *)dev,
		.flags = 0
	};

	if (cancel) {
		int err = counter_cancel_channel_alarm(counter, chan_id);

		if (err < 0) {
			return err;
		}
	}

	return counter_set_channel_alarm(counter, chan_id, &alarm_cfg);
}

static int wdt_counter_install_timeout(const struct device *dev,
				   const struct wdt_timeout_cfg *cfg)
{
	struct wdt_counter_data *data = dev->data;
	const struct wdt_counter_config *config = dev->config;
	const struct device *counter = config->counter;
	int chan_id;

	if (!device_is_ready(counter)) {
		return -EIO;
	}

	uint32_t max_timeout = counter_get_top_value(counter) -
				counter_get_guard_period(counter,
				COUNTER_GUARD_PERIOD_LATE_TO_SET);
	uint32_t timeout_ticks = counter_us_to_ticks(counter, cfg->window.max * 1000);

	if (cfg->flags & ~WDT_SUPPORTED_CFG_FLAGS) {
		return -ENOTSUP;
	}

	if (cfg->window.min != 0U) {
		return -EINVAL;
	}

	if (timeout_ticks > max_timeout || timeout_ticks == 0) {
		return -EINVAL;
	}

	if (data->alloc_cnt == 0) {
		return -ENOMEM;
	}

	data->alloc_cnt--;
	chan_id = data->alloc_cnt;
	data->timeout[chan_id] = timeout_ticks;
	data->callback[chan_id] = cfg->callback;
	data->flags[chan_id] = cfg->flags;

	int err = timeout_set(dev, chan_id, false);

	if (err < 0) {
		return err;
	}

	return chan_id;
}

static int wdt_counter_feed(const struct device *dev, int chan_id)
{
	const struct wdt_counter_config *config = dev->config;

	if (chan_id > counter_get_num_of_channels(config->counter)) {
		return -EINVAL;
	}

	/* Move alarm further in time. */
	return timeout_set(dev, chan_id, true);
}

static const struct wdt_driver_api wdt_counter_driver_api = {
	.setup = wdt_counter_setup,
	.disable = wdt_counter_disable,
	.install_timeout = wdt_counter_install_timeout,
	.feed = wdt_counter_feed,
};

static const struct wdt_counter_config wdt_counter_config = {
	.counter = DEVICE_DT_GET(DT_PHANDLE(DT_WDT_COUNTER, counter)),
};


static int wdt_counter_init(const struct device *dev)
{
	const struct wdt_counter_config *config = dev->config;
	struct wdt_counter_data *data = dev->data;
	uint8_t ch_cnt = counter_get_num_of_channels(config->counter);

	data->alloc_cnt = MIN(ch_cnt, CONFIG_WDT_COUNTER_CH_COUNT);

	return 0;
}

DEVICE_DT_DEFINE(DT_WDT_COUNTER, wdt_counter_init, NULL,
		 &wdt_data, &wdt_counter_config,
		 POST_KERNEL,
		 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		 &wdt_counter_driver_api);
