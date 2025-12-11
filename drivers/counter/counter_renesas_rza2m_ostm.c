/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rza2m_ostm_counter

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/interrupt_controller/gic.h>
#include <zephyr/drivers/clock_control.h>

#define RZA2M_OSTM_TOP_VALUE UINT32_MAX

#define RZA2M_OSTMnCMP_OFFSET 0x00
#define RZA2M_OSTMnCNT_OFFSET 0x04
#define RZA2M_OSTMnTS_OFFSET  0x14
#define RZA2M_OSTMnTT_OFFSET  0x18
#define RZA2M_OSTMnCTL_OFFSET 0x20

#define RZA2M_OSTMnCTL_FREERUN_MODE  0x02
#define RZA2M_OSTMnCTL_INTERVAL_MODE 0x00

#define DEV_CFG(_dev)  ((const struct counter_rza2m_ostm_config *)(_dev)->config)
#define DEV_DATA(_dev) ((struct counter_rza2m_ostm_data *const)(_dev)->data)

struct counter_rza2m_ostm_config {
	struct counter_config_info config_info; /* Must be first */

	DEVICE_MMIO_NAMED_ROM(counter_ostm_mmio);

	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
};

struct counter_rza2m_ostm_data {
	DEVICE_MMIO_NAMED_RAM(counter_ostm_mmio);
	/* Top callback function */
	counter_top_callback_t top_cb;
	/* Alarm callback function */
	counter_alarm_callback_t alarm_cb;

	void *user_data;

	struct k_spinlock lock;
	uint32_t guard_period;
	uint32_t top_val;
	bool is_started;
	bool is_periodic;

	uint32_t clk_rate;

	uint8_t channel;
	uint32_t period_counts;
	uint32_t cycle_end_irq;
};

static void renesas_rza2m_ostm_write_8(const struct device *dev, uint32_t offset, uint8_t value)
{
	sys_write8(value, DEVICE_MMIO_NAMED_GET(dev, counter_ostm_mmio) + offset);
}

static uint32_t renesas_rza2m_ostm_read_32(const struct device *dev, uint32_t offset)
{
	return sys_read32(DEVICE_MMIO_NAMED_GET(dev, counter_ostm_mmio) + offset);
}

static void renesas_rza2m_ostm_write_32(const struct device *dev, uint32_t offset, uint32_t value)
{
	sys_write32(value, DEVICE_MMIO_NAMED_GET(dev, counter_ostm_mmio) + offset);
}

static void renesas_rza2m_ostm_period_set(const struct device *dev, uint32_t val)
{
	renesas_rza2m_ostm_write_32(dev, RZA2M_OSTMnCMP_OFFSET, val);
}

static void renesas_rza2m_ostm_switch_timer_mode(const struct device *dev)
{
	struct counter_rza2m_ostm_data *data = dev->data;

	/* Stop */
	renesas_rza2m_ostm_write_8(dev, RZA2M_OSTMnTT_OFFSET, 1);

	/* Open */
	if (data->is_periodic) {
		renesas_rza2m_ostm_write_8(dev, RZA2M_OSTMnCTL_OFFSET,
					   RZA2M_OSTMnCTL_INTERVAL_MODE);
	} else {
		/* Free running mode */
		data->top_cb = NULL;
		data->top_val = RZA2M_OSTM_TOP_VALUE;

		renesas_rza2m_ostm_write_8(dev, RZA2M_OSTMnCTL_OFFSET, RZA2M_OSTMnCTL_FREERUN_MODE);
	}
	data->period_counts = data->top_val;
	renesas_rza2m_ostm_write_32(dev, RZA2M_OSTMnCMP_OFFSET, data->period_counts);

	/* Start */
	renesas_rza2m_ostm_write_8(dev, RZA2M_OSTMnTS_OFFSET, 1);
}

static uint32_t ticks_sub(uint32_t val, uint32_t old)
{
	if (val >= old) {
		return (val - old);
	} else {
		return (val + (RZA2M_OSTM_TOP_VALUE - old + 1));
	}
}

static int counter_rza2m_ostm_get_value(const struct device *dev, uint32_t *ticks)
{
	uint32_t val;

	val = renesas_rza2m_ostm_read_32(dev, RZA2M_OSTMnCNT_OFFSET);
	*ticks = val;

	return 0;
}

static int renesas_rza2m_ostm_abs_alarm_set(const struct device *dev, uint32_t val,
					    bool irq_on_late)
{
	struct counter_rza2m_ostm_data *data = dev->data;
	uint32_t max_rel_val;
	uint32_t read_again;
	uint32_t diff;
	int err;

	/* Set new period */
	renesas_rza2m_ostm_period_set(dev, val);

	err = counter_rza2m_ostm_get_value(dev, &read_again);
	if (err != 0) {
		return -EIO;
	}

	max_rel_val = RZA2M_OSTM_TOP_VALUE - data->guard_period;
	diff = ticks_sub(val, read_again);
	if (diff > max_rel_val || diff == 0) {
		err = -ETIME;

		if (irq_on_late) {
			irq_enable(data->cycle_end_irq);
			arm_gic_irq_set_pending(data->cycle_end_irq);
		} else {
			data->alarm_cb = NULL;
		}
	} else {
		arm_gic_irq_clear_pending(data->cycle_end_irq);
		irq_enable(data->cycle_end_irq);
	}

	return err;
}

static int renesas_rza2m_ostm_rel_alarm_set(const struct device *dev, uint32_t val,
					    bool irq_on_late)
{
	struct counter_rza2m_ostm_data *data = dev->data;
	uint32_t max_rel_val;
	uint32_t read_again;
	uint32_t diff;
	uint32_t now;
	int err;

	err = counter_rza2m_ostm_get_value(dev, &now);
	if (err != 0) {
		return -EIO;
	}

	val = (now + val) & RZA2M_OSTM_TOP_VALUE;

	/* Set new period */
	renesas_rza2m_ostm_period_set(dev, val);

	err = counter_rza2m_ostm_get_value(dev, &read_again);
	if (err != 0) {
		return -EIO;
	}

	max_rel_val = irq_on_late ? RZA2M_OSTM_TOP_VALUE / 2U : RZA2M_OSTM_TOP_VALUE;
	diff = ticks_sub(val, read_again);
	if (diff > max_rel_val || diff == 0) {
		if (irq_on_late) {
			irq_enable(data->cycle_end_irq);
			arm_gic_irq_set_pending(data->cycle_end_irq);
		} else {
			data->alarm_cb = NULL;
		}
	} else {
		arm_gic_irq_clear_pending(data->cycle_end_irq);
		irq_enable(data->cycle_end_irq);
	}

	return 0;
}

static int renesas_rza2m_ostm_check_reset_if_late(const struct device *dev, uint32_t flags)
{
	struct counter_rza2m_ostm_data *data = dev->data;
	uint32_t curr_tick;
	bool reset;
	int err;

	reset = false;
	err = 0;

	if (flags & COUNTER_TOP_CFG_DONT_RESET) {
		/* Don't reset counter */
		err = counter_rza2m_ostm_get_value(dev, &curr_tick);
		if (err != 0) {
			return -EIO;
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
		/* Stop */
		renesas_rza2m_ostm_write_8(dev, RZA2M_OSTMnTT_OFFSET, 1);

		/* Start */
		renesas_rza2m_ostm_write_8(dev, RZA2M_OSTMnTS_OFFSET, 1);
	}

	return err;
}

static int counter_rza2m_ostm_init(const struct device *dev)
{
	struct counter_rza2m_ostm_data *data = dev->data;
	const struct counter_rza2m_ostm_config *cfg = dev->config;
	int err;

	if (!device_is_ready(cfg->clock_dev)) {
		return -ENODEV;
	}

	err = clock_control_on(cfg->clock_dev, (clock_control_subsys_t)cfg->clock_subsys);
	if (err < 0) {
		return err;
	}

	err = clock_control_get_rate(cfg->clock_dev, (clock_control_subsys_t)cfg->clock_subsys,
				     &data->clk_rate);
	if (err < 0) {
		return err;
	}

	DEVICE_MMIO_NAMED_MAP(dev, counter_ostm_mmio, K_MEM_CACHE_NONE);

	/* Stop timer */
	renesas_rza2m_ostm_write_8(dev, RZA2M_OSTMnTT_OFFSET, 1);

	/* Set period register */
	data->top_val = data->period_counts;
	renesas_rza2m_ostm_period_set(dev, data->period_counts);

	/* Set counter mode (GTMnMD) */
	renesas_rza2m_ostm_write_8(dev, RZA2M_OSTMnCTL_OFFSET, RZA2M_OSTMnCTL_FREERUN_MODE);

	return err;
}

static int counter_rza2m_ostm_start(const struct device *dev)
{
	struct counter_rza2m_ostm_data *data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);

	if (data->is_started) {
		k_spin_unlock(&data->lock, key);
		return -EALREADY;
	}

	if (data->is_periodic) {
		data->period_counts = data->top_val;
	}

	renesas_rza2m_ostm_switch_timer_mode(dev);

	arm_gic_irq_clear_pending(data->cycle_end_irq);
	data->is_started = true;
	if (data->top_cb) {
		irq_enable(data->cycle_end_irq);
	}

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int counter_rza2m_ostm_stop(const struct device *dev)
{
	struct counter_rza2m_ostm_data *data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);

	if (!data->is_started) {
		k_spin_unlock(&data->lock, key);
		return 0;
	}

	/* Stop timer */
	renesas_rza2m_ostm_write_8(dev, RZA2M_OSTMnTT_OFFSET, 1);

	/* Disable irq */
	irq_disable(data->cycle_end_irq);
	arm_gic_irq_clear_pending(data->cycle_end_irq);

	data->top_cb = NULL;
	data->alarm_cb = NULL;
	data->user_data = NULL;

	data->is_started = false;

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int counter_rza2m_ostm_set_alarm(const struct device *dev, uint8_t chan,
					const struct counter_alarm_cfg *alarm_cfg)
{
	struct counter_rza2m_ostm_data *data = dev->data;
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
		if (val > data->period_counts) {
			k_spin_unlock(&data->lock, key);
			return -EINVAL;
		}

		/* Restore free running mode */
		data->is_periodic = false;
		renesas_rza2m_ostm_switch_timer_mode(dev);
	}

	data->alarm_cb = alarm_cfg->callback;
	data->user_data = alarm_cfg->user_data;

	if (absolute) {
		irq_on_late = alarm_cfg->flags & COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE ? true : false;

		err = renesas_rza2m_ostm_abs_alarm_set(dev, val, irq_on_late);
		if (err) {
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
		irq_on_late = val < (RZA2M_OSTM_TOP_VALUE / 2U);

		err = renesas_rza2m_ostm_rel_alarm_set(dev, val, irq_on_late);
		if (err) {
			k_spin_unlock(&data->lock, key);
			return err;
		}
	}

	k_spin_unlock(&data->lock, key);

	return err;
}

static int counter_rza2m_ostm_cancel_alarm(const struct device *dev, uint8_t chan)
{
	struct counter_rza2m_ostm_data *data = dev->data;
	k_spinlock_key_t key;

	if (chan != 0) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	if (!data->is_started) {
		k_spin_unlock(&data->lock, key);
		return -EINVAL;
	}

	if (!data->alarm_cb) {
		k_spin_unlock(&data->lock, key);
		return 0;
	}

	irq_disable(data->cycle_end_irq);
	arm_gic_irq_clear_pending(data->cycle_end_irq);
	data->alarm_cb = NULL;
	data->user_data = NULL;

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int counter_rza2m_ostm_set_top_value(const struct device *dev,
					    const struct counter_top_cfg *top_cfg)
{
	struct counter_rza2m_ostm_data *data = dev->data;
	k_spinlock_key_t key;
	int err = 0;

	if (!top_cfg) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	/* -EBUSY if any alarm is active */
	if (data->alarm_cb) {
		return -EBUSY;
	}

	data->top_cb = top_cfg->callback;
	data->user_data = top_cfg->user_data;
	data->top_val = top_cfg->ticks;

	if (!data->is_periodic && top_cfg->ticks == RZA2M_OSTM_TOP_VALUE) {
		k_spin_unlock(&data->lock, key);
		return err;
	}

	if (top_cfg->ticks == RZA2M_OSTM_TOP_VALUE) {
		/* Restore free running mode */
		data->user_data = NULL;
		data->is_periodic = false;
		if (data->is_started) {
			renesas_rza2m_ostm_switch_timer_mode(dev);
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
		renesas_rza2m_ostm_switch_timer_mode(dev);

		if (data->top_cb) {
			irq_enable(data->cycle_end_irq);
		}

		k_spin_unlock(&data->lock, key);
		return err;
	}

	if (!data->top_cb) {
		/* New top cfg is without callback - stop IRQs */
		irq_disable(data->cycle_end_irq);
	}

	/* Timer already in interval mode - only change top value */
	renesas_rza2m_ostm_period_set(dev, data->top_val);

	/* Check if counter reset is required */
	err = renesas_rza2m_ostm_check_reset_if_late(dev, top_cfg->flags);

	k_spin_unlock(&data->lock, key);

	return err;
}

static uint32_t counter_rza2m_ostm_get_pending_int(const struct device *dev)
{
	struct counter_rza2m_ostm_data *data = dev->data;

	return arm_gic_irq_is_pending(data->cycle_end_irq);
}

static uint32_t counter_rza2m_ostm_get_top_value(const struct device *dev)
{
	struct counter_rza2m_ostm_data *data = dev->data;
	uint32_t top_val = RZA2M_OSTM_TOP_VALUE;

	if (data->is_periodic) {
		top_val = data->period_counts;
	}

	return top_val;
}
static uint32_t counter_rza2m_ostm_get_guard_period(const struct device *dev, uint32_t flags)
{
	struct counter_rza2m_ostm_data *data = dev->data;

	ARG_UNUSED(flags);

	return data->guard_period;
}

static int counter_rza2m_ostm_set_guard_period(const struct device *dev, uint32_t guard,
					       uint32_t flags)
{
	struct counter_rza2m_ostm_data *data = dev->data;

	ARG_UNUSED(flags);
	if (counter_rza2m_ostm_get_top_value(dev) < guard) {
		return -EINVAL;
	}

	data->guard_period = guard;

	return 0;
}

static uint32_t counter_rza2m_ostm_get_freq(const struct device *dev)
{
	struct counter_rza2m_ostm_data *data = dev->data;

	return data->clk_rate;
}

static DEVICE_API(counter, counter_rza2m_ostm_driver_api) = {
	.start = counter_rza2m_ostm_start,
	.stop = counter_rza2m_ostm_stop,
	.get_value = counter_rza2m_ostm_get_value,
	.set_alarm = counter_rza2m_ostm_set_alarm,
	.cancel_alarm = counter_rza2m_ostm_cancel_alarm,
	.set_top_value = counter_rza2m_ostm_set_top_value,
	.get_pending_int = counter_rza2m_ostm_get_pending_int,
	.get_top_value = counter_rza2m_ostm_get_top_value,
	.get_guard_period = counter_rza2m_ostm_get_guard_period,
	.set_guard_period = counter_rza2m_ostm_set_guard_period,
	.get_freq = counter_rza2m_ostm_get_freq,
};

void counter_rza2m_ostm_ovf_isr(const struct device *dev)
{
	struct counter_rza2m_ostm_data *data = dev->data;
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
		if (counter_rza2m_ostm_get_value(dev, &now) != 0) {
			return;
		}

		alarm_callback(dev, 0, now, usr_data);
	} else if (top_callback) {
		top_callback(dev, usr_data);
	} else {
		/* Do nothing */
	}
}

#define RZA2M_OSTM(idx) DT_INST_PARENT(idx)

#define RZA2M_OSTM_GET_IRQ_FLAGS(idx, irq_name) DT_IRQ_BY_NAME(RZA2M_OSTM(idx), irq_name, flags)

#define COUNTER_RZ_OSTM_INIT(inst)                                                                 \
	uint32_t counter_clock_subsys##inst = DT_CLOCKS_CELL(RZA2M_OSTM(inst), clk_id);            \
	static const struct counter_rza2m_ostm_config counter_rza2m_ostm_config_##inst = {         \
		DEVICE_MMIO_NAMED_ROM_INIT(counter_ostm_mmio, RZA2M_OSTM(inst)),                   \
		.config_info =                                                                     \
			{                                                                          \
				.max_top_value = RZA2M_OSTM_TOP_VALUE,                             \
				.flags = COUNTER_CONFIG_INFO_COUNT_UP,                             \
				.channels = 1,                                                     \
			},                                                                         \
		.clock_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR(RZA2M_OSTM(inst))),                      \
		.clock_subsys = (clock_control_subsys_t)(&counter_clock_subsys##inst),             \
	};                                                                                         \
	static struct counter_rza2m_ostm_data counter_rza2m_ostm_data_##inst = {                   \
		.channel = DT_PROP(RZA2M_OSTM(inst), channel),                                     \
		.period_counts = RZA2M_OSTM_TOP_VALUE,                                             \
		.cycle_end_irq =                                                                   \
			DT_IRQ_BY_NAME(RZA2M_OSTM(inst), overflow, irq) - GIC_SPI_INT_BASE,        \
	};                                                                                         \
	static int counter_rza2m_ostm_init_##inst(const struct device *dev)                        \
	{                                                                                          \
		IRQ_CONNECT(DT_IRQ_BY_NAME(RZA2M_OSTM(inst), overflow, irq) - GIC_SPI_INT_BASE,    \
			    DT_IRQ_BY_NAME(RZA2M_OSTM(inst), overflow, priority),                  \
			    counter_rza2m_ostm_ovf_isr, DEVICE_DT_INST_GET(inst),                  \
			    RZA2M_OSTM_GET_IRQ_FLAGS(inst, overflow));                             \
		return counter_rza2m_ostm_init(dev);                                               \
	}                                                                                          \
	DEVICE_DT_INST_DEFINE(inst, counter_rza2m_ostm_init_##inst, NULL,                          \
			      &counter_rza2m_ostm_data_##inst, &counter_rza2m_ostm_config_##inst,  \
			      PRE_KERNEL_1, CONFIG_COUNTER_INIT_PRIORITY,                          \
			      &counter_rza2m_ostm_driver_api);

DT_INST_FOREACH_STATUS_OKAY(COUNTER_RZ_OSTM_INIT)
