/*
 * Copyright (c) 2026 Mahad Faisal
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_ra_gpt_counter

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/clock_control/renesas_ra_cgc.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/logging/log.h>

#include "r_gpt.h"
#include "r_gpt_cfg.h"

LOG_MODULE_REGISTER(counter_renesas_ra_gpt, CONFIG_COUNTER_LOG_LEVEL);

#define RA_GPT_MAX_TOP_VALUE UINT32_MAX

#define EVENT_GPT_CAPTURE_COMPARE_A(channel) \
	BSP_PRV_IELS_ENUM(CONCAT(EVENT_GPT, channel, _CAPTURE_COMPARE_A))

#define EVENT_GPT_COUNTER_OVERFLOW(channel) \
	BSP_PRV_IELS_ENUM(CONCAT(EVENT_GPT, channel, _COUNTER_OVERFLOW))

#define RA_GPT_PARENT(index) DT_INST_PARENT(index)
#define RA_GPT_CHANNEL(index) DT_PROP(RA_GPT_PARENT(index), channel)

extern void gpt_capture_compare_a_isr(void);
extern void gpt_counter_overflow_isr(void);

struct counter_renesas_ra_gpt_config {
	struct counter_config_info info;
	const struct device *clock_dev;
	struct clock_control_ra_subsys_cfg clock_subsys;
	uint16_t compare_a_event;
	uint16_t overflow_event;
};

struct counter_renesas_ra_gpt_data {
	gpt_instance_ctrl_t fsp_ctrl;
	timer_cfg_t fsp_cfg;
	gpt_extended_cfg_t extend_cfg;
	timer_callback_args_t callback_memory;

	counter_alarm_callback_t alarm_cb;
	void *alarm_user_data;
	bool alarm_active;

	counter_top_callback_t top_cb;
	void *top_user_data;

	struct k_spinlock lock;
	bool started;
	uint32_t top_value;
	uint32_t freq;
};

static int counter_renesas_ra_gpt_get_value(const struct device *dev, uint32_t *ticks)
{
	struct counter_renesas_ra_gpt_data *data = dev->data;
	timer_status_t status;
	fsp_err_t err;

	err = R_GPT_StatusGet(&data->fsp_ctrl, &status);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	*ticks = status.counter;

	return 0;
}

static uint32_t counter_renesas_ra_gpt_get_top_value(const struct device *dev)
{
	struct counter_renesas_ra_gpt_data *data = dev->data;

	return data->top_value;
}

static uint32_t counter_renesas_ra_gpt_get_freq(const struct device *dev)
{
	struct counter_renesas_ra_gpt_data *data = dev->data;

	return data->freq;
}

static void counter_renesas_ra_gpt_callback(timer_callback_args_t *args)
{
	const struct device *dev = args->p_context;
	struct counter_renesas_ra_gpt_data *data = dev->data;
	counter_alarm_callback_t alarm_cb = NULL;
	counter_top_callback_t top_cb = NULL;
	void *user_data = NULL;
	uint32_t now = 0;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);

	if ((args->event == TIMER_EVENT_COMPARE_A || args->event == TIMER_EVENT_CAPTURE_A) &&
	    data->alarm_active) {
		alarm_cb = data->alarm_cb;
		user_data = data->alarm_user_data;
		data->alarm_cb = NULL;
		data->alarm_user_data = NULL;
		data->alarm_active = false;
	} else if (args->event == TIMER_EVENT_CYCLE_END) {
		top_cb = data->top_cb;
		user_data = data->top_user_data;
	}

	k_spin_unlock(&data->lock, key);

	if (alarm_cb != NULL) {
		if (counter_renesas_ra_gpt_get_value(dev, &now) == 0) {
			alarm_cb(dev, 0, now, user_data);
		}
	} else if (top_cb != NULL) {
		top_cb(dev, user_data);
	}
}

static int counter_renesas_ra_gpt_start(const struct device *dev)
{
	struct counter_renesas_ra_gpt_data *data = dev->data;
	fsp_err_t err;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);

	if (data->started) {
		k_spin_unlock(&data->lock, key);
		return -EALREADY;
	}

	err = R_GPT_Start(&data->fsp_ctrl);
	if (err != FSP_SUCCESS) {
		k_spin_unlock(&data->lock, key);
		return -EIO;
	}

	data->started = true;

	irq_enable(data->fsp_cfg.cycle_end_irq);

	if (data->alarm_active) {
		irq_enable(data->extend_cfg.capture_a_irq);
	}

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int counter_renesas_ra_gpt_stop(const struct device *dev)
{
	struct counter_renesas_ra_gpt_data *data = dev->data;
	fsp_err_t err;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);

	if (!data->started) {
		k_spin_unlock(&data->lock, key);
		return 0;
	}

	irq_disable(data->fsp_cfg.cycle_end_irq);
	irq_disable(data->extend_cfg.capture_a_irq);

	err = R_GPT_Stop(&data->fsp_ctrl);
	if (err != FSP_SUCCESS) {
		k_spin_unlock(&data->lock, key);
		return -EIO;
	}

	data->started = false;

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int counter_renesas_ra_gpt_set_top_value(const struct device *dev,
						const struct counter_top_cfg *cfg)
{
	struct counter_renesas_ra_gpt_data *data = dev->data;
	fsp_err_t err;
	k_spinlock_key_t key;

	if (cfg->ticks == 0U) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	err = R_GPT_PeriodSet(&data->fsp_ctrl, cfg->ticks);
	if (err != FSP_SUCCESS) {
		k_spin_unlock(&data->lock, key);
		return -EIO;
	}

	data->top_value = cfg->ticks;
	data->top_cb = cfg->callback;
	data->top_user_data = cfg->user_data;

	if (!(cfg->flags & COUNTER_TOP_CFG_DONT_RESET)) {
		(void)R_GPT_Reset(&data->fsp_ctrl);
	}

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int counter_renesas_ra_gpt_set_alarm(const struct device *dev, uint8_t chan_id,
					    const struct counter_alarm_cfg *alarm_cfg)
{
	struct counter_renesas_ra_gpt_data *data = dev->data;
	uint32_t now;
	uint32_t alarm_ticks;
	uint64_t top_plus_one;
	fsp_err_t err;
	k_spinlock_key_t key;

	if (chan_id != 0U || alarm_cfg->callback == NULL) {
		return -EINVAL;
	}

	err = counter_renesas_ra_gpt_get_value(dev, &now);
	if (err != 0) {
		return err;
	}

	key = k_spin_lock(&data->lock);

	if (data->alarm_active) {
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}

	if (alarm_cfg->ticks > data->top_value) {
		k_spin_unlock(&data->lock, key);
		return -EINVAL;
	}

	if (alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) {
		alarm_ticks = alarm_cfg->ticks;
	} else {
		top_plus_one = (uint64_t)data->top_value + 1ULL;
		alarm_ticks = (uint32_t)(((uint64_t)now + alarm_cfg->ticks) % top_plus_one);
	}

	err = R_GPT_CompareMatchSet(&data->fsp_ctrl, alarm_ticks, TIMER_COMPARE_MATCH_A);
	if (err != FSP_SUCCESS) {
		k_spin_unlock(&data->lock, key);
		return -EIO;
	}

	data->alarm_cb = alarm_cfg->callback;
	data->alarm_user_data = alarm_cfg->user_data;
	data->alarm_active = true;

	if (data->started) {
		irq_enable(data->extend_cfg.capture_a_irq);
	}

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int counter_renesas_ra_gpt_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	struct counter_renesas_ra_gpt_data *data = dev->data;
	k_spinlock_key_t key;

	if (chan_id != 0U) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	data->alarm_cb = NULL;
	data->alarm_user_data = NULL;
	data->alarm_active = false;

	irq_disable(data->extend_cfg.capture_a_irq);

	k_spin_unlock(&data->lock, key);

	return 0;
}

static uint32_t counter_renesas_ra_gpt_get_pending_int(const struct device *dev)
{
	struct counter_renesas_ra_gpt_data *data = dev->data;

	return NVIC_GetPendingIRQ(data->fsp_cfg.cycle_end_irq) ||
	       NVIC_GetPendingIRQ(data->extend_cfg.capture_a_irq);
}

static int counter_renesas_ra_gpt_init(const struct device *dev)
{
	const struct counter_renesas_ra_gpt_config *cfg = dev->config;
	struct counter_renesas_ra_gpt_data *data = dev->data;
	timer_info_t info;
	fsp_err_t fsp_err;
	int err;

	if (!device_is_ready(cfg->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	err = clock_control_on(cfg->clock_dev, (clock_control_subsys_t)&cfg->clock_subsys);
	if (err < 0) {
		LOG_ERR("Could not initialize clock (%d)", err);
		return err;
	}

	data->fsp_cfg.p_callback = counter_renesas_ra_gpt_callback;
	data->fsp_cfg.p_context = (void *)dev;
	data->fsp_cfg.p_extend = &data->extend_cfg;

	fsp_err = R_GPT_Open(&data->fsp_ctrl, &data->fsp_cfg);
	if (fsp_err != FSP_SUCCESS) {
		return -EIO;
	}

	fsp_err = R_GPT_InfoGet(&data->fsp_ctrl, &info);
	if (fsp_err != FSP_SUCCESS) {
		return -EIO;
	}

	data->top_value = (info.period_counts == 0U) ? RA_GPT_MAX_TOP_VALUE : info.period_counts;
	data->freq = info.clock_frequency;

	R_BSP_IrqCfgEnable(data->fsp_cfg.cycle_end_irq, data->fsp_cfg.cycle_end_ipl,
			   &data->fsp_ctrl);
	R_BSP_IrqCfgEnable(data->extend_cfg.capture_a_irq, data->extend_cfg.capture_a_ipl,
			   &data->fsp_ctrl);

	R_ICU->IELSR[data->fsp_cfg.cycle_end_irq] = (elc_event_t)cfg->overflow_event;
	R_ICU->IELSR[data->extend_cfg.capture_a_irq] = (elc_event_t)cfg->compare_a_event;

	irq_disable(data->fsp_cfg.cycle_end_irq);
	irq_disable(data->extend_cfg.capture_a_irq);

	return 0;
}

static DEVICE_API(counter, counter_renesas_ra_gpt_api) = {
	.start = counter_renesas_ra_gpt_start,
	.stop = counter_renesas_ra_gpt_stop,
	.get_value = counter_renesas_ra_gpt_get_value,
	.set_alarm = counter_renesas_ra_gpt_set_alarm,
	.cancel_alarm = counter_renesas_ra_gpt_cancel_alarm,
	.set_top_value = counter_renesas_ra_gpt_set_top_value,
	.get_pending_int = counter_renesas_ra_gpt_get_pending_int,
	.get_top_value = counter_renesas_ra_gpt_get_top_value,
	.get_freq = counter_renesas_ra_gpt_get_freq,
};

#define COUNTER_RA_GPT_INIT(index)								\
	static const gpt_extended_cfg_t counter_renesas_ra_gpt_extend_##index = {		\
		.gtioca = {									\
			.output_enabled = false,						\
			.stop_level = GPT_PIN_LEVEL_LOW,					\
		},										\
		.gtiocb = {									\
			.output_enabled = false,						\
			.stop_level = GPT_PIN_LEVEL_LOW,					\
		},										\
		.start_source = (gpt_source_t)(GPT_SOURCE_NONE),				\
		.stop_source = (gpt_source_t)(GPT_SOURCE_NONE),					\
		.clear_source = (gpt_source_t)(GPT_SOURCE_NONE),				\
		.count_up_source = (gpt_source_t)(GPT_SOURCE_NONE),				\
		.count_down_source = (gpt_source_t)(GPT_SOURCE_NONE),				\
		.capture_a_source = (gpt_source_t)(GPT_SOURCE_NONE),				\
		.capture_b_source = (gpt_source_t)(GPT_SOURCE_NONE),				\
		.capture_a_ipl = DT_IRQ_BY_NAME(RA_GPT_PARENT(index), gtioca, priority),	\
		.capture_b_ipl = BSP_IRQ_DISABLED,						\
		.capture_a_irq = DT_IRQ_BY_NAME(RA_GPT_PARENT(index), gtioca, irq),		\
		.capture_b_irq = FSP_INVALID_VECTOR,						\
		.capture_filter_gtioca = GPT_CAPTURE_FILTER_NONE,				\
		.capture_filter_gtiocb = GPT_CAPTURE_FILTER_NONE,				\
		.p_pwm_cfg = NULL,								\
		.gtior_setting.gtior = (0x0U),						\
		.gtioca_polarity = GPT_GTIOC_POLARITY_NORMAL,				\
		.gtiocb_polarity = GPT_GTIOC_POLARITY_NORMAL,				\
	};											\
	static struct counter_renesas_ra_gpt_data counter_renesas_ra_gpt_data_##index = {	\
		.fsp_cfg = {									\
			.mode = TIMER_MODE_PERIODIC,						\
			.period_counts = RA_GPT_MAX_TOP_VALUE,				\
			.source_div = DT_PROP(RA_GPT_PARENT(index), divider),		\
			.channel = DT_PROP(RA_GPT_PARENT(index), channel),		\
			.cycle_end_ipl = DT_IRQ_BY_NAME(RA_GPT_PARENT(index), overflow, priority),\
			.cycle_end_irq = DT_IRQ_BY_NAME(RA_GPT_PARENT(index), overflow, irq),	\
		},										\
		.extend_cfg = counter_renesas_ra_gpt_extend_##index,			\
	};											\
	static const struct counter_renesas_ra_gpt_config counter_renesas_ra_gpt_config_##index = {\
		.info = {									\
			.max_top_value = RA_GPT_MAX_TOP_VALUE,				\
			.flags = COUNTER_CONFIG_INFO_COUNT_UP,				\
			.channels = 1,								\
			.freq = 0,								\
		},										\
		.clock_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR(RA_GPT_PARENT(index))),		\
		.clock_subsys = {								\
			.mstp = (uint32_t)DT_CLOCKS_CELL_BY_IDX(RA_GPT_PARENT(index), 0, mstp),\
			.stop_bit = DT_CLOCKS_CELL_BY_IDX(RA_GPT_PARENT(index), 0, stop_bit),	\
		},										\
		.compare_a_event = EVENT_GPT_CAPTURE_COMPARE_A(RA_GPT_CHANNEL(index)),\
		.overflow_event = EVENT_GPT_COUNTER_OVERFLOW(RA_GPT_CHANNEL(index)),\
	};											\
	static int counter_renesas_ra_gpt_init_##index(const struct device *dev)		\
	{											\
		BSP_ASSIGN_EVENT_TO_CURRENT_CORE(						\
			EVENT_GPT_CAPTURE_COMPARE_A(DT_PROP(RA_GPT_PARENT(index), channel)));	\
		BSP_ASSIGN_EVENT_TO_CURRENT_CORE(						\
			EVENT_GPT_COUNTER_OVERFLOW(DT_PROP(RA_GPT_PARENT(index), channel)));	\
		IRQ_CONNECT(DT_IRQ_BY_NAME(RA_GPT_PARENT(index), gtioca, irq),			\
			    DT_IRQ_BY_NAME(RA_GPT_PARENT(index), gtioca, priority),		\
			    gpt_capture_compare_a_isr, NULL, 0);				\
		IRQ_CONNECT(DT_IRQ_BY_NAME(RA_GPT_PARENT(index), overflow, irq),		\
			    DT_IRQ_BY_NAME(RA_GPT_PARENT(index), overflow, priority),		\
			    gpt_counter_overflow_isr, NULL, 0);				\
		return counter_renesas_ra_gpt_init(dev);					\
	}											\
	DEVICE_DT_INST_DEFINE(index, counter_renesas_ra_gpt_init_##index, NULL,		\
			      &counter_renesas_ra_gpt_data_##index,			\
			      &counter_renesas_ra_gpt_config_##index,			\
			      POST_KERNEL, CONFIG_COUNTER_INIT_PRIORITY,			\
			      &counter_renesas_ra_gpt_api);

DT_INST_FOREACH_STATUS_OKAY(COUNTER_RA_GPT_INIT)
