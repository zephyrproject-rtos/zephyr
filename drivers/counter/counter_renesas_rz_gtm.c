/*
 * Copyright (c) 2024-2025 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rz_gtm_counter

#include <r_gtm.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/interrupt_controller/gic.h>

#define RZ_GTM_TOP_VALUE UINT32_MAX

struct counter_rz_gtm_config {
	struct counter_config_info config_info;
	const timer_api_t *fsp_api;
};

struct counter_rz_gtm_data {
	timer_cfg_t *fsp_cfg;
	gtm_instance_ctrl_t *fsp_ctrl;
	/* Top callback function */
	counter_top_callback_t top_cb;
	/* Alarm callback function */
	counter_alarm_callback_t alarm_cb;
	void *user_data;
	uint32_t clk_freq;
	struct k_spinlock lock;
	uint32_t guard_period;
	uint32_t top_val;
	bool is_started;
	bool is_periodic;
};

static inline void counter_rz_gtm_clear_pending(unsigned int irq)
{
#if defined(CONFIG_GIC)
	arm_gic_irq_clear_pending(irq);
#else  /* NVIC */
	NVIC_ClearPendingIRQ(irq);
#endif /* CONFIG_GIC */
}

static inline void counter_rz_gtm_set_pending(unsigned int irq)
{
#if defined(CONFIG_GIC)
	arm_gic_irq_set_pending(irq);
#else  /* NVIC */
	NVIC_SetPendingIRQ(irq);
#endif /* CONFIG_GIC */
}

static inline uint32_t counter_rz_gtm_is_pending(unsigned int irq)
{
#if defined(CONFIG_GIC)
	return arm_gic_irq_is_pending(irq);
#else  /* NVIC */
	return NVIC_GetPendingIRQ(irq);
#endif /* CONFIG_GIC */
}

static int counter_rz_gtm_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct counter_rz_gtm_config *cfg = dev->config;
	struct counter_rz_gtm_data *data = dev->data;
	timer_status_t timer_status;
	fsp_err_t err;

	err = cfg->fsp_api->statusGet(data->fsp_ctrl, &timer_status);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}
	*ticks = (uint32_t)timer_status.counter;

	return 0;
}

static void counter_rz_gtm_irq_handler(timer_callback_args_t *p_args)
{
	const struct device *dev = p_args->p_context;
	struct counter_rz_gtm_data *data = dev->data;
	counter_alarm_callback_t alarm_callback;
	counter_top_callback_t top_callback;
	void *usr_data;
	uint32_t now;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);

	alarm_callback = data->alarm_cb;
	top_callback = data->top_cb;
	usr_data = data->user_data;

	if (alarm_callback) {
		data->alarm_cb = NULL;
		data->user_data = NULL;
	}

	k_spin_unlock(&data->lock, key);

	if (alarm_callback) {

		if (counter_rz_gtm_get_value(dev, &now) != 0) {
			return;
		}

		alarm_callback(dev, 0, now, usr_data);
	} else if (data->top_cb) {
		top_callback(dev, usr_data);
	} else {
		/* Do nothing */
	}
}

static int counter_rz_gtm_init(const struct device *dev)
{
	struct counter_rz_gtm_data *data = dev->data;
	const struct counter_rz_gtm_config *cfg = dev->config;
	fsp_err_t err;

	data->top_val = data->fsp_cfg->period_counts;

	err = cfg->fsp_api->open(data->fsp_ctrl, data->fsp_cfg);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	data->is_periodic = false;

	return 0;
}

static int renesas_rz_gtm_period_set(const struct device *dev, uint32_t val)
{
	const struct counter_rz_gtm_config *cfg = dev->config;
	struct counter_rz_gtm_data *data = dev->data;
	fsp_err_t err;

	data->fsp_cfg->period_counts = val;
	err = cfg->fsp_api->periodSet(data->fsp_ctrl, data->fsp_cfg->period_counts);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static int renesas_rz_gtm_switch_timer_mode(const struct device *dev)
{
	const struct counter_rz_gtm_config *cfg = dev->config;
	struct counter_rz_gtm_data *data = dev->data;
	gtm_extended_cfg_t *fsp_cfg_extend = (gtm_extended_cfg_t *)data->fsp_cfg->p_extend;
	fsp_err_t err;

	if (data->is_periodic) {
		fsp_cfg_extend->gtm_mode = GTM_TIMER_MODE_INTERVAL;
	} else {
		data->top_cb = NULL;
		data->top_val = RZ_GTM_TOP_VALUE;
		fsp_cfg_extend->gtm_mode = GTM_TIMER_MODE_FREERUN;
	}

	data->fsp_cfg->period_counts = data->top_val;

	err = cfg->fsp_api->close(data->fsp_ctrl);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	err = cfg->fsp_api->open(data->fsp_ctrl, data->fsp_cfg);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	err = cfg->fsp_api->start(data->fsp_ctrl);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static int counter_rz_gtm_start(const struct device *dev)
{
	struct counter_rz_gtm_data *data = dev->data;
	k_spinlock_key_t key;
	int err;

	key = k_spin_lock(&data->lock);

	if (data->is_started) {
		k_spin_unlock(&data->lock, key);
		return -EALREADY;
	}

	if (data->is_periodic) {
		data->fsp_cfg->period_counts = data->top_val;
	}

	err = renesas_rz_gtm_switch_timer_mode(dev);
	if (err < 0) {
		k_spin_unlock(&data->lock, key);
		return err;
	}

	counter_rz_gtm_clear_pending(data->fsp_cfg->cycle_end_irq);
	data->is_started = true;
	if (data->top_cb) {
		irq_enable(data->fsp_cfg->cycle_end_irq);
	}

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int counter_rz_gtm_stop(const struct device *dev)
{
	const struct counter_rz_gtm_config *cfg = dev->config;
	struct counter_rz_gtm_data *data = dev->data;
	k_spinlock_key_t key;
	fsp_err_t err;

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
	counter_rz_gtm_clear_pending(data->fsp_cfg->cycle_end_irq);

	data->top_cb = NULL;
	data->alarm_cb = NULL;
	data->user_data = NULL;
	data->is_started = false;

	k_spin_unlock(&data->lock, key);

	return 0;
}

static uint32_t ticks_sub(uint32_t val, uint32_t old)
{
	if (val >= old) {
		return (val - old);
	} else {
		return (val + (RZ_GTM_TOP_VALUE - old + 1));
	}
}

static int renesas_rz_gtm_abs_alarm_set(const struct device *dev, uint32_t val, bool irq_on_late)
{
	struct counter_rz_gtm_data *data = dev->data;
	uint32_t max_rel_val;
	uint32_t read_again;
	uint32_t diff;
	int err;

	/* Set new period */
	err = renesas_rz_gtm_period_set(dev, val);
	if (err < 0) {
		return err;
	}

	err = counter_rz_gtm_get_value(dev, &read_again);
	if (err < 0) {
		return err;
	}

	max_rel_val = RZ_GTM_TOP_VALUE - data->guard_period;
	diff = ticks_sub(val, read_again);
	if (diff > max_rel_val || diff == 0) {
		err = -ETIME;

		if (irq_on_late) {
			irq_enable(data->fsp_cfg->cycle_end_irq);
			counter_rz_gtm_set_pending(data->fsp_cfg->cycle_end_irq);
		} else {
			data->alarm_cb = NULL;
		}
	} else {
		counter_rz_gtm_clear_pending(data->fsp_cfg->cycle_end_irq);
		irq_enable(data->fsp_cfg->cycle_end_irq);
	}

	return err;
}

static int renesas_rz_gtm_rel_alarm_set(const struct device *dev, uint32_t val, bool irq_on_late)
{
	struct counter_rz_gtm_data *data = dev->data;
	uint32_t max_rel_val;
	uint32_t read_again;
	uint32_t diff;
	uint32_t now;
	int err;

	err = counter_rz_gtm_get_value(dev, &now);
	if (err < 0) {
		return err;
	}

	val = (now + val) & RZ_GTM_TOP_VALUE;

	/* Set new period */
	err = renesas_rz_gtm_period_set(dev, val);
	if (err < 0) {
		return err;
	}

	err = counter_rz_gtm_get_value(dev, &read_again);
	if (err < 0) {
		return err;
	}

	max_rel_val = irq_on_late ? RZ_GTM_TOP_VALUE / 2U : RZ_GTM_TOP_VALUE;
	diff = ticks_sub(val, read_again);
	if (diff > max_rel_val || diff == 0) {
		if (irq_on_late) {
			irq_enable(data->fsp_cfg->cycle_end_irq);
			counter_rz_gtm_set_pending(data->fsp_cfg->cycle_end_irq);
		} else {
			data->alarm_cb = NULL;
		}
	} else {
		counter_rz_gtm_clear_pending(data->fsp_cfg->cycle_end_irq);
		irq_enable(data->fsp_cfg->cycle_end_irq);
	}

	return err;
}

static int counter_rz_gtm_set_alarm(const struct device *dev, uint8_t chan,
				    const struct counter_alarm_cfg *alarm_cfg)
{
	struct counter_rz_gtm_data *data = dev->data;

	bool absolute;
	uint32_t val;
	k_spinlock_key_t key;
	bool irq_on_late;
	int err;

	if (chan != 0) {
		return -EINVAL;
	}

	if (!alarm_cfg) {
		return -EINVAL;
	}

	if (!alarm_cfg->callback) {
		return -EINVAL;
	}

	absolute = alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE;
	val = alarm_cfg->ticks;

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

	/* Timer is currently in interval mode */
	if (data->is_periodic) {
		/* Return error because val exceeded the limit set alarm */
		if (val > data->fsp_cfg->period_counts) {
			k_spin_unlock(&data->lock, key);
			return -EINVAL;
		}

		/* Restore free running mode */
		data->is_periodic = false;
		err = renesas_rz_gtm_switch_timer_mode(dev);
		if (err < 0) {
			k_spin_unlock(&data->lock, key);
			return err;
		}
	}

	data->alarm_cb = alarm_cfg->callback;
	data->user_data = alarm_cfg->user_data;

	if (absolute) {
		irq_on_late = alarm_cfg->flags & COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE;

		err = renesas_rz_gtm_abs_alarm_set(dev, val, irq_on_late);
		if (err < 0) {
			k_spin_unlock(&data->lock, key);
			return err;
		}
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

		err = renesas_rz_gtm_rel_alarm_set(dev, val, irq_on_late);
		if (err < 0) {
			k_spin_unlock(&data->lock, key);
			return err;
		}
	}

	k_spin_unlock(&data->lock, key);

	return err;
}

static int counter_rz_gtm_cancel_alarm(const struct device *dev, uint8_t chan)
{
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

	irq_disable(data->fsp_cfg->cycle_end_irq);
	counter_rz_gtm_clear_pending(data->fsp_cfg->cycle_end_irq);
	data->alarm_cb = NULL;
	data->user_data = NULL;

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int renesas_rz_gtm_check_reset_if_late(const struct device *dev, uint32_t flags)
{
	const struct counter_rz_gtm_config *cfg = dev->config;
	struct counter_rz_gtm_data *data = dev->data;
	uint32_t curr_tick;
	bool reset;
	int err;

	reset = false;
	err = 0;

	if (flags & COUNTER_TOP_CFG_DONT_RESET) {
		/* Don't reset counter */
		err = counter_rz_gtm_get_value(dev, &curr_tick);
		if (err < 0) {
			return err;
		}

		if (curr_tick >= data->top_val) {
			err = -ETIME;
			if (flags & COUNTER_TOP_CFG_RESET_WHEN_LATE) {
				/* Reset counter if current is late */
				reset = true;
			}
		}
	} else {
		reset = true;
	}

	if (reset) {
		if (cfg->fsp_api->reset(data->fsp_ctrl) != FSP_SUCCESS) {
			return -EIO;
		}
	}

	return err;
}
static int counter_rz_gtm_set_top_value(const struct device *dev,
					const struct counter_top_cfg *top_cfg)
{
	struct counter_rz_gtm_data *data = dev->data;
	k_spinlock_key_t key;
	int err = 0;

	if (!top_cfg) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	/* -EBUSY if any alarm is active */
	if (data->alarm_cb) {
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}

	data->top_cb = top_cfg->callback;
	data->user_data = top_cfg->user_data;
	data->top_val = top_cfg->ticks;

	if (!data->is_periodic && top_cfg->ticks == RZ_GTM_TOP_VALUE) {
		k_spin_unlock(&data->lock, key);
		return err;
	}

	if (top_cfg->ticks == RZ_GTM_TOP_VALUE) {
		/* Restore free running mode */
		data->user_data = NULL;
		data->is_periodic = false;
		if (data->is_started) {
			err = renesas_rz_gtm_switch_timer_mode(dev);
			if (err < 0) {
				k_spin_unlock(&data->lock, key);
				return err;
			}
		}

		k_spin_unlock(&data->lock, key);
		return err;
	}

	if (!data->is_started) {
		data->is_periodic = true;

		k_spin_unlock(&data->lock, key);
		return err;
	}

	if (!data->is_periodic) {
		/* Switch to interval mode first time, restart timer */
		data->is_periodic = true;
		err = renesas_rz_gtm_switch_timer_mode(dev);
		if (err < 0) {
			k_spin_unlock(&data->lock, key);
			return err;
		}

		if (data->top_cb) {
			irq_enable(data->fsp_cfg->cycle_end_irq);
		}

		k_spin_unlock(&data->lock, key);
		return err;
	}

	if (!data->top_cb) {
		/* New top cfg is without callback - stop IRQs */
		irq_disable(data->fsp_cfg->cycle_end_irq);
	}

	/* Timer already in interval mode - only change top value */
	err = renesas_rz_gtm_period_set(dev, data->top_val);
	if (err < 0) {
		k_spin_unlock(&data->lock, key);
		return err;
	}

	/* Check if counter reset is required */
	err = renesas_rz_gtm_check_reset_if_late(dev, top_cfg->flags);

	k_spin_unlock(&data->lock, key);

	return err;
}

static uint32_t counter_rz_gtm_get_pending_int(const struct device *dev)
{
	struct counter_rz_gtm_data *data = dev->data;

	return counter_rz_gtm_is_pending(data->fsp_cfg->cycle_end_irq);
}

static uint32_t counter_rz_gtm_get_top_value(const struct device *dev)
{
	struct counter_rz_gtm_data *data = dev->data;
	const struct counter_rz_gtm_config *cfg = dev->config;
	uint32_t top_val = RZ_GTM_TOP_VALUE;

	if (data->is_periodic) {
		timer_info_t info;
		fsp_err_t err;

		err = cfg->fsp_api->infoGet(data->fsp_ctrl, &info);
		if (err != FSP_SUCCESS) {
			return -EIO;
		}
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
	if (counter_rz_gtm_get_top_value(dev) < guard) {
		return -EINVAL;
	}

	data->guard_period = guard;

	return 0;
}

static uint32_t counter_rz_gtm_get_freq(const struct device *dev)
{
	struct counter_rz_gtm_data *data = dev->data;
	const struct counter_rz_gtm_config *cfg = dev->config;
	timer_info_t info;
	fsp_err_t err;

	err = cfg->fsp_api->infoGet(data->fsp_ctrl, &info);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

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

void gtm_int_isr(IRQn_Type const irq);

void counter_rz_gtm_ovf_isr(const struct device *dev)
{
	struct counter_rz_gtm_data *data = dev->data;

	gtm_int_isr(data->fsp_cfg->cycle_end_irq);
}

#define RZ_GTM(idx) DT_INST_PARENT(idx)

#ifdef CONFIG_CPU_CORTEX_M
#define RZ_GTM_GET_IRQ_FLAGS(idx, irq_name) 0
#else /* Cortex-A/R */
#define RZ_GTM_GET_IRQ_FLAGS(idx, irq_name) DT_IRQ_BY_NAME(RZ_GTM(idx), irq_name, flags)
#endif

#define COUNTER_RZ_GTM_INIT(inst)                                                                  \
	static gtm_instance_ctrl_t g_timer##inst##_ctrl;                                           \
	static gtm_extended_cfg_t g_timer##inst##_extend = {                                       \
		.generate_interrupt_when_starts = GTM_GIWS_TYPE_DISABLED,                          \
		.gtm_mode = GTM_TIMER_MODE_FREERUN,                                                \
	};                                                                                         \
	static timer_cfg_t g_timer##inst##_cfg = {                                                 \
		.mode = TIMER_MODE_PERIODIC,                                                       \
		.period_counts = (uint32_t)RZ_GTM_TOP_VALUE,                                       \
		.channel = DT_PROP(RZ_GTM(inst), channel),                                         \
		.p_callback = counter_rz_gtm_irq_handler,                                          \
		.p_context = DEVICE_DT_GET(DT_DRV_INST(inst)),                                     \
		.p_extend = &g_timer##inst##_extend,                                               \
		.cycle_end_ipl = DT_IRQ_BY_NAME(RZ_GTM(inst), overflow, priority),                 \
		.cycle_end_irq = DT_IRQ_BY_NAME(RZ_GTM(inst), overflow, irq),                      \
	};                                                                                         \
	static const struct counter_rz_gtm_config counter_rz_gtm_config_##inst = {                 \
		.config_info =                                                                     \
			{                                                                          \
				.max_top_value = RZ_GTM_TOP_VALUE,                                 \
				.flags = COUNTER_CONFIG_INFO_COUNT_UP,                             \
				.channels = 1,                                                     \
			},                                                                         \
		.fsp_api = &g_timer_on_gtm,                                                        \
	};                                                                                         \
	static struct counter_rz_gtm_data counter_rz_gtm_data_##inst = {                           \
		.fsp_cfg = &g_timer##inst##_cfg, .fsp_ctrl = &g_timer##inst##_ctrl};               \
	static int counter_rz_gtm_init_##inst(const struct device *dev)                            \
	{                                                                                          \
		IRQ_CONNECT(DT_IRQ_BY_NAME(RZ_GTM(inst), overflow, irq),                           \
			    DT_IRQ_BY_NAME(RZ_GTM(inst), overflow, priority),                      \
			    counter_rz_gtm_ovf_isr, DEVICE_DT_INST_GET(inst),                      \
			    RZ_GTM_GET_IRQ_FLAGS(inst, overflow));                                 \
		return counter_rz_gtm_init(dev);                                                   \
	}                                                                                          \
	DEVICE_DT_INST_DEFINE(inst, counter_rz_gtm_init_##inst, NULL, &counter_rz_gtm_data_##inst, \
			      &counter_rz_gtm_config_##inst, PRE_KERNEL_1,                         \
			      CONFIG_COUNTER_INIT_PRIORITY, &counter_rz_gtm_driver_api);

DT_INST_FOREACH_STATUS_OKAY(COUNTER_RZ_GTM_INIT)
