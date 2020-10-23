/*
 * Copyright (c) 2020, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <drivers/counter.h>
#include <soc.h>
#include <hw_counter.h>

#define DT_COUNTER_LABEL counter0
#define DRIVER_CONFIG_INFO_FLAGS (COUNTER_CONFIG_INFO_COUNT_UP)
#define DRIVER_CONFIG_INFO_CHANNELS CONFIG_COUNTER_NATIVE_POSIX_CHANNELS_NUMBER
#define COUNTER_NATIVE_POSIX_IRQ_FLAGS (0)
#define COUNTER_NATIVE_POSIX_IRQ_PRIORITY (2)

#define COUNTER_PERIOD (USEC_PER_SEC / CONFIG_COUNTER_NATIVE_POSIX_FREQUENCY)
#define TOP_VALUE (UINT_MAX)

struct counter_alarm {
	struct counter_alarm_cfg config;
	bool active;
};

static struct counter_top_cfg top_cfg;
static struct counter_alarm alarms[DRIVER_CONFIG_INFO_CHANNELS];
static const struct device *device;

static bool any_alarm_active(void)
{
	for (int ch_id = 0; ch_id < DRIVER_CONFIG_INFO_CHANNELS; ch_id++) {
		if (alarms[ch_id].active) {
			return true;
		}
	}
	return false;
}

static void schedule_next_irq(void)
{
	uint32_t next_value = top_cfg.ticks;
	uint32_t current_value = hw_counter_get_value();

	for (int ch_id = 0; ch_id < DRIVER_CONFIG_INFO_CHANNELS; ch_id++) {
		if (alarms[ch_id].active &&
		    alarms[ch_id].config.ticks >= current_value &&
		    alarms[ch_id].config.ticks < next_value) {
			next_value = alarms[ch_id].config.ticks;
		}
	}

	hw_counter_set_target(next_value);
	irq_enable(COUNTER_EVENT_IRQ);
}

static void counter_isr(const void *arg)
{
	ARG_UNUSED(arg);
	uint32_t current_value = hw_counter_get_value();

	for (int ch_id = 0; ch_id < DRIVER_CONFIG_INFO_CHANNELS; ch_id++) {
		if (alarms[ch_id].active &&
		    alarms[ch_id].config.ticks >= current_value) {
			alarms[ch_id].active = false;
			alarms[ch_id].config.callback(
				device, 0, current_value,
				alarms[ch_id].config.user_data);
		}
	}

	if (current_value == top_cfg.ticks) {
		if (top_cfg.callback) {
			top_cfg.callback(device, top_cfg.user_data);
		}
		hw_counter_reset();
	}

	schedule_next_irq();
}

static int ctr_init(const struct device *dev)
{
	device = dev;
	for (int ch_id = 0; ch_id < DRIVER_CONFIG_INFO_CHANNELS; ch_id++) {
		alarms[ch_id].active = false;
	}
	top_cfg.ticks = TOP_VALUE;

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
	schedule_next_irq();
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

	if (any_alarm_active()) {
		return -EBUSY;
	}

	if ((cfg->flags & COUNTER_TOP_CFG_DONT_RESET) &&
	    (cfg->ticks < hw_counter_get_value())) {
		return -ETIME;
	}

	if (cfg->ticks < hw_counter_get_value()) {
		hw_counter_reset();
	}

	top_cfg = *cfg;
	schedule_next_irq();

	return 0;
}

static uint32_t ctr_get_top_value(const struct device *dev)
{
	return top_cfg.ticks;
}

static uint32_t ctr_get_max_relative_alarm(const struct device *dev)
{
	return top_cfg.ticks;
}

static int ctr_set_alarm(const struct device *dev, uint8_t chan_id,
			 const struct counter_alarm_cfg *alarm_cfg)
{
	ARG_UNUSED(dev);

	if (chan_id >= DRIVER_CONFIG_INFO_CHANNELS) {
		posix_print_warning("channel %u is not supported\n", chan_id);
		return -ENOTSUP;
	}

	if (alarms[chan_id].active) {
		posix_print_warning("channel %u is busy\n", chan_id);
		return -EBUSY;
	}

	if (alarm_cfg->ticks > top_cfg.ticks) {
		posix_print_warning(
			"channel %u alarm value exceeds top value (%d)\n",
			chan_id, top_cfg.ticks);
		return -EINVAL;
	}

	alarms[chan_id].config = *alarm_cfg;
	alarms[chan_id].active = true;

	if (!(alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE)) {
		alarms[chan_id].config.ticks =
			hw_counter_get_value() + alarm_cfg->ticks;
		if (alarms[chan_id].config.ticks > top_cfg.ticks) {
			alarms[chan_id].config.ticks =
				alarms[chan_id].config.ticks - top_cfg.ticks;
		}
	}

	schedule_next_irq();
	return 0;
}

static int ctr_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	ARG_UNUSED(dev);

	if (chan_id >= DRIVER_CONFIG_INFO_CHANNELS) {
		posix_print_warning("channel %u is not supported\n", chan_id);
		return -ENOTSUP;
	}

	alarms[chan_id].active = false;
	schedule_next_irq();

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
	.get_max_relative_alarm = ctr_get_max_relative_alarm,
};

static const struct counter_config_info ctr_config = {
	.max_top_value = UINT_MAX,
	.freq = CONFIG_COUNTER_NATIVE_POSIX_FREQUENCY,
	.channels = DRIVER_CONFIG_INFO_CHANNELS,
	.flags = DRIVER_CONFIG_INFO_FLAGS
};

DEVICE_AND_API_INIT(posix_rtc0, DT_LABEL(DT_NODELABEL(DT_COUNTER_LABEL)),
		    &ctr_init, NULL, &ctr_config, PRE_KERNEL_1,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &ctr_api);
