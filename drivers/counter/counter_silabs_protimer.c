/*
 * Copyright 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_protimer

#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <soc.h>
#include <zephyr/drivers/counter.h>
#include "sl_rail.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(counter_silabs_protimer, CONFIG_COUNTER_LOG_LEVEL);

#define DT_RTC DT_COMPAT_GET_ANY_STATUS_OKAY(counter_silabs_protimer)

#define RAILTIMER_ALARM_NUM  1
/* RAIL timer has fixed 32-bit wrap; only top value UINT32_MAX is supported for overflow */
#define RAIL_TIMER_TOP_VALUE UINT32_MAX
/* RAIL time is in microseconds -> 1 tick = 1 us */
#define RAIL_TIMER_FREQ_HZ   1000000

struct counter_silabs_rail_config {
	struct counter_config_info info;
	void (*irq_config)(void);
	uint8_t counter_size;
};

typedef struct counter_silabs_alarm_data counter_silabs_alarm_data;

struct counter_silabs_alarm_data {
	counter_alarm_callback_t callback;
	uint8_t chan_id;
	uint32_t ticks;
	const struct device *dev;
	const void *user_data;
};

struct counter_silabs_top_data {
	counter_top_callback_t callback;
	uint32_t ticks;
	const struct device *dev;
	const void *user_data;
};

struct counter_silabs_data {
	struct counter_silabs_alarm_data alarm[RAILTIMER_ALARM_NUM];
	struct counter_silabs_top_data top_data;
	sl_rail_multi_timer_t alarm_timer[RAILTIMER_ALARM_NUM];
	sl_rail_multi_timer_t top_timer;
};

static void top_callback(sl_rail_multi_timer_t *handle, void *data)
{
	struct counter_silabs_top_data *top_data = (struct counter_silabs_top_data *)data;
	struct counter_silabs_data *dev_data = (struct counter_silabs_data *)(top_data->dev)->data;

	if (top_data->callback != NULL) {
		top_data->callback(
			top_data->dev,
			((const struct counter_top_cfg *)top_data->user_data)
				->user_data);
		/* Re-arm overflow at next occurrence of max value (absolute UINT32_MAX) */
		sl_rail_set_multi_timer(SL_RAIL_EFR32_HANDLE, &dev_data->top_timer,
					RAIL_TIMER_TOP_VALUE, SL_RAIL_TIME_ABSOLUTE,
					(sl_rail_multi_timer_callback_t)top_callback,
					(void *)top_data);
	}
}

static void us_alarm_callback(struct sl_rail_multi_timer *handle, void *data)
{
	struct counter_silabs_alarm_data *alarm_data = (struct counter_silabs_alarm_data *)data;
	/* RAIL time is us, counter ticks are 1:1 us */
	uint32_t count = sl_rail_get_time(SL_RAIL_EFR32_HANDLE);

	if (alarm_data->callback != NULL) {
		alarm_data->callback(
			alarm_data->dev, alarm_data->chan_id, count,
			((const struct counter_alarm_cfg *)alarm_data->user_data)->user_data);
	}
}

static int counter_silabs_rail_start(const struct device *dev)
{
	struct counter_silabs_data *const data = (struct counter_silabs_data *const)dev->data;

	bool is_top_timer_running =
		sl_rail_is_multi_timer_running(SL_RAIL_EFR32_HANDLE, &data->top_timer);
	if (!is_top_timer_running && data->top_data.callback != NULL) {
		/* Overflow interrupt: absolute timer at max value (UINT32_MAX) */
		sl_rail_status_t rail_status = sl_rail_set_multi_timer(
			SL_RAIL_EFR32_HANDLE, &data->top_timer, RAIL_TIMER_TOP_VALUE,
			SL_RAIL_TIME_ABSOLUTE, (sl_rail_multi_timer_callback_t)top_callback,
			(void *)&data->top_data);

		return (rail_status == 0) ? 0 : -EIO;
	}
	return 0;
}

static int counter_silabs_rail_stop(const struct device *dev)
{
	ARG_UNUSED(dev);
	/* RAIL timer cannot be stopped; stopping would also require cancelling active alarms. */
	return -ENOTSUP;
}

static int counter_silabs_rail_get_value(const struct device *dev, uint32_t *ticks)
{
	/* RAIL time is us, counter ticks are 1:1 us */
	*ticks = sl_rail_get_time(SL_RAIL_EFR32_HANDLE);
	return 0;
}

static uint32_t counter_silabs_rail_get_pending_int(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static uint32_t counter_silabs_rail_get_top_value(const struct device *dev)
{
	ARG_UNUSED(dev);
	return RAIL_TIMER_TOP_VALUE;
}

static int counter_silabs_rail_set_alarm(const struct device *dev, uint8_t chan_id,
					 const struct counter_alarm_cfg *alarm_cfg)
{
	struct counter_silabs_data *const dev_data = (struct counter_silabs_data *const)(dev)->data;
	sl_rail_time_mode_t time_mode = ((alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) != 0)
						? SL_RAIL_TIME_ABSOLUTE
						: SL_RAIL_TIME_DELAY;
	uint32_t time_value = alarm_cfg->ticks;

	if (alarm_cfg->ticks > RAIL_TIMER_TOP_VALUE) {
		return -EINVAL;
	}

	if (chan_id >= RAILTIMER_ALARM_NUM) {
		printk("Alarm timer count exceeded\n");
		return -EINVAL;
	}
	bool is_alarm_timer_running = sl_rail_is_multi_timer_running(
		SL_RAIL_EFR32_HANDLE, &dev_data->alarm_timer[chan_id]);
	if (is_alarm_timer_running) {
		sl_rail_cancel_multi_timer(SL_RAIL_EFR32_HANDLE, &dev_data->alarm_timer[chan_id]);
	}

	dev_data->alarm[chan_id].callback = alarm_cfg->callback;
	dev_data->alarm[chan_id].chan_id = chan_id;
	dev_data->alarm[chan_id].dev = dev;
	dev_data->alarm[chan_id].user_data = alarm_cfg;
	dev_data->alarm[chan_id].ticks = alarm_cfg->ticks;

	sl_rail_status_t rail_status = sl_rail_set_multi_timer(
		SL_RAIL_EFR32_HANDLE, &dev_data->alarm_timer[chan_id], time_value, time_mode,
		(sl_rail_multi_timer_callback_t)us_alarm_callback,
		(void *)&dev_data->alarm[chan_id]);
	return (rail_status == 0) ? 0 : -EIO;
}

static int counter_silabs_rail_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	struct counter_silabs_data *const dev_data = (struct counter_silabs_data *const)(dev)->data;

	if (chan_id >= RAILTIMER_ALARM_NUM) {
		LOG_DBG("Alarm timer count exceeded\n");
		return -EINVAL;
	}

	sl_rail_status_t rail_status =
		sl_rail_cancel_multi_timer(SL_RAIL_EFR32_HANDLE, &dev_data->alarm_timer[chan_id]);

	dev_data->alarm[chan_id].callback = NULL;
	dev_data->alarm[chan_id].user_data = NULL;

	LOG_DBG("cancel alarm: channel %u", chan_id);
	return (rail_status == 0) ? 0 : -EIO;
}

static int counter_silabs_rail_set_top_value(const struct device *dev,
					     const struct counter_top_cfg *cfg)
{
	struct counter_silabs_data *const dev_data = (struct counter_silabs_data *const)(dev)->data;

	/* Only top value UINT32_MAX is supported (overflow interrupt via absolute timer at max) */
	if (cfg->ticks != RAIL_TIMER_TOP_VALUE) {
		return -ENOTSUP;
	}
	/* RAIL API does not support counter reset to 0; only DONT_RESET (wrap) is supported */
	if ((cfg->flags & COUNTER_TOP_CFG_DONT_RESET) == 0) {
		return -ENOTSUP;
	}

	bool is_top_timer_running =
		sl_rail_is_multi_timer_running(SL_RAIL_EFR32_HANDLE, &dev_data->top_timer);
	if (is_top_timer_running) {
		sl_rail_cancel_multi_timer(SL_RAIL_EFR32_HANDLE, &dev_data->top_timer);
	}

	dev_data->top_data.callback = cfg->callback;
	dev_data->top_data.ticks = cfg->ticks;
	dev_data->top_data.dev = dev;
	dev_data->top_data.user_data = cfg;

	if (cfg->callback != NULL) {
		sl_rail_status_t rail_status = sl_rail_set_multi_timer(
			SL_RAIL_EFR32_HANDLE, &dev_data->top_timer, cfg->ticks,
			SL_RAIL_TIME_ABSOLUTE, (sl_rail_multi_timer_callback_t)top_callback,
			(void *)&dev_data->top_data);
		return (rail_status == 0) ? 0 : -EIO;
	}
	return 0;
}

static int counter_silabs_rail_init(const struct device *dev)
{
	const struct counter_silabs_rail_config *cfg =
		(const struct counter_silabs_rail_config *const)dev->config;
	struct counter_silabs_data *data = (struct counter_silabs_data *)dev->data;

	cfg->irq_config();
	memset(&data->top_timer, 0, sizeof(data->top_timer));
	memset(&data->alarm_timer, 0, sizeof(data->alarm_timer));

	LOG_INF("Device %s initialized", (dev)->name);
	return 0;
}

static DEVICE_API(counter, counter_silabs_rail_api) = {
	.start = counter_silabs_rail_start,
	.stop = counter_silabs_rail_stop,
	.get_value = counter_silabs_rail_get_value,
	.set_alarm = counter_silabs_rail_set_alarm,
	.cancel_alarm = counter_silabs_rail_cancel_alarm,
	.set_top_value = counter_silabs_rail_set_top_value,
	.get_pending_int = counter_silabs_rail_get_pending_int,
	.get_top_value = counter_silabs_rail_get_top_value,
};

static void counter_silabs_rail_0_irq_config(void)
{
	/* No IRQ configuration needed, as rail interrupts are handled internally*/
}

static const struct counter_silabs_rail_config counter_silabs_rail_0_config = {
	.info = {
		.freq = RAIL_TIMER_FREQ_HZ,
		.max_top_value = RAIL_TIMER_TOP_VALUE,
		.flags = COUNTER_CONFIG_INFO_COUNT_UP,
		.channels = RAILTIMER_ALARM_NUM,
	},
	.irq_config = counter_silabs_rail_0_irq_config,
};

static struct counter_silabs_data counter_silabs_rail_0_data;

DEVICE_DT_INST_DEFINE(0, counter_silabs_rail_init, NULL, &counter_silabs_rail_0_data,
		      &counter_silabs_rail_0_config, POST_KERNEL,
		      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &counter_silabs_rail_api);
