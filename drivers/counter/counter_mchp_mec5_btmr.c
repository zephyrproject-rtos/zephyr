/*
 * Copyright (c) 2024 Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_mec5_btmr_counter

/**
 * @file
 * @brief Microchip MEC5 HAL basic timer counter driver
 *
 * Basic timers implement either 16-bit or 32-bit up/down counter with
 * an auto-reload mode when the counter reaches its terminal count.
 * Count down mode:
 *   Software loads the count register with the starting value.
 *   When the counter reaches zero it sets the event status flag which
 *   can fire an interrupt if enabled.  If auto-reload is enabled the
 *   and it reaches zero, the value in the preload register is copied
 *   into the count register and counting down continues.
 *
 * Count up mode:
 *   Counts up from the initial value in the count register to the maximum
 *   value (0xffff or 0xffffffff). Once the terminal value is reached the
 *   event status bit is set and if interrupt is enabled an interrupt is
 *   generated. If auto-reload is enabled, hardware loads the value from the
 *   preload register into the count register and counting up continues.
 *
 * Driver does not implement count up mode support.
 */


#include <soc.h>
#include <errno.h>
#include <stdbool.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(counter_mec5_btmr, CONFIG_COUNTER_LOG_LEVEL);

#include <device_mec5.h>
#include <mec_btimer_api.h>

struct cntr_mec5_btmr_devcfg {
	struct counter_config_info info;
	void (*irq_cfg_func)(void);
	struct btmr_regs *regs;
	uint16_t prescaler;
};

struct cntr_mec5_btmr_dev_data {
	uint32_t isr_count;
	uint32_t top_ticks;
	counter_alarm_callback_t alarm_cb;
	void *alarm_cb_ud;
	counter_top_callback_t top_cb;
	void *top_cb_ud;
};

/* API - Start counter device in free running mode
 * Basic timer does not have a free-running counter.
 * Also, the application may have called set alarm or set top
 * which both modify the control register. We set the basic
 * timer enable and start bits preserving other bits.
 * Setting the basic timer start bit causes hardware to:
 *   reset internal clock divider counter
 *   enable internal clock divider counter
 *   start the timer counter (up/down)
 *   clear all interrupt status
 * NOTE: timer starts with current value in Count register.
 * Preload register is only loaded into Count if the auto-restart
 * bit is set in the control register. Driver init, set alarm, and
 * set top should load both Count and Preload.
 */
static int cntr_mec5_btmr_start(const struct device *dev)
{
	const struct cntr_mec5_btmr_devcfg *const devcfg = dev->config;
	struct btmr_regs *const regs = devcfg->regs;

	mec_btimer_start(regs);

	return 0;
}

/* API - Stop the counter
 * Clears the basic timer Start bit.
 *  disables internal clock divider counter
 *  stops the timer counter
 * Clear any pending interrupt after stopping
 */
static int cntr_mec5_btmr_stop(const struct device *dev)
{
	const struct cntr_mec5_btmr_devcfg *const devcfg = dev->config;
	struct btmr_regs *const regs = devcfg->regs;

	mec_btimer_stop(regs);
	mec_btimer_intr_clr(regs);

	return 0;
}

/* API - Get current counter value */
static int cntr_mec5_btmr_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct cntr_mec5_btmr_devcfg *const devcfg = dev->config;
	struct btmr_regs *const regs = devcfg->regs;

	if (!ticks) {
		return -EINVAL;
	}

	*ticks = mec_btimer_count(regs);

	return 0;
}

/* API - Set a single-shot alarm
 * Basic timer only signals an event when it reaches terminal condition.
 * Setting an alarm means changing the current count value while it may
 * be running.
 * If the basic timer is running use the reload feature:
 *   write new initial count value based on alarm to preload register
 *   Set RELOAD bit to force basic timer to reload counter from preload register.
 * Else basic timer is not running
 *   start basic timer using alarm ticks for initial count and preload.
 *
 * Notes:
 * Alarm callback is mandatory.
 * Absolute alarm is not supported because basic timer interrupt is only
 * triggered when the counter reaches its terminal value.
 */
static int cntr_mec5_btmr_set_alarm(const struct device *dev, uint8_t chan_id,
				    const struct counter_alarm_cfg *alarm_cfg)
{
	const struct cntr_mec5_btmr_devcfg *const devcfg = dev->config;
	struct cntr_mec5_btmr_dev_data *const data = dev->data;
	struct btmr_regs *const regs = devcfg->regs;
	uint32_t ticks = 0, ticks_alarm;
	int ret = 0;

	if (chan_id != 0) {
		LOG_ERR("Invalid channel id %u", chan_id);
		return -ENOTSUP;
	}

	if (data->alarm_cb) {
		ret = -EBUSY;
	}

	if (!alarm_cfg) {
		LOG_ERR("Invalid alarm config");
		return -EINVAL;
	}

	if (!alarm_cfg->callback) {
		LOG_ERR("Alarm callback function cannot be null");
		return -EINVAL;
	}

	if (alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) {
		LOG_ERR("Absolute alarm is not supported");
		return -ENOTSUP;
	}

	ticks_alarm = alarm_cfg->ticks;

	if (ticks_alarm > data->top_ticks) {
		LOG_DBG("Request alarm ticks %u > %u current top",
			alarm_cfg->ticks, data->top_ticks);
		return -EINVAL;
	}

	/* down counter: set count register = alarm_cfg->ticks */
	ticks = ticks_alarm;

	mec_btimer_intr_en(regs, 0);

	data->alarm_cb = alarm_cfg->callback;
	data->alarm_cb_ud = alarm_cfg->user_data;

	if (mec_btimer_is_started(regs)) {
		mec_btimer_halt(regs);
		mec_btimer_intr_clr(regs);
		mec_btimer_count_set(regs, ticks);
		mec_btimer_unhalt(regs);
	} else { /* timer is stopped, load values but do not start */
		mec_btimer_count_set(regs, ticks);
		mec_btimer_start(regs);
	}

	mec_btimer_intr_en(regs, 1);

	return ret;
}

/* Cancels an alarm if previously configured.
 * Do not disable interrupt if a top callback is installed.
 */
static int cntr_mec5_btmr_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	const struct cntr_mec5_btmr_devcfg *const devcfg = dev->config;
	struct btmr_regs *const regs = devcfg->regs;
	struct cntr_mec5_btmr_dev_data *const data = dev->data;

	if (chan_id != 0) {
		LOG_ERR("Invalid channel id %u", chan_id);
		return -ENOTSUP;
	}

	mec_btimer_intr_en(regs, 0);

	data->alarm_cb = NULL;
	data->alarm_cb_ud = NULL;

	if (data->top_cb) {
		mec_btimer_intr_en(regs, 1);
	}

	LOG_DBG("%p Counter alarm canceled", dev);

	return 0;
}

static uint32_t cntr_mec5_btmr_get_pending_int(const struct device *dev)
{
	const struct cntr_mec5_btmr_devcfg *const devcfg = dev->config;
	struct btmr_regs *const regs = devcfg->regs;

	return mec_btimer_status(regs);
}

/* API - Return the current count top value.
 * We return the current top value set by driver init or successful
 * call to the set top value API.
 */
static uint32_t cntr_mec5_btmr_get_top_value(const struct device *dev)
{
	/* const struct cntr_mec5_btmr_devcfg *const devcfg = dev->config; */
	struct cntr_mec5_btmr_dev_data *const data = dev->data;

	return data->top_ticks;
}

/* API - Set a new top value and optional callback.
 * cfg->flags
 * COUNTER_TOP_CFG_DONT_RESET - Allow counter to free run while setting new top
 * COUNTER_TOP_CFG_RESET_WHEN_LATE - Reset counter if new top value will go out of bounds
 * NOTES:
 * Basic timer COUNT register should not be written while it is running.
 * Preload can be written while timer is running but there is a race condition
 * if the write is issues when the timer is about to reach its terminal count.
 * Hardware does not implement a free running counter therefore we can't support
 * COUNTER_TOP_CFG_DONT_RESET.
 *
 */
static int cntr_mec5_btmr_set_top_value(const struct device *dev,
					const struct counter_top_cfg *cfg)
{
	const struct cntr_mec5_btmr_devcfg *const devcfg = dev->config;
	const struct counter_config_info *info = &devcfg->info;
	struct cntr_mec5_btmr_dev_data *const data = dev->data;
	struct btmr_regs *const regs = devcfg->regs;
	bool running = false;
	uint32_t ticks = 0;
	int ret = 0;

	if (data->alarm_cb) {
		LOG_ERR("Changing top while an alarm is active is not allowed");
		return -EBUSY;
	}

	if (!cfg) {
		LOG_ERR("Invalid top config");
		return -EINVAL;
	}

	if (cfg->ticks > info->max_top_value) {
		LOG_ERR("New top exceeds max top value");
		return -EINVAL;
	}

	ticks = cfg->ticks;

	mec_btimer_intr_en(regs, 0);

	data->top_ticks = ticks;
	data->top_cb = cfg->callback;
	data->top_cb_ud = cfg->user_data;

	running = mec_btimer_is_started(regs);
	if (running) {
		if (cfg->flags & COUNTER_TOP_CFG_DONT_RESET) {
			if (mec_btimer_count(regs) > ticks) {
				ret = -ETIME;
				if (cfg->flags & COUNTER_TOP_CFG_RESET_WHEN_LATE) {
					mec_btimer_halt(regs);
					mec_btimer_count_set(regs, ticks);
					mec_btimer_unhalt(regs);
				}
			}
			mec_btimer_preload_set(regs, ticks);
		} else {
			mec_btimer_halt(regs);
			mec_btimer_preload_set(regs, ticks);
			mec_btimer_count_set(regs, ticks);
			mec_btimer_unhalt(regs);
		}
	} else {
		mec_btimer_preload_set(regs, ticks);
		mec_btimer_count_set(regs, ticks);
	}

	if (data->top_cb) {
		mec_btimer_auto_restart(regs, 1);
		mec_btimer_intr_en(regs, 1);
	} else {
		mec_btimer_auto_restart(regs, 0);
	}

	if (!running) {
		mec_btimer_start(regs);
	}

	return ret;
}

static uint32_t cntr_mec5_btmr_get_freq(const struct device *dev)
{
	const struct cntr_mec5_btmr_devcfg *const devcfg = dev->config;
	struct btmr_regs *const regs = devcfg->regs;

	return mec_btimer_freq(regs);
}

static void cntr_mec5_btmr_isr(const struct device *dev)
{
	const struct cntr_mec5_btmr_devcfg *const devcfg = dev->config;
	struct btmr_regs *const regs = devcfg->regs;
	struct cntr_mec5_btmr_dev_data *const data = dev->data;
	counter_alarm_callback_t alarm_cb = NULL;
	void *user_data = NULL;
	uint32_t cnt = mec_btimer_count(regs);

	data->isr_count++;

	mec_btimer_intr_clr(regs);

	LOG_DBG("%p Counter ISR", dev);

	/* Was interrupt from an alarm? */
	if (data->alarm_cb) {
		mec_btimer_intr_en(regs, 0);
		alarm_cb = data->alarm_cb;
		user_data = data->alarm_cb_ud;
		data->alarm_cb = NULL;
		data->alarm_cb_ud = NULL;
		alarm_cb(dev, 0, cnt, user_data);
	} else if (data->top_cb) {
		mec_btimer_intr_en(regs, 1);
		data->top_cb(dev, data->top_cb_ud);
	}
}

static const struct counter_driver_api cntr_mec5_btmr_api = {
		.start = cntr_mec5_btmr_start,
		.stop = cntr_mec5_btmr_stop,
		.get_value = cntr_mec5_btmr_get_value,
		.set_alarm = cntr_mec5_btmr_set_alarm,
		.cancel_alarm = cntr_mec5_btmr_cancel_alarm,
		.set_top_value = cntr_mec5_btmr_set_top_value,
		.get_pending_int = cntr_mec5_btmr_get_pending_int,
		.get_top_value = cntr_mec5_btmr_get_top_value,
		.get_freq = cntr_mec5_btmr_get_freq,
};

static int cntr_mec5_btmr_dev_init(const struct device *dev)
{
	const struct cntr_mec5_btmr_devcfg *const devcfg = dev->config;
	const struct counter_config_info *info = &devcfg->info;
	struct cntr_mec5_btmr_dev_data *const data = dev->data;
	struct btmr_regs *const regs = devcfg->regs;
	int ret = 0;
	uint32_t flags = 0;

	data->top_ticks = info->max_top_value;

	if (info->flags & COUNTER_CONFIG_INFO_COUNT_UP) {
		LOG_ERR("Count up not supported");
		return -ENOTSUP;
	}

	ret = mec_btimer_init(regs, (devcfg->prescaler + 1u), info->max_top_value, flags);
	if (ret != MEC_RET_OK) {
		return -EIO;
	}

	if (devcfg->irq_cfg_func) {
		devcfg->irq_cfg_func();
	}

	return 0;
}

#define CNTR_MEC5_BTMR_FREQ(i)					\
	((uint32_t)DT_INST_PROP_OR(i, clock_frequency, MHZ(48)) /\
	 ((uint32_t)DT_INST_PROP_OR(i, prescaler, 0) + 1))

#define COUNTER_MEC5_BTMR_INIT(inst)						\
	static void cntr_mec5_btmr_irq_config_##inst(void);			\
										\
	static struct cntr_mec5_btmr_dev_data cntr_mec5_btmr_dev_data_##inst;	\
										\
	static struct cntr_mec5_btmr_devcfg cntr_mec5_btmr_dcfg_##inst = {	\
		.info = {							\
			.max_top_value = DT_INST_PROP(inst, max_value),		\
			.freq = CNTR_MEC5_BTMR_FREQ(inst),			\
			.flags = 0,						\
			.channels = 1,						\
		},								\
										\
		.irq_cfg_func = cntr_mec5_btmr_irq_config_##inst,		\
		.regs = (struct btmr_regs *)DT_INST_REG_ADDR(inst),		\
		.prescaler = DT_INST_PROP_OR(inst, prescaler, 0),		\
	};									\
										\
	DEVICE_DT_INST_DEFINE(inst,						\
			    cntr_mec5_btmr_dev_init,				\
			    NULL,						\
			    &cntr_mec5_btmr_dev_data_##inst,			\
			    &cntr_mec5_btmr_dcfg_##inst,			\
			    POST_KERNEL,					\
			    CONFIG_COUNTER_INIT_PRIORITY,			\
			    &cntr_mec5_btmr_api);				\
										\
	static void cntr_mec5_btmr_irq_config_##inst(void)			\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(inst),					\
			    DT_INST_IRQ(inst, priority),			\
			    cntr_mec5_btmr_isr,					\
			    DEVICE_DT_INST_GET(inst), 0);			\
		irq_enable(DT_INST_IRQN(inst));					\
	}

DT_INST_FOREACH_STATUS_OKAY(COUNTER_MEC5_BTMR_INIT)
