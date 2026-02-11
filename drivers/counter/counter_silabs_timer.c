/*
 * Copyright (c) 2021, Phuc Hoang <donp172748@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zephyr/drivers/regulator.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_silabs.h>

#include <sl_hal_timer.h>

LOG_MODULE_REGISTER(silabs_timer_counter, CONFIG_COUNTER_LOG_LEVEL);

#define DT_DRV_COMPAT silabs_timer_counter

struct counter_silabs_timer_config {
	struct counter_config_info info;
	const struct device *clock_dev;
	const struct silabs_clock_control_cmu_config clock_cfg;
	uint16_t clock_div;
	void (*irq_config_func)(void);
	TIMER_TypeDef *timer; /* Reference pointer to counter device */
};

struct counter_silabs_timer_data {
	struct counter_top_cfg top;
	struct counter_alarm_cfg *alarms;
};

/* Helper func */

static inline bool is_counter_running(const struct device *dev)
{
	const struct counter_silabs_timer_config *config = dev->config;

	return (sl_hal_timer_get_status(config->timer) & TIMER_STATUS_RUNNING) ? true : false;
}

static inline uint32_t get_IEN_flag(uint8_t chan_id)
{
	return BIT(_TIMER_IEN_CC0_SHIFT + chan_id);
}

static inline uint32_t get_IF_flag(uint8_t chan_id)
{
	return BIT(_TIMER_IF_CC0_SHIFT + chan_id);
}

/* Driver APIs */
static int counter_silabs_timer_start(const struct device *dev)
{
	const struct counter_silabs_timer_config *config = dev->config;

	if (!is_counter_running(dev)) {
		sl_hal_timer_start(config->timer);
	}

	return 0;
}

static int counter_silabs_timer_stop(const struct device *dev)
{
	const struct counter_silabs_timer_config *config = dev->config;

	if (is_counter_running(dev)) {
		sl_hal_timer_stop(config->timer);
	}

	return 0;
}


static int counter_silabs_timer_reset(const struct device *dev)
{
	const struct counter_silabs_timer_config *config = dev->config;

	if (is_counter_running(dev)) {
		sl_hal_timer_set_counter(config->timer, 0);
	}

	return 0;
}

static int counter_silabs_timer_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct counter_silabs_timer_config *config = dev->config;

	*ticks = sl_hal_timer_get_counter(config->timer);

	return 0;
}

static int counter_silabs_timer_set_value(const struct device *dev, uint32_t ticks)
{
	const struct counter_silabs_timer_config *config = dev->config;

	uint32_t top_val = sl_hal_timer_get_top(config->timer);

	if (ticks > top_val) {
		return -EINVAL;
	}

	sl_hal_timer_set_counter(config->timer, ticks);

	return 0;
}

static uint32_t counter_silabs_timer_get_top_value(const struct device *dev)
{
	const struct counter_silabs_timer_config *config = dev->config;

	return sl_hal_timer_get_top(config->timer);
}

static int counter_silabs_timer_set_top_value(const struct device *dev,
	const struct counter_top_cfg *cfg)
{
	if (is_counter_running(dev)) {
		return -EBUSY;
	}

	struct counter_silabs_timer_data *data = dev->data;
	const struct counter_silabs_timer_config *config = dev->config;

	uint32_t now_ticks = sl_hal_timer_get_counter(config->timer);

	data->top.ticks = cfg->ticks;
	data->top.callback = cfg->callback;
	data->top.user_data = cfg->user_data;
	data->top.flags = cfg->flags;

	sl_hal_timer_set_top(config->timer, cfg->ticks);

	if (!(cfg->flags & COUNTER_TOP_CFG_DONT_RESET) &&
		!(cfg->flags & COUNTER_TOP_CFG_RESET_WHEN_LATE)) {
		sl_hal_timer_set_counter(config->timer, 0);
	} else {
		if ((cfg->flags & COUNTER_TOP_CFG_DONT_RESET) &&
					(now_ticks > cfg->ticks)) {
			return -ETIME;
		}
		if ((cfg->flags & COUNTER_TOP_CFG_RESET_WHEN_LATE) &&
					(now_ticks > cfg->ticks)) {
			sl_hal_timer_set_counter(config->timer, 0);
		}
	}

	sl_hal_timer_enable_interrupts(config->timer, TIMER_IF_OF);

	return 0;
}

static int counter_silabs_timer_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	const struct counter_silabs_timer_config *config = dev->config;

	if (chan_id >= config->info.channels) {
		return -EINVAL;
	}

	sl_hal_timer_disable_interrupts(config->timer, get_IEN_flag(chan_id));
	sl_hal_timer_clear_interrupts(config->timer, get_IF_flag(chan_id));
	sl_hal_timer_channel_set_compare(config->timer, chan_id, 0);

	return 0;
}

static int counter_silabs_timer_set_alarm(const struct device *dev, uint8_t chan_id,
				const struct counter_alarm_cfg *alarm_cfg)
{
	struct counter_silabs_timer_data *data = dev->data;
	const struct counter_silabs_timer_config *config = dev->config;

	uint32_t top_val = sl_hal_timer_get_top(config->timer);

	if (chan_id >= config->info.channels) {
		return -EINVAL;
	}
	if (alarm_cfg->ticks > top_val) {
		return -EINVAL;
	}
	/* Cancel current channel alarm */
	counter_silabs_timer_cancel_alarm(dev, chan_id);

	if (alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) {
		/* Absolute */
		sl_hal_timer_channel_set_compare(config->timer, chan_id,
						alarm_cfg->ticks);
	} else {
		/* Relative */
		uint32_t now_ticks = sl_hal_timer_get_counter(config->timer);

		sl_hal_timer_channel_set_compare(config->timer, chan_id,
						(now_ticks + alarm_cfg->ticks) % top_val);
	}

	/* Update alarm metadata */
	data->alarms[chan_id].ticks = alarm_cfg->ticks;
	data->alarms[chan_id].callback = alarm_cfg->callback;
	data->alarms[chan_id].user_data = alarm_cfg->user_data;
	data->alarms[chan_id].flags = alarm_cfg->flags;

	/* Re-enable interrupt channel */
	sl_hal_timer_enable_interrupts(config->timer, get_IEN_flag(chan_id));

	return 0;
}

static uint32_t counter_silabs_timer_get_pending_int(const struct device *dev)
{
	const struct counter_silabs_timer_config *config = dev->config;

	return sl_hal_timer_get_pending_interrupts(config->timer);
}

static uint32_t counter_silabs_timer_get_freq(const struct device *dev)
{
	const struct counter_silabs_timer_config *config = dev->config;
	const struct device *clock_dev = config->clock_dev;
	uint32_t freq_hz = 0;

	if (clock_control_get_rate(clock_dev,
		(clock_control_subsys_t)(uintptr_t)&config->clock_cfg, &freq_hz)) {
		return 0;
	}

	return freq_hz / config->clock_div;
}

static void counter_silabs_timer_isr(const struct device *dev)
{
	struct counter_silabs_timer_data *data = dev->data;
	const struct counter_silabs_timer_config *config = dev->config;
	uint32_t now_ticks;
	uint32_t pendings = sl_hal_timer_get_pending_interrupts(config->timer);

	counter_silabs_timer_get_value(dev, &now_ticks);

	/* Handle top timer expired */
	if (pendings & TIMER_IF_OF) {
		sl_hal_timer_clear_interrupts(config->timer, TIMER_IF_OF);
		if (data->top.callback) {
			data->top.callback(dev, data->top.user_data);
		}
	}

	for (uint8_t idx = 0; idx < config->info.channels; idx++) {
		/* Check which channel is fired */
		if (pendings & get_IF_flag(idx)) {
			/* Alarm fire only one time, not repeated */
			counter_silabs_timer_cancel_alarm(dev, idx);

			if (data->alarms[idx].callback) {
				data->alarms[idx].callback(dev, idx, now_ticks,
								data->alarms[idx].user_data);
			}
		}
	}
}

static DEVICE_API(counter, counter_silabs_timer_driver_api) = {
	.start = counter_silabs_timer_start,
	.stop = counter_silabs_timer_stop,
	.reset = counter_silabs_timer_reset,
	.set_value = counter_silabs_timer_set_value,
	.get_value = counter_silabs_timer_get_value,
	.set_alarm = counter_silabs_timer_set_alarm,
	.cancel_alarm = counter_silabs_timer_cancel_alarm,
	.set_top_value = counter_silabs_timer_set_top_value,
	.get_pending_int = counter_silabs_timer_get_pending_int,
	.get_top_value = counter_silabs_timer_get_top_value,
	.get_freq = counter_silabs_timer_get_freq,
};

static int counter_silabs_timer_init(const struct device *dev)
{
	const struct counter_silabs_timer_config *config = dev->config;
	int err;

	/* Clock setting */
	err = clock_control_on(config->clock_dev,
		(clock_control_subsys_t)(uintptr_t)&config->clock_cfg);
	if (err < 0 && err != -EALREADY) {
		return err;
	}

	/* Init counter */
	sl_hal_timer_init_t init = SL_HAL_TIMER_INIT_DEFAULT;

	init.prescaler = config->clock_div - 1;

	sl_hal_timer_init(config->timer, &init);
	sl_hal_timer_enable(config->timer);
	sl_hal_timer_set_top(config->timer, config->info.max_top_value);
	sl_hal_timer_wait_sync(config->timer);

	/* Init alarm */
	sl_hal_timer_channel_init_t channel_init = SL_HAL_TIMER_CHANNEL_INIT_DEFAULT;

	channel_init.channel_mode = SL_HAL_TIMER_CHANNEL_MODE_COMPARE;

	for (uint8_t idx = 0; idx < config->info.channels; idx++) {
		sl_hal_timer_channel_init(config->timer, idx, &channel_init);
	}

	/* Interrupt setting */
	config->irq_config_func();
	return 0;
}

#define COUNTER_SILABS_TIMER_DEVICE_INIT(idx)                                                     \
	                                                                                          \
	static void counter_silabs_timer_irq_config_##idx(void)                                   \
	{                                                                                         \
		IRQ_CONNECT(DT_IRQN(DT_INST_PARENT(idx)),                                         \
					DT_IRQ(DT_INST_PARENT(idx), priority),                    \
					counter_silabs_timer_isr,                                 \
					DEVICE_DT_INST_GET(idx), 0);                              \
		irq_enable(DT_IRQN(DT_INST_PARENT(idx)));                                         \
	}                                                                                         \
	                                                                                          \
	static const struct counter_silabs_timer_config counter_silabs_timer_config_##idx = {     \
		.info = {                                                                         \
			.max_top_value = GENMASK(DT_PROP(                                         \
						DT_INST_PARENT(idx), counter_size) - 1, 0),       \
			.flags = COUNTER_CONFIG_INFO_COUNT_UP,                                    \
			.channels = DT_PROP(DT_INST_PARENT(idx), channels),                       \
		},                                                                                \
		.clock_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR(DT_PARENT(DT_DRV_INST(idx)))),          \
		.clock_cfg = SILABS_DT_CLOCK_CFG(DT_INST_PARENT(idx)),                            \
		.clock_div = DT_PROP(DT_INST_PARENT(idx), clock_div),                             \
		.irq_config_func = counter_silabs_timer_irq_config_##idx,                         \
		.timer = (TIMER_TypeDef *)DT_REG_ADDR(DT_INST_PARENT(idx)),                       \
	};                                                                                        \
	                                                                                          \
	static struct counter_alarm_cfg counter_alarm_data_##idx[DT_PROP(                         \
						DT_INST_PARENT(idx), channels)];                  \
	static struct counter_silabs_timer_data counter_silabs_timer_data_##idx = {               \
		.alarms = counter_alarm_data_##idx,                                               \
	};                                                                                        \
	                                                                                          \
	DEVICE_DT_INST_DEFINE(idx,                                                                \
		counter_silabs_timer_init, NULL,                                                  \
		&counter_silabs_timer_data_##idx,                                                 \
		&counter_silabs_timer_config_##idx,                                               \
		POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,                                  \
		&counter_silabs_timer_driver_api);                                                \

DT_INST_FOREACH_STATUS_OKAY(COUNTER_SILABS_TIMER_DEVICE_INIT)
