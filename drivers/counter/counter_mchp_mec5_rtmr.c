/*
 * Copyright (c) 2024 Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_mec5_rtmr_counter

/**
 * @file
 * @brief Microchip MEC5 HAL RTOS timer counter driver
 *
 * The RTOS timer is a 32-bit down counter using a fixed 32 KHz input clock.
 * When the timer count reaches 0 it signals an interrupt if enabled and
 * if enabled can reload the counter from the preload register.
 *
 */


#include <soc.h>
#include <errno.h>
#include <stdbool.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>

#include <device_mec5.h>
#include <mec_rtimer_api.h>

LOG_MODULE_REGISTER(counter_mec5_rtmr, CONFIG_COUNTER_LOG_LEVEL);

/* NOTE: struct counter_config_info must be first member */
struct cntr_mec5_rtmr_devcfg {
	struct counter_config_info info;
	struct rtmr_regs *regs;
	void (*irq_cfg_func)(void);
};

struct cntr_mec5_rtmr_dev_data {
	uint32_t top_ticks;
	counter_alarm_callback_t alarm_cb;
	void *alarm_cb_ud;
	counter_top_callback_t top_cb;
	void *top_cb_ud;
};

/* API - Start counter device in free running mode
 * RTOS timer implements a 32-bit count down counter.
 * On start, it loads the value in the preload register into
 * it count register and begins counting down. Once the count
 * register reaches 0 it stops counting and asserts its interrupt
 * signal. If auto-reload is enabled it will load count from preload
 * and begin counting down again.
 */
static int cntr_mec5_rtmr_start(const struct device *dev)
{
	const struct cntr_mec5_rtmr_devcfg *const devcfg = dev->config;
	struct rtmr_regs *const regs = devcfg->regs;

	mec_rtimer_start(regs);

	return 0;
}

/* API - Stop the counter
 * Clears the RTOS timer start bit.
 * Clear any pending interrupt after stopping
 */
static int cntr_mec5_rtmr_stop(const struct device *dev)
{
	const struct cntr_mec5_rtmr_devcfg *const devcfg = dev->config;
	struct rtmr_regs *const regs = devcfg->regs;

	mec_rtimer_stop(regs);
	mec_rtimer_status_clear(regs, BIT(MEC_RTMR_STATUS_TERM_POS));

	return 0;
}

/* API - Get current counter value */
static int cntr_mec5_rtmr_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct cntr_mec5_rtmr_devcfg *const devcfg = dev->config;
	struct rtmr_regs *const regs = devcfg->regs;

	if (!ticks) {
		return -EINVAL;
	}

	*ticks = mec_rtimer_count(regs);

	return 0;
}

/* API - Set a single-shot alarm
 * RTOS timer only signals an event when it reaches terminal count (0).
 * Setting an alarm means changing the current count value while it may
 * be running.
 * If the basic timer is running
 *    Halt timer, write new value to count, and unhalt
 * Else basic timer is not running
 *    Write alarm value to preload and do not start
 *
 * Notes:
 * Alarm callback is mandatory.
 * Absolute alarm is not supported because basic timer interrupt is only
 * triggered when the counter reaches its terminal value.
 */
static int cntr_mec5_rtmr_set_alarm(const struct device *dev, uint8_t chan_id,
				    const struct counter_alarm_cfg *alarm_cfg)
{
	const struct cntr_mec5_rtmr_devcfg *const devcfg = dev->config;
	struct cntr_mec5_rtmr_dev_data *const data = dev->data;
	struct rtmr_regs *const regs = devcfg->regs;
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

	if (alarm_cfg->ticks > data->top_ticks) {
		LOG_DBG("Request alarm ticks %u > %u current top",
			alarm_cfg->ticks, data->top_ticks);
		return -EINVAL;
	}

	mec_rtimer_intr_ctrl(regs, 0);

	data->alarm_cb = alarm_cfg->callback;
	data->alarm_cb_ud = alarm_cfg->user_data;

	mec_rtimer_restart(regs, alarm_cfg->ticks, 1);
	mec_rtimer_intr_ctrl(regs, 1);

	return ret;
}

/* Cancels an alarm if previously configured.
 * Do not disable interrupt if a top callback is installed.
 */
static int cntr_mec5_rtmr_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	const struct cntr_mec5_rtmr_devcfg *const devcfg = dev->config;
	struct rtmr_regs *const regs = devcfg->regs;
	struct cntr_mec5_rtmr_dev_data *const data = dev->data;

	if (chan_id != 0) {
		LOG_ERR("Invalid channel id %u", chan_id);
		return -ENOTSUP;
	}

	mec_rtimer_intr_ctrl(regs, 0);

	data->alarm_cb = NULL;
	data->alarm_cb_ud = NULL;

	if (data->top_cb) {
		mec_rtimer_intr_ctrl(regs, 1);
	}

	LOG_DBG("%p Counter alarm canceled", dev);

	return 0;
}

static uint32_t cntr_mec5_rtmr_get_pending_int(const struct device *dev)
{
	const struct cntr_mec5_rtmr_devcfg *const devcfg = dev->config;
	struct rtmr_regs *const regs = devcfg->regs;

	return mec_rtimer_status(regs);
}

/* API - Return the current count top value.
 * We return the current top value set by driver init or successful
 * call to the set top value API.
 */
static uint32_t cntr_mec5_rtmr_get_top_value(const struct device *dev)
{
	struct cntr_mec5_rtmr_dev_data *const data = dev->data;

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
static int cntr_mec5_rtmr_set_top_value(const struct device *dev,
					const struct counter_top_cfg *cfg)
{
	const struct cntr_mec5_rtmr_devcfg *const devcfg = dev->config;
	const struct counter_config_info *info = &devcfg->info;
	struct cntr_mec5_rtmr_dev_data *const data = dev->data;
	struct rtmr_regs *const regs = devcfg->regs;
	uint32_t ticks = 0;
	int ret = 0;
	uint8_t restart = 0;

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

	if (cfg->flags & COUNTER_TOP_CFG_DONT_RESET) {
		LOG_ERR("Updating top value without reset is not supported");
		return -ENOTSUP;
	}

	ticks = cfg->ticks;

	mec_rtimer_intr_ctrl(regs, 0);

	data->top_ticks = ticks;
	data->top_cb = cfg->callback;
	data->top_cb_ud = cfg->user_data;

	if (mec_rtimer_is_counting(regs)) {
		restart = 1u;
	}

	mec_rtimer_restart(regs, ticks, restart);

	if (data->top_cb) {
		mec_rtimer_auto_reload(regs, 1);
		mec_rtimer_intr_ctrl(regs, 1);
	} else {
		mec_rtimer_auto_reload(regs, 0);
	}

	return ret;
}

static uint32_t cntr_mec5_rtmr_get_freq(const struct device *dev)
{
	return (uint32_t)(MEC_RTIMER_MAIN_CLK_FREQ);
}

static void cntr_mec5_rtmr_isr(const struct device *dev)
{
	const struct cntr_mec5_rtmr_devcfg *const devcfg = dev->config;
	struct rtmr_regs *const regs = devcfg->regs;
	struct cntr_mec5_rtmr_dev_data *const data = dev->data;
	counter_alarm_callback_t alarm_cb = NULL;
	void *user_data = NULL;
	uint32_t status = mec_rtimer_status(regs);
	uint32_t cnt = mec_rtimer_count(regs);

	mec_rtimer_status_clear(regs, status);

	LOG_DBG("%p Counter ISR", dev);

	/* Was interrupt from an alarm? */
	if (data->alarm_cb) {
		mec_rtimer_intr_ctrl(regs, 0);
		alarm_cb = data->alarm_cb;
		user_data = data->alarm_cb_ud;
		data->alarm_cb = NULL;
		data->alarm_cb_ud = NULL;
		alarm_cb(dev, 0, cnt, user_data);
	} else if (data->top_cb) {
		mec_rtimer_intr_ctrl(regs, 1);
		data->top_cb(dev, data->top_cb_ud);
	}
}

static const struct counter_driver_api cntr_mec5_rtmr_api = {
		.start = cntr_mec5_rtmr_start,
		.stop = cntr_mec5_rtmr_stop,
		.get_value = cntr_mec5_rtmr_get_value,
		.set_alarm = cntr_mec5_rtmr_set_alarm,
		.cancel_alarm = cntr_mec5_rtmr_cancel_alarm,
		.set_top_value = cntr_mec5_rtmr_set_top_value,
		.get_pending_int = cntr_mec5_rtmr_get_pending_int,
		.get_top_value = cntr_mec5_rtmr_get_top_value,
		.get_freq = cntr_mec5_rtmr_get_freq,
};

static int cntr_mec5_rtmr_dev_init(const struct device *dev)
{
	const struct cntr_mec5_rtmr_devcfg *const devcfg = dev->config;
	const struct counter_config_info *info = &devcfg->info;
	struct cntr_mec5_rtmr_dev_data *const data = dev->data;
	struct rtmr_regs *const regs = devcfg->regs;
	uint32_t rtmr_cfg = BIT(MEC_RTMR_CFG_EN_POS);
	int ret = 0;

	if (IS_ENABLED(CONFIG_SOC_MEC_DEBUG_AND_TRACING)) {
		rtmr_cfg |= BIT(MEC_RTMR_CFG_DBG_HALT_POS);
	}

	data->top_ticks = info->max_top_value;

	if (info->flags & COUNTER_CONFIG_INFO_COUNT_UP) {
		LOG_ERR("Count up not supported");
		return -ENOTSUP;
	}

	ret = mec_rtimer_init(regs, rtmr_cfg, info->max_top_value);
	if (ret != MEC_RET_OK) {
		return -EIO;
	}

	if (devcfg->irq_cfg_func) {
		devcfg->irq_cfg_func();
	}

	return 0;
}

#define COUNTER_MEC5_RTMR_INIT(inst)						\
	static void cntr_mec5_rtmr_irq_config_##inst(void);			\
										\
	static struct cntr_mec5_rtmr_dev_data cntr_mec5_rtmr_dev_data_##inst;	\
										\
	static struct cntr_mec5_rtmr_devcfg cntr_mec5_rtmr_dcfg_##inst = {	\
		.regs = (struct rtmr_regs *)DT_INST_REG_ADDR(inst),		\
		.irq_cfg_func = cntr_mec5_rtmr_irq_config_##inst,		\
		.info = {							\
			.max_top_value = DT_INST_PROP(inst, max_value),		\
			.freq = MEC_RTIMER_MAIN_CLK_FREQ,			\
			.flags = 0,						\
			.channels = 1,						\
		},								\
	};									\
										\
	DEVICE_DT_INST_DEFINE(inst,						\
			      cntr_mec5_rtmr_dev_init,				\
			      NULL,						\
			      &cntr_mec5_rtmr_dev_data_##inst,			\
			      &cntr_mec5_rtmr_dcfg_##inst,			\
			      POST_KERNEL,					\
			      CONFIG_COUNTER_INIT_PRIORITY,			\
			      &cntr_mec5_rtmr_api);				\
										\
	static void cntr_mec5_rtmr_irq_config_##inst(void)			\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(inst),					\
			    DT_INST_IRQ(inst, priority),			\
			    cntr_mec5_rtmr_isr,					\
			    DEVICE_DT_INST_GET(inst), 0);			\
		irq_enable(DT_INST_IRQN(inst));					\
	}

DT_INST_FOREACH_STATUS_OKAY(COUNTER_MEC5_RTMR_INIT)
