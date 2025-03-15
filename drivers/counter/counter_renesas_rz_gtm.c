/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rz_gtm_counter

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/counter.h>
#include <r_gtm.h>

#define RZ_GTM_TOP_VALUE UINT32_MAX

#define counter_rz_gtm_clear_pending(irq) NVIC_ClearPendingIRQ(irq)
#define counter_rz_gtm_set_pending(irq)   NVIC_SetPendingIRQ(irq)
#define counter_rz_gtm_is_pending(irq)    NVIC_GetPendingIRQ(irq)

struct counter_rz_gtm_config {
	struct counter_config_info config_info;
	const timer_api_t *fsp_api;
	uint8_t irqn;
};

struct counter_rz_gtm_data {
	timer_cfg_t *fsp_cfg;
	gtm_instance_ctrl_t *fsp_ctrl;
	/* top callback function */
	counter_top_callback_t top_cb;
	/* alarm callback function */
	counter_alarm_callback_t alarm_cb;
	void *user_data;
	uint32_t clk_freq;
	struct k_spinlock lock;
	uint32_t guard_period;
	uint32_t top_val;
	bool is_started;
	bool is_periodic;
};

static int counter_rz_gtm_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct counter_rz_gtm_config *cfg = dev->config;
	struct counter_rz_gtm_data *data = dev->data;
	timer_status_t timer_status;
	fsp_err_t err;

	err = cfg->fsp_api->statusGet(data->fsp_ctrl, &timer_status);
	if (err != FSP_SUCCESS) {
		return err;
	}
	uint32_t value = (uint32_t)timer_status.counter;
	*ticks = value;

	return 0;
}

static void counter_rz_gtm_irq_handler(timer_callback_args_t *p_args)
{
	const struct device *dev = p_args->p_context;
	struct counter_rz_gtm_data *data = dev->data;
	counter_alarm_callback_t alarm_callback = data->alarm_cb;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);

	if (alarm_callback) {
		uint32_t now;

		counter_rz_gtm_get_value(dev, &now);
		data->alarm_cb = NULL;
		alarm_callback(dev, 0, now, data->user_data);
	} else if (data->top_cb) {
		data->top_cb(dev, data->user_data);
	}

	k_spin_unlock(&data->lock, key);
}

static int counter_rz_gtm_init(const struct device *dev)
{
	struct counter_rz_gtm_data *data = dev->data;
	const struct counter_rz_gtm_config *cfg = dev->config;
	int err;

	data->top_val = data->fsp_cfg->period_counts;

	err = cfg->fsp_api->open(data->fsp_ctrl, data->fsp_cfg);
	if (err != FSP_SUCCESS) {
		return err;
	}
	return err;
}

static void counter_rz_gtm_start_freerun(const struct device *dev)
{
	/* enable counter in free running mode */
	const struct counter_rz_gtm_config *cfg = dev->config;
	struct counter_rz_gtm_data *data = dev->data;

	gtm_extended_cfg_t *fsp_cfg_extend = (gtm_extended_cfg_t *)data->fsp_cfg->p_extend;

	fsp_cfg_extend->gtm_mode = GTM_TIMER_MODE_FREERUN;
	cfg->fsp_api->close(data->fsp_ctrl);
	cfg->fsp_api->open(data->fsp_ctrl, data->fsp_cfg);
	cfg->fsp_api->start(data->fsp_ctrl);
}

static void counter_rz_gtm_start_interval(const struct device *dev)
{
	/* start timer in interval mode */
	const struct counter_rz_gtm_config *cfg = dev->config;
	struct counter_rz_gtm_data *data = dev->data;

	gtm_extended_cfg_t *fsp_cfg_extend = (gtm_extended_cfg_t *)data->fsp_cfg->p_extend;

	fsp_cfg_extend->gtm_mode = GTM_TIMER_MODE_INTERVAL;
	cfg->fsp_api->close(data->fsp_ctrl);
	cfg->fsp_api->open(data->fsp_ctrl, data->fsp_cfg);
	cfg->fsp_api->start(data->fsp_ctrl);
}

static int counter_rz_gtm_start(const struct device *dev)
{
	const struct counter_rz_gtm_config *cfg = dev->config;
	struct counter_rz_gtm_data *data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);

	if (data->is_started) {
		k_spin_unlock(&data->lock, key);
		return -EALREADY;
	}

	if (data->is_periodic) {
		data->fsp_cfg->period_counts = data->top_val;
		counter_rz_gtm_start_interval(dev);
	} else {
		counter_rz_gtm_start_freerun(dev);
	}

	counter_rz_gtm_clear_pending(cfg->irqn);
	data->is_started = true;
	if (data->top_cb) {
		irq_enable(cfg->irqn);
	}

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int counter_rz_gtm_stop(const struct device *dev)
{
	const struct counter_rz_gtm_config *cfg = dev->config;
	struct counter_rz_gtm_data *data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);

	if (!data->is_started) {
		k_spin_unlock(&data->lock, key);
		return 0;
	}

	fsp_err_t err = FSP_SUCCESS;

	/* Stop timer */
	err = cfg->fsp_api->stop(data->fsp_ctrl);

	/* dis irq */
	irq_disable(cfg->irqn);
	counter_rz_gtm_clear_pending(cfg->irqn);

	data->top_cb = NULL;
	data->alarm_cb = NULL;
	data->user_data = NULL;

	data->is_started = false;

	k_spin_unlock(&data->lock, key);

	return err;
}

static int counter_rz_gtm_set_alarm(const struct device *dev, uint8_t chan,
				    const struct counter_alarm_cfg *alarm_cfg)
{
	const struct counter_rz_gtm_config *cfg = dev->config;
	struct counter_rz_gtm_data *data = dev->data;

	bool absolute = alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE;
	uint32_t val = alarm_cfg->ticks;
	k_spinlock_key_t key;
	bool irq_on_late;
	uint32_t max_rel_val;
	uint32_t now, diff;
	int err = 0;

	if (!alarm_cfg) {
		return -EINVAL;
	}
	/* Alarm callback is mandatory */
	if (!alarm_cfg->callback) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	if (!data->is_started) {
		k_spin_unlock(&data->lock, key);
		return -EINVAL;
	}

	/** Alarm_cb need equal NULL before */
	if (data->alarm_cb) {
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}

	/** Timer is currently in interval mode*/
	if (data->is_periodic) {
		/** return error because val exceeded the limit set alarm */
		if (val > data->fsp_cfg->period_counts) {
			k_spin_unlock(&data->lock, key);
			return -EINVAL;
		}

		/* restore free running mode */
		irq_disable(cfg->irqn);
		data->top_cb = NULL;
		data->alarm_cb = alarm_cfg->callback;
		data->user_data = NULL;
		data->top_val = RZ_GTM_TOP_VALUE;
		data->is_periodic = false;

		if (data->is_started) {
			data->fsp_cfg->period_counts = data->top_val;
			counter_rz_gtm_start_freerun(dev);
		}
	}

	counter_rz_gtm_get_value(dev, &now);
	data->alarm_cb = alarm_cfg->callback;
	data->user_data = alarm_cfg->user_data;

	if (absolute) {
		max_rel_val = RZ_GTM_TOP_VALUE - data->guard_period;
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
		irq_on_late = val < (RZ_GTM_TOP_VALUE / 2U);
		/* limit max to detect short relative being set too late. */
		max_rel_val = irq_on_late ? RZ_GTM_TOP_VALUE / 2U : RZ_GTM_TOP_VALUE;
		val = (now + val) & RZ_GTM_TOP_VALUE;
	}

	/** Set new period */
	data->fsp_cfg->period_counts = val;
	err = cfg->fsp_api->periodSet(data->fsp_ctrl, data->fsp_cfg->period_counts);
	if (err != FSP_SUCCESS) {
		k_spin_unlock(&data->lock, key);
		return err;
	}

	uint32_t read_counter_again = 0;

	counter_rz_gtm_get_value(dev, &read_counter_again);
	diff = ((val - 1U) - read_counter_again) & RZ_GTM_TOP_VALUE;
	if (diff > max_rel_val) {
		if (absolute) {
			err = -ETIME;
		}

		/* Interrupt is triggered always for relative alarm and
		 * for absolute depending on the flag.
		 */
		if (irq_on_late) {
			irq_enable(cfg->irqn);
			counter_rz_gtm_set_pending(cfg->irqn);
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
			irq_enable(cfg->irqn);
			counter_rz_gtm_set_pending(cfg->irqn);
		} else {
			counter_rz_gtm_clear_pending(cfg->irqn);
			irq_enable(cfg->irqn);
		}
	}

	k_spin_unlock(&data->lock, key);

	return err;
}

static int counter_rz_gtm_cancel_alarm(const struct device *dev, uint8_t chan)
{
	const struct counter_rz_gtm_config *cfg = dev->config;
	struct counter_rz_gtm_data *data = dev->data;
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

	irq_disable(cfg->irqn);
	counter_rz_gtm_clear_pending(cfg->irqn);
	data->alarm_cb = NULL;
	data->user_data = NULL;

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int counter_rz_gtm_set_top_value(const struct device *dev,
					const struct counter_top_cfg *top_cfg)
{
	const struct counter_rz_gtm_config *cfg = dev->config;
	struct counter_rz_gtm_data *data = dev->data;
	k_spinlock_key_t key;
	uint32_t cur_tick;
	int ret = 0;

	if (!top_cfg) {
		return -EINVAL;
	}

	/** -EBUSY if any alarm is active */
	if (data->alarm_cb) {
		return -EBUSY;
	}

	key = k_spin_lock(&data->lock);

	if (!data->is_periodic && top_cfg->ticks == RZ_GTM_TOP_VALUE) {
		goto exit_unlock;
	}

	if (top_cfg->ticks == RZ_GTM_TOP_VALUE) {
		/* restore free running mode */
		irq_disable(cfg->irqn);
		counter_rz_gtm_clear_pending(cfg->irqn);
		data->top_cb = NULL;
		data->user_data = NULL;
		data->top_val = RZ_GTM_TOP_VALUE;
		data->is_periodic = false;

		if (data->is_started) {
			counter_rz_gtm_start_freerun(dev);
			counter_rz_gtm_clear_pending(cfg->irqn);
		}
		goto exit_unlock;
	}

	data->top_cb = top_cfg->callback;
	data->user_data = top_cfg->user_data;
	data->top_val = top_cfg->ticks;

	if (!data->is_started) {
		data->is_periodic = true;
		goto exit_unlock;
	}

	if (!data->is_periodic) {
		/* switch to interval mode first time, restart timer */
		ret = cfg->fsp_api->stop(data->fsp_ctrl);
		irq_disable(cfg->irqn);
		data->is_periodic = true;
		data->fsp_cfg->period_counts = data->top_val;
		counter_rz_gtm_start_interval(dev);

		if (data->top_cb) {
			counter_rz_gtm_clear_pending(cfg->irqn);
			irq_enable(cfg->irqn);
		}
		goto exit_unlock;
	}

	if (!data->top_cb) {
		/* new top cfg is without callback - stop IRQs */
		irq_disable(cfg->irqn);
		counter_rz_gtm_clear_pending(cfg->irqn);
	}
	/* timer already in interval mode - only change top value */
	data->fsp_cfg->period_counts = data->top_val;
	cfg->fsp_api->periodSet(&data->fsp_ctrl, data->fsp_cfg->period_counts);

	/* check if counter reset is required */
	if (top_cfg->flags & COUNTER_TOP_CFG_DONT_RESET) {
		/* Don't reset counter */
		counter_rz_gtm_get_value(dev, &cur_tick);

		if (cur_tick >= data->top_val) {
			ret = -ETIME;
			if (top_cfg->flags & COUNTER_TOP_CFG_RESET_WHEN_LATE) {
				/* Reset counter if current is late */
				cfg->fsp_api->stop(data->fsp_ctrl);
				cfg->fsp_api->start(data->fsp_ctrl);
			}
		}
	} else {
		/* reset counter */
		cfg->fsp_api->stop(data->fsp_ctrl);
		cfg->fsp_api->start(data->fsp_ctrl);
	}

exit_unlock:
	k_spin_unlock(&data->lock, key);
	return ret;
}

static uint32_t counter_rz_gtm_get_pending_int(const struct device *dev)
{
	const struct counter_rz_gtm_config *cfg = dev->config;

	/* There is no register to check TIMER peripheral to check for interrupt
	 * pending, check directly in NVIC.
	 */
	return counter_rz_gtm_is_pending(cfg->irqn);
}

static uint32_t counter_rz_gtm_get_top_value(const struct device *dev)
{
	struct counter_rz_gtm_data *data = dev->data;
	const struct counter_rz_gtm_config *cfg = dev->config;
	uint32_t top_val = RZ_GTM_TOP_VALUE;

	if (data->is_periodic) {
		timer_info_t info;

		cfg->fsp_api->infoGet(data->fsp_ctrl, &info);
		top_val = info.period_counts;
	}

	return top_val;
}
static uint32_t counter_rz_gtm_get_guard_period(const struct device *dev, uint32_t flags)
{
	struct counter_rz_gtm_data *data = dev->data;

	ARG_UNUSED(flags);
	return data->guard_period;
}

static int counter_rz_gtm_set_guard_period(const struct device *dev, uint32_t guard, uint32_t flags)
{
	struct counter_rz_gtm_data *data = dev->data;

	ARG_UNUSED(flags);
	__ASSERT_NO_MSG(guard < counter_rz_gtm_get_top_value(dev));

	data->guard_period = guard;

	return 0;
}

static uint32_t counter_rz_gtm_get_freq(const struct device *dev)
{
	struct counter_rz_gtm_data *data = dev->data;
	const struct counter_rz_gtm_config *cfg = dev->config;
	timer_info_t info;

	cfg->fsp_api->infoGet(data->fsp_ctrl, &info);

	return info.clock_frequency;
}

static DEVICE_API(counter, counter_rz_gtm_driver_api) = {
	.start = counter_rz_gtm_start,
	.stop = counter_rz_gtm_stop,
	.get_value = counter_rz_gtm_get_value,
	.set_alarm = counter_rz_gtm_set_alarm,
	.cancel_alarm = counter_rz_gtm_cancel_alarm,
	.set_top_value = counter_rz_gtm_set_top_value,
	.get_pending_int = counter_rz_gtm_get_pending_int,
	.get_top_value = counter_rz_gtm_get_top_value,
	.get_guard_period = counter_rz_gtm_get_guard_period,
	.set_guard_period = counter_rz_gtm_set_guard_period,
	.get_freq = counter_rz_gtm_get_freq,
};

extern void gtm_int_isr(void);

#define GTM(idx) DT_INST_PARENT(idx)

#define COUNTER_RZG_GTM_INIT(inst)                                                                 \
	static gtm_instance_ctrl_t g_timer##inst##_ctrl;                                           \
	static gtm_extended_cfg_t g_timer##inst##_extend = {                                       \
		.generate_interrupt_when_starts = GTM_GIWS_TYPE_DISABLED,                          \
		.gtm_mode = GTM_TIMER_MODE_FREERUN,                                                \
	};                                                                                         \
	static timer_cfg_t g_timer##inst##_cfg = {                                                 \
		.mode = TIMER_MODE_PERIODIC,                                                       \
		.period_counts = (uint32_t)RZ_GTM_TOP_VALUE,                                       \
		.channel = DT_PROP(GTM(inst), channel),                                            \
		.p_callback = counter_rz_gtm_irq_handler,                                          \
		.p_context = DEVICE_DT_GET(DT_DRV_INST(inst)),                                     \
		.p_extend = &g_timer##inst##_extend,                                               \
		.cycle_end_ipl = DT_IRQ_BY_NAME(GTM(inst), overflow, priority),                    \
		.cycle_end_irq = DT_IRQ_BY_NAME(GTM(inst), overflow, irq),                         \
	};                                                                                         \
	static const struct counter_rz_gtm_config counter_rz_gtm_config_##inst = {                 \
		.config_info =                                                                     \
			{                                                                          \
				.max_top_value = RZ_GTM_TOP_VALUE,                                 \
				.flags = COUNTER_CONFIG_INFO_COUNT_UP,                             \
				.channels = 1,                                                     \
			},                                                                         \
		.fsp_api = &g_timer_on_gtm,                                                        \
		.irqn = DT_IRQ_BY_NAME(GTM(inst), overflow, irq),                                  \
	};                                                                                         \
	static struct counter_rz_gtm_data counter_rz_gtm_data_##inst = {                           \
		.fsp_cfg = &g_timer##inst##_cfg, .fsp_ctrl = &g_timer##inst##_ctrl};               \
	static int counter_rz_gtm_init_##inst(const struct device *dev)                            \
	{                                                                                          \
		IRQ_CONNECT(DT_IRQ_BY_NAME(GTM(inst), overflow, irq),                              \
			    DT_IRQ_BY_NAME(GTM(inst), overflow, priority), gtm_int_isr,            \
			    DEVICE_DT_INST_GET(inst), 0);                                          \
		return counter_rz_gtm_init(dev);                                                   \
	}                                                                                          \
	DEVICE_DT_INST_DEFINE(inst, counter_rz_gtm_init_##inst, NULL, &counter_rz_gtm_data_##inst, \
			      &counter_rz_gtm_config_##inst, PRE_KERNEL_1,                         \
			      CONFIG_COUNTER_INIT_PRIORITY, &counter_rz_gtm_driver_api);

DT_INST_FOREACH_STATUS_OKAY(COUNTER_RZG_GTM_INIT)
