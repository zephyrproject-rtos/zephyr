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

#define RAILTIMER_ALARM_NUM 1
#define RAILTIMER_MAX_VALUE 0xFFFFFFFFUL

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
	struct device *dev;
	void *user_data;
    int mFiredCount;
};

struct counter_silabs_top_data {
	counter_top_callback_t callback;
	uint32_t ticks;
	struct device *dev;
	void *user_data;
};

struct counter_silabs_data {
	struct counter_silabs_alarm_data alarm[RAILTIMER_ALARM_NUM];
	struct counter_silabs_top_data top_data;
};

static sl_rail_multi_timer_t alarm_timer[RAILTIMER_ALARM_NUM];
static sl_rail_multi_timer_t top_timer;

static void top_callback(sl_rail_multi_timer_t *handle, void *data)
{
	struct counter_silabs_top_data *top_data = (struct counter_silabs_top_data *)data;

	if (top_data->callback != NULL) {
		top_data->callback(top_data->dev,
				   ((struct counter_top_cfg *)(top_data->user_data))->user_data);
	}
}

void us_alarm_callback(struct sl_rail_multi_timer *handle, void *data)
{
    struct counter_silabs_alarm_data *alarm_data = (struct counter_silabs_alarm_data *)data;
	uint32_t count =
		((counter_us_to_ticks(alarm_data->dev, sl_rail_get_time(SL_RAIL_EFR32_HANDLE))) %
		 (((struct counter_silabs_data *const)(alarm_data->dev)->data)->top_data.ticks));

	if (alarm_data->callback != NULL) {
		alarm_data->callback(
			alarm_data->dev, alarm_data->chan_id, count,
			((struct counter_alarm_cfg *)(alarm_data->user_data))->user_data);
	}
}

static int counter_silabs_rail_start(const struct device *dev)
{
    ARG_UNUSED(dev);
	struct counter_silabs_data *const data = (struct counter_silabs_data *const)dev->data;
    const struct counter_silabs_rail_config *cfg = dev->config;
    bool is_top_timer_running = false;
    is_top_timer_running = sl_rail_is_multi_timer_running(SL_RAIL_EFR32_HANDLE, &top_timer);
    if (!is_top_timer_running)
    {
       sl_rail_status_t rail_status = sl_rail_set_multi_timer(SL_RAIL_EFR32_HANDLE,
                                                      &top_timer,
                                                      counter_ticks_to_us(dev, data->top_data.ticks),
                                                      SL_RAIL_TIME_DELAY,
                                                      (sl_rail_multi_timer_callback_t) top_callback,
                                                      (void *)&data->top_data);
        return rail_status;
    }
    return 0;
}

static int counter_silabs_rail_stop(const struct device *dev)
{
    ARG_UNUSED(dev);

    bool is_top_timer_running = false;
    is_top_timer_running = sl_rail_is_multi_timer_running(SL_RAIL_EFR32_HANDLE, &top_timer);
    if (is_top_timer_running)
    {
        sl_rail_status_t rail_status = sl_rail_cancel_multi_timer(SL_RAIL_EFR32_HANDLE, &top_timer);
        return rail_status;
    }
    return 0;
}

static int counter_silabs_rail_get_value(const struct device *dev, uint32_t *ticks)
{
    struct counter_silabs_data *const dev_data = (struct counter_silabs_data *const)(dev)->data;
    *ticks = counter_us_to_ticks(dev, (sl_rail_get_time(SL_RAIL_EFR32_HANDLE)));// % dev_data->top_data.ticks;
    return 0;
}

static uint32_t counter_silabs_rail_get_pending_int(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static uint32_t counter_silabs_rail_get_top_value(const struct device *dev)
{
	struct counter_silabs_data *const dev_data = (struct counter_silabs_data *const)(dev)->data;

	return dev_data->top_data.ticks;
}

static int counter_silabs_rail_set_alarm(const struct device *dev, uint8_t chan_id,
				     const struct counter_alarm_cfg *alarm_cfg)
{
	ARG_UNUSED(dev);
    bool is_alarm_timer_running = false;
    struct counter_silabs_data *const dev_data = (struct counter_silabs_data *const)(dev)->data;
	uint32_t now_ticks = 0;
	uint32_t top_val = counter_silabs_rail_get_top_value(dev);

	if ((top_val != 0) && (alarm_cfg->ticks > top_val)) {
		return -EINVAL;
	}

	if (chan_id >= RAILTIMER_ALARM_NUM) {
		printk("Alarm timer count exceeded\n");
		return -EINVAL;
	}
    is_alarm_timer_running = sl_rail_is_multi_timer_running(SL_RAIL_EFR32_HANDLE, &alarm_timer[chan_id]);
    if (is_alarm_timer_running) {
        sl_rail_status_t rail_status = sl_rail_cancel_multi_timer(SL_RAIL_EFR32_HANDLE, &alarm_timer[chan_id]);
    }

    if ((alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) != 0) {
		/* Absolute */
		(void)counter_silabs_rail_get_value(dev, &now_ticks);
		if (now_ticks < alarm_cfg->ticks) {
			dev_data->alarm[chan_id].ticks = top_val + (alarm_cfg->ticks - now_ticks);
		} else {
			dev_data->alarm[chan_id].ticks =
				(top_val - (now_ticks - alarm_cfg->ticks)) % top_val;
		}
	} else {
		/* Relative */
		dev_data->alarm[chan_id].ticks = alarm_cfg->ticks;
	}

	dev_data->alarm[chan_id].callback = alarm_cfg->callback;
	dev_data->alarm[chan_id].chan_id = chan_id;
	dev_data->alarm[chan_id].dev = (struct device *)dev;
	dev_data->alarm[chan_id].user_data = (struct counter_alarm_cfg *)alarm_cfg;

    sl_rail_status_t rail_status = sl_rail_set_multi_timer(SL_RAIL_EFR32_HANDLE,
                                                      &alarm_timer[chan_id],
                                                      counter_ticks_to_us(dev, dev_data->alarm[chan_id].ticks),
                                                      SL_RAIL_TIME_DELAY,
                                                      (sl_rail_multi_timer_callback_t)us_alarm_callback,
                                                      (void *)&dev_data->alarm[chan_id]);
	return 0;
}

static int counter_silabs_rail_cancel_alarm(const struct device *dev, uint8_t chan_id)
{

    struct counter_silabs_data *const dev_data = (struct counter_silabs_data *const)(dev)->data;

	if (chan_id >= RAILTIMER_ALARM_NUM) {
		LOG_DBG("Alarm timer count exceeded\n");
		return -EINVAL;
	}

    sl_rail_status_t rail_status = sl_rail_cancel_multi_timer(SL_RAIL_EFR32_HANDLE, &alarm_timer[chan_id]);

	dev_data->alarm[chan_id].callback = NULL;
	dev_data->alarm[chan_id].user_data = NULL;

	LOG_DBG("cancel alarm: channel %u", chan_id);
	return 0;
}

static int counter_silabs_rail_set_top_value(const struct device *dev, const struct counter_top_cfg *cfg)
{
	struct counter_silabs_data *const dev_data = (struct counter_silabs_data *const)(dev)->data;
	bool is_top_timer_running = false;

	sl_rail_status_t rail_status = sl_rail_is_multi_timer_running(SL_RAIL_EFR32_HANDLE, &top_timer);
	if (is_top_timer_running) {
		sl_rail_cancel_multi_timer(SL_RAIL_EFR32_HANDLE, &top_timer);
	}

	dev_data->top_data.callback = cfg->callback;
	dev_data->top_data.ticks = cfg->ticks;
	dev_data->top_data.dev = (struct device *)dev;
	dev_data->top_data.user_data = (struct counter_top_cfg *)cfg;
	return 0;
}

static int counter_silabs_rail_init(const struct device *dev)
{
    const struct counter_silabs_rail_config *cfg = (const struct counter_silabs_rail_config *const)dev->config;
	struct counter_silabs_data *data = (struct counter_silabs_data *const)dev->data;

    /* Avoid reconfiguring of IRQs */
    cfg->irq_config();

    //data->ticks = 0;
    data->top_data.ticks = UINT32_MAX;
    //data->alarm.mFiredCount = 0;
    memset(&top_timer, 0, sizeof(top_timer));
    memset(&alarm_timer, 0, sizeof(alarm_timer));

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
	#define RADIO_IRQN(name)     DT_IRQ_BY_NAME(DT_NODELABEL(radio), name, irq)
	irq_enable(RADIO_IRQN(protimer));
}

static const struct counter_silabs_rail_config counter_silabs_rail_0_config = {
	.info = {
			.max_top_value = UINT32_MAX,
			.flags = COUNTER_CONFIG_INFO_COUNT_UP,
			.channels = RAILTIMER_ALARM_NUM,
		},
	.irq_config = counter_silabs_rail_0_irq_config,
};

static struct counter_silabs_data counter_silabs_rail_0_data;

DEVICE_DT_INST_DEFINE(0, counter_silabs_rail_init, NULL, &counter_silabs_rail_0_data, &counter_silabs_rail_0_config,
		      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &counter_silabs_rail_api);