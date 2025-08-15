/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rz_cmtw_counter

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/interrupt_controller/gic.h>
#include <r_cmtw.h>

#define RZ_CMTW_TOP_VALUE UINT32_MAX

#define RZ_CMTW_CMWCR_CCLR_OFFSET (13U)
#define RZ_CMTW_CMWCR_CCLR_MASK   (0x07U)

#define counter_rz_cmtw_clear_pending(irq) arm_gic_irq_clear_pending(irq)
#define counter_rz_cmtw_set_pending(irq)   arm_gic_irq_set_pending(irq)
#define counter_rz_cmtw_is_pending(irq)    arm_gic_irq_is_pending(irq)

struct counter_rz_cmtw_config {
	struct counter_config_info config_info;
	const timer_api_t *fsp_api;
};

struct counter_rz_cmtw_data {
	timer_cfg_t *fsp_cfg;
	cmtw_instance_ctrl_t *fsp_ctrl;
	/* Top callback function */
	counter_top_callback_t top_cb;
	/* Alarm callback function */
	counter_alarm_callback_t alarm_cb;
	void *user_data;
	struct k_spinlock lock;
	uint32_t guard_period;
	bool is_started;
};

static int counter_rz_cmtw_period_set(const struct device *dev, uint32_t period)
{
	const struct counter_rz_cmtw_config *cfg = dev->config;
	struct counter_rz_cmtw_data *data = dev->data;
	int err;

	if (data->is_started) {
		err = cfg->fsp_api->stop(data->fsp_ctrl);
		if (err != FSP_SUCCESS) {
			return -EIO;
		}
	}

	err = cfg->fsp_api->periodSet(data->fsp_ctrl, period);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	if (data->is_started) {
		err = cfg->fsp_api->start(data->fsp_ctrl);
		if (err != FSP_SUCCESS) {
			return -EIO;
		}
	}

	return 0;
}

static void counter_rz_cmtw_apply_clear_source(const struct device *dev,
					       cmtw_clear_source_t clear_source)
{
	struct counter_rz_cmtw_data *data = dev->data;
	cmtw_extended_cfg_t *fsp_cfg_extend = (cmtw_extended_cfg_t *)data->fsp_cfg->p_extend;
	uint32_t cmwcr;

	fsp_cfg_extend->clear_source = clear_source;

	cmwcr = data->fsp_ctrl->p_reg->CMWCR;
	cmwcr &= ~(RZ_CMTW_CMWCR_CCLR_MASK << RZ_CMTW_CMWCR_CCLR_OFFSET);
	cmwcr |= (fsp_cfg_extend->clear_source & RZ_CMTW_CMWCR_CCLR_MASK)
		 << RZ_CMTW_CMWCR_CCLR_OFFSET;

	data->fsp_ctrl->p_reg->CMWCR = cmwcr;
}

static int counter_rz_cmtw_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct counter_rz_cmtw_config *cfg = dev->config;
	struct counter_rz_cmtw_data *data = dev->data;
	timer_status_t timer_status;
	int err;

	err = cfg->fsp_api->statusGet(data->fsp_ctrl, &timer_status);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}
	*ticks = (uint32_t)timer_status.counter;

	return 0;
}

static void counter_rz_cmtw_irq_handler(timer_callback_args_t *p_args)
{
	const struct device *dev = p_args->p_context;
	struct counter_rz_cmtw_data *data = dev->data;
	counter_alarm_callback_t alarm_callback = data->alarm_cb;

	if (alarm_callback) {
		uint32_t now;

		if (counter_rz_cmtw_get_value(dev, &now) != 0) {
			return;
		}
		data->alarm_cb = NULL;
		alarm_callback(dev, 0, now, data->user_data);
	} else if (data->top_cb) {
		data->top_cb(dev, data->user_data);
	}
}

static int counter_rz_cmtw_init(const struct device *dev)
{
	struct counter_rz_cmtw_data *data = dev->data;
	const struct counter_rz_cmtw_config *cfg = dev->config;
	int err;

	err = cfg->fsp_api->open(data->fsp_ctrl, data->fsp_cfg);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	return err;
}

static int counter_rz_cmtw_start(const struct device *dev)
{
	const struct counter_rz_cmtw_config *cfg = dev->config;
	struct counter_rz_cmtw_data *data = dev->data;
	k_spinlock_key_t key;
	int err;

	key = k_spin_lock(&data->lock);

	if (data->is_started) {
		k_spin_unlock(&data->lock, key);
		return -EALREADY;
	}

	err = cfg->fsp_api->start(data->fsp_ctrl);
	if (err != FSP_SUCCESS) {
		k_spin_unlock(&data->lock, key);
		return -EIO;
	}

	data->is_started = true;
	irq_enable(data->fsp_cfg->cycle_end_irq);

	k_spin_unlock(&data->lock, key);

	return err;
}

static int counter_rz_cmtw_stop(const struct device *dev)
{
	const struct counter_rz_cmtw_config *cfg = dev->config;
	struct counter_rz_cmtw_data *data = dev->data;
	k_spinlock_key_t key;
	int err;

	key = k_spin_lock(&data->lock);

	if (!data->is_started) {
		k_spin_unlock(&data->lock, key);
		return 0;
	}

	/* Stop timer */
	err = cfg->fsp_api->stop(data->fsp_ctrl);
	if (err != FSP_SUCCESS) {
		k_spin_unlock(&data->lock, key);
		return -EIO;
	}

	/* Disable irq */
	irq_disable(data->fsp_cfg->cycle_end_irq);
	counter_rz_cmtw_clear_pending(data->fsp_cfg->cycle_end_irq);

	data->top_cb = NULL;
	data->alarm_cb = NULL;
	data->user_data = NULL;
	data->is_started = false;

	k_spin_unlock(&data->lock, key);

	return err;
}

static int counter_rz_cmtw_set_alarm(const struct device *dev, uint8_t chan,
				     const struct counter_alarm_cfg *alarm_cfg)
{
	struct counter_rz_cmtw_data *data = dev->data;
	cmtw_extended_cfg_t *fsp_cfg_extend = (cmtw_extended_cfg_t *)data->fsp_cfg->p_extend;

	bool absolute;
	uint32_t val;
	k_spinlock_key_t key;
	bool irq_on_late;
	uint32_t max_rel_val;
	uint32_t now, diff;
	uint32_t read_again;
	int err;

	if (chan != 0) {
		return -EINVAL;
	}

	if (!alarm_cfg) {
		return -EINVAL;
	}

	absolute = alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE;
	val = alarm_cfg->ticks;

	/* Alarm callback is mandatory */
	if (!alarm_cfg->callback) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	if (!data->is_started) {
		k_spin_unlock(&data->lock, key);
		return -EINVAL;
	}

	/* Alarm_cb need equal NULL before */
	if (data->alarm_cb) {
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}

	if (fsp_cfg_extend->clear_source != CMTW_CLEAR_SOURCE_DISABLED) {
		if (val > data->fsp_ctrl->period) {
			k_spin_unlock(&data->lock, key);
			return -EINVAL;
		}

		counter_rz_cmtw_apply_clear_source(dev, CMTW_CLEAR_SOURCE_DISABLED);
	}

	err = counter_rz_cmtw_get_value(dev, &now);
	if (err) {
		k_spin_unlock(&data->lock, key);
		return err;
	}

	data->alarm_cb = alarm_cfg->callback;
	data->user_data = alarm_cfg->user_data;

	if (absolute) {
		max_rel_val = RZ_CMTW_TOP_VALUE - data->guard_period;
		irq_on_late = alarm_cfg->flags & COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE;
	} else {
		/* If relative value is smaller than half of the counter range
		 * it is assumed that there is a risk of setting value too late
		 * and late detection algorithm must be applied. When late
		 * setting is detected, interrupt shall be triggered for
		 * immediate expiration of the timer. Detection is performed
		 * by limiting relative distance between CC and counter.
		 *
		 * Note that half of counter range is an arbitrary value.
		 */
		irq_on_late = val < (RZ_CMTW_TOP_VALUE / 2U);
		/* Limit max to detect short relative being set too late. */
		max_rel_val = irq_on_late ? RZ_CMTW_TOP_VALUE / 2U : RZ_CMTW_TOP_VALUE;
		val = (now + val) & RZ_CMTW_TOP_VALUE;
	}

	err = counter_rz_cmtw_period_set(dev, val);
	if (err) {
		k_spin_unlock(&data->lock, key);
		return err;
	}

	err = counter_rz_cmtw_get_value(dev, &read_again);
	if (err) {
		k_spin_unlock(&data->lock, key);
		return err;
	}

	if (val >= read_again) {
		diff = (val - read_again);
	} else {
		diff = val + (RZ_CMTW_TOP_VALUE - read_again);
	}

	if (diff > max_rel_val) {
		if (absolute) {
			err = -ETIME;
		}

		/* Interrupt is triggered always for relative alarm and
		 * for absolute depending on the flag.
		 */
		if (irq_on_late) {
			irq_enable(data->fsp_cfg->cycle_end_irq);
			counter_rz_cmtw_set_pending(data->fsp_cfg->cycle_end_irq);
		} else {
			data->alarm_cb = NULL;
		}
	} else {
		if (diff == 0) {
			/* RELOAD value could be set just in time for interrupt
			 * trigger or too late. In any case time is interrupt
			 * should be triggered. No need to enable interrupt
			 * on TIMER just make sure interrupt is pending.
			 */
			irq_enable(data->fsp_cfg->cycle_end_irq);
			counter_rz_cmtw_set_pending(data->fsp_cfg->cycle_end_irq);
		} else {
			counter_rz_cmtw_clear_pending(data->fsp_cfg->cycle_end_irq);
			irq_enable(data->fsp_cfg->cycle_end_irq);
		}
	}

	k_spin_unlock(&data->lock, key);

	return err;
}

static int counter_rz_cmtw_cancel_alarm(const struct device *dev, uint8_t chan)
{
	struct counter_rz_cmtw_data *data = dev->data;
	k_spinlock_key_t key;

	ARG_UNUSED(chan);

	key = k_spin_lock(&data->lock);

	if (!data->is_started) {
		k_spin_unlock(&data->lock, key);
		return -EINVAL;
	}

	if (!data->alarm_cb) {
		k_spin_unlock(&data->lock, key);
		return 0;
	}

	irq_disable(data->fsp_cfg->cycle_end_irq);
	counter_rz_cmtw_clear_pending(data->fsp_cfg->cycle_end_irq);
	data->alarm_cb = NULL;
	data->user_data = NULL;

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int counter_rz_cmtw_set_top_value(const struct device *dev,
					 const struct counter_top_cfg *top_cfg)
{
	const struct counter_rz_cmtw_config *cfg = dev->config;
	struct counter_rz_cmtw_data *data = dev->data;
	k_spinlock_key_t key;
	uint32_t cur_tick;
	bool reset;
	int err;

	cmtw_extended_cfg_t *fsp_cfg_extend = (cmtw_extended_cfg_t *)data->fsp_cfg->p_extend;

	if (!top_cfg) {
		return -EINVAL;
	}

	/* -EBUSY if any alarm is active */
	if (data->alarm_cb) {
		return -EBUSY;
	}

	key = k_spin_lock(&data->lock);

	data->top_cb = top_cfg->callback;
	data->user_data = top_cfg->user_data;

	if (!data->top_cb) {
		/* New top cfg is without callback - stop IRQs */
		irq_disable(data->fsp_cfg->cycle_end_irq);
		counter_rz_cmtw_clear_pending(data->fsp_cfg->cycle_end_irq);
	}

	if (fsp_cfg_extend->clear_source != CMTW_CLEAR_SOURCE_COMPARE_MATCH_CMWCOR) {
		counter_rz_cmtw_apply_clear_source(dev, CMTW_CLEAR_SOURCE_COMPARE_MATCH_CMWCOR);
	}

	err = counter_rz_cmtw_period_set(dev, top_cfg->ticks);
	if (err) {
		k_spin_unlock(&data->lock, key);
		return err;
	}

	/* Check if counter reset is required */
	reset = false;
	if (top_cfg->flags & COUNTER_TOP_CFG_DONT_RESET) {
		/* Don't reset counter */
		err = counter_rz_cmtw_get_value(dev, &cur_tick);
		if (err) {
			k_spin_unlock(&data->lock, key);
			return err;
		}

		if (cur_tick >= data->fsp_ctrl->period) {
			err = -ETIME;
			if (top_cfg->flags & COUNTER_TOP_CFG_RESET_WHEN_LATE) {
				/* Reset counter if current is late */
				reset = true;
			}
		}
	} else {
		reset = true;
	}

	if (reset) {
		err = cfg->fsp_api->reset(data->fsp_ctrl);
		if (err != FSP_SUCCESS) {
			k_spin_unlock(&data->lock, key);
			return -EIO;
		}
	}

	k_spin_unlock(&data->lock, key);

	return err;
}

static uint32_t counter_rz_cmtw_get_pending_int(const struct device *dev)
{
	struct counter_rz_cmtw_data *data = dev->data;

	return counter_rz_cmtw_is_pending(data->fsp_cfg->cycle_end_irq);
}

static uint32_t counter_rz_cmtw_get_top_value(const struct device *dev)
{
	struct counter_rz_cmtw_data *data = dev->data;

	return data->fsp_ctrl->period;
}

static uint32_t counter_rz_cmtw_get_guard_period(const struct device *dev, uint32_t flags)
{
	struct counter_rz_cmtw_data *data = dev->data;

	ARG_UNUSED(flags);

	return data->guard_period;
}

static int counter_rz_cmtw_set_guard_period(const struct device *dev, uint32_t guard,
					    uint32_t flags)
{
	struct counter_rz_cmtw_data *data = dev->data;

	ARG_UNUSED(flags);
	if (counter_rz_cmtw_get_top_value(dev) < guard) {
		return -EINVAL;
	}

	data->guard_period = guard;

	return 0;
}

static uint32_t counter_rz_cmtw_get_freq(const struct device *dev)
{
	struct counter_rz_cmtw_data *data = dev->data;
	const struct counter_rz_cmtw_config *cfg = dev->config;
	timer_info_t info;
	int err;

	err = cfg->fsp_api->infoGet(data->fsp_ctrl, &info);
	if (err != FSP_SUCCESS) {
		return 0;
	}

	return info.clock_frequency;
}

static DEVICE_API(counter, counter_rz_cmtw_driver_api) = {
	.start = counter_rz_cmtw_start,
	.stop = counter_rz_cmtw_stop,
	.get_value = counter_rz_cmtw_get_value,
	.set_alarm = counter_rz_cmtw_set_alarm,
	.cancel_alarm = counter_rz_cmtw_cancel_alarm,
	.set_top_value = counter_rz_cmtw_set_top_value,
	.get_pending_int = counter_rz_cmtw_get_pending_int,
	.get_top_value = counter_rz_cmtw_get_top_value,
	.get_guard_period = counter_rz_cmtw_get_guard_period,
	.set_guard_period = counter_rz_cmtw_set_guard_period,
	.get_freq = counter_rz_cmtw_get_freq,
};

void cmtw_cm_int_isr(void);

void counter_rz_cmtw_cmwi_isr(const struct device *dev)
{
	ARG_UNUSED(dev);
	cmtw_cm_int_isr();
}

#define RZ_CMTW(idx) DT_INST_PARENT(idx)

#ifdef CONFIG_CPU_CORTEX_M
#define RZ_CMTW_GET_IRQ_FLAGS(idx, irq_name) 0
#else /* Cortex-A/R */
#define RZ_CMTW_GET_IRQ_FLAGS(idx, irq_name) DT_IRQ_BY_NAME(RZ_CMTW(idx), irq_name, flags)
#endif

#define COUNTER_RZ_CMTW_INIT(inst)                                                                 \
	static cmtw_instance_ctrl_t g_timer##inst##_ctrl;                                          \
	static cmtw_extended_cfg_t g_timer##inst##_extend = {                                      \
		.clear_source = CMTW_CLEAR_SOURCE_DISABLED,                                        \
		.counter_size = TIMER_VARIANT_32_BIT,                                              \
	};                                                                                         \
	static timer_cfg_t g_timer##inst##_cfg = {                                                 \
		.mode = TIMER_MODE_PERIODIC,                                                       \
		.period_counts = (uint32_t)RZ_CMTW_TOP_VALUE,                                      \
		.source_div = DT_ENUM_IDX(RZ_CMTW(inst), prescaler),                               \
		.channel = DT_PROP(RZ_CMTW(inst), channel),                                        \
		.p_callback = counter_rz_cmtw_irq_handler,                                         \
		.p_context = DEVICE_DT_GET(DT_DRV_INST(inst)),                                     \
		.p_extend = &g_timer##inst##_extend,                                               \
		.cycle_end_ipl = DT_IRQ_BY_NAME(RZ_CMTW(inst), cmwi, priority),                    \
		.cycle_end_irq = DT_IRQ_BY_NAME(RZ_CMTW(inst), cmwi, irq),                         \
	};                                                                                         \
	static const struct counter_rz_cmtw_config counter_rz_cmtw_config_##inst = {               \
		.config_info =                                                                     \
			{                                                                          \
				.max_top_value = RZ_CMTW_TOP_VALUE,                                \
				.flags = COUNTER_CONFIG_INFO_COUNT_UP,                             \
				.channels = 1,                                                     \
			},                                                                         \
		.fsp_api = &g_timer_on_cmtw,                                                       \
	};                                                                                         \
	static struct counter_rz_cmtw_data counter_rz_cmtw_data_##inst = {                         \
		.fsp_cfg = &g_timer##inst##_cfg, .fsp_ctrl = &g_timer##inst##_ctrl};               \
	static int counter_rz_cmtw_init_##inst(const struct device *dev)                           \
	{                                                                                          \
		IRQ_CONNECT(DT_IRQ_BY_NAME(RZ_CMTW(inst), cmwi, irq),                              \
			    DT_IRQ_BY_NAME(RZ_CMTW(inst), cmwi, priority),                         \
			    counter_rz_cmtw_cmwi_isr, DEVICE_DT_INST_GET(inst),                    \
			    RZ_CMTW_GET_IRQ_FLAGS(inst, cmwi));                                    \
		return counter_rz_cmtw_init(dev);                                                  \
	}                                                                                          \
	DEVICE_DT_INST_DEFINE(inst, counter_rz_cmtw_init_##inst, NULL,                             \
			      &counter_rz_cmtw_data_##inst, &counter_rz_cmtw_config_##inst,        \
			      PRE_KERNEL_1, CONFIG_COUNTER_INIT_PRIORITY,                          \
			      &counter_rz_cmtw_driver_api);

DT_INST_FOREACH_STATUS_OKAY(COUNTER_RZ_CMTW_INIT)
