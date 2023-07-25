/*
 * Copyright (c) 2020, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_native_posix_counter

#include <zephyr/device.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/irq.h>
#include <soc.h>
#include <hw_counter.h>
#include <limits.h>

#define DRIVER_CONFIG_INFO_FLAGS (COUNTER_CONFIG_INFO_COUNT_UP)
#define DRIVER_CONFIG_INFO_CHANNELS 1
#define COUNTER_NATIVE_POSIX_IRQ_FLAGS (0)
#define COUNTER_NATIVE_POSIX_IRQ_PRIORITY (2)

#define COUNTER_PERIOD (USEC_PER_SEC / CONFIG_COUNTER_NATIVE_POSIX_FREQUENCY)
#define TOP_VALUE (UINT_MAX)

static struct counter_alarm_cfg pending_alarm;
static bool is_alarm_pending;
static const struct device *device;

static void counter_isr(const void *arg)
{
	ARG_UNUSED(arg);
	uint32_t current_value = hw_counter_get_value();

	if (is_alarm_pending) {
		is_alarm_pending = false;
		pending_alarm.callback(device, 0, current_value,
				       pending_alarm.user_data);
	}
}

static int ctr_init(const struct device *dev)
{
	device = dev;
	is_alarm_pending = false;

	IRQ_CONNECT(COUNTER_EVENT_IRQ, COUNTER_NATIVE_POSIX_IRQ_PRIORITY,
		    counter_isr, NULL, COUNTER_NATIVE_POSIX_IRQ_FLAGS);
	hw_counter_set_period(COUNTER_PERIOD);
	hw_counter_set_target(TOP_VALUE);

	return 0;
}

static int ctr_start(const struct device *dev)
{
	ARG_UNUSED(dev);

	hw_counter_start();
	return 0;
}

static int ctr_stop(const struct device *dev)
{
	ARG_UNUSED(dev);

	hw_counter_stop();
	return 0;
}

static int ctr_get_value(const struct device *dev, uint32_t *ticks)
{
	ARG_UNUSED(dev);

	*ticks = hw_counter_get_value();
	return 0;
}

static uint32_t ctr_get_pending_int(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

static int ctr_set_top_value(const struct device *dev,
			     const struct counter_top_cfg *cfg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cfg);

	posix_print_warning("%s not supported\n", __func__);
	return -ENOTSUP;
}

static uint32_t ctr_get_top_value(const struct device *dev)
{
	return TOP_VALUE;
}

static int ctr_set_alarm(const struct device *dev, uint8_t chan_id,
			 const struct counter_alarm_cfg *alarm_cfg)
{
	ARG_UNUSED(dev);

	if (chan_id >= DRIVER_CONFIG_INFO_CHANNELS) {
		posix_print_warning("channel %u is not supported\n", chan_id);
		return -ENOTSUP;
	}

	pending_alarm = *alarm_cfg;
	is_alarm_pending = true;

	if (!(alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE)) {
		pending_alarm.ticks =
			hw_counter_get_value() + pending_alarm.ticks;
	}

	hw_counter_set_target(pending_alarm.ticks);
	irq_enable(COUNTER_EVENT_IRQ);

	return 0;
}

static int ctr_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	ARG_UNUSED(dev);

	if (chan_id >= DRIVER_CONFIG_INFO_CHANNELS) {
		posix_print_warning("channel %u is not supported\n", chan_id);
		return -ENOTSUP;
	}

	is_alarm_pending = false;

	return 0;
}

static const struct counter_driver_api ctr_api = {
	.start = ctr_start,
	.stop = ctr_stop,
	.get_value = ctr_get_value,
	.set_alarm = ctr_set_alarm,
	.cancel_alarm = ctr_cancel_alarm,
	.set_top_value = ctr_set_top_value,
	.get_pending_int = ctr_get_pending_int,
	.get_top_value = ctr_get_top_value,
};

static const struct counter_config_info ctr_config = {
	.max_top_value = UINT_MAX,
	.freq = CONFIG_COUNTER_NATIVE_POSIX_FREQUENCY,
	.channels = DRIVER_CONFIG_INFO_CHANNELS,
	.flags = DRIVER_CONFIG_INFO_FLAGS
};

DEVICE_DT_INST_DEFINE(0, ctr_init,
		    NULL, NULL, &ctr_config, PRE_KERNEL_1,
		    CONFIG_COUNTER_INIT_PRIORITY, &ctr_api);
