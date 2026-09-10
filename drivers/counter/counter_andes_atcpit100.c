/*
 * Copyright (c) 2022 Andes Technology
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/counter.h>
#include <zephyr/spinlock.h>
#include <zephyr/irq.h>
#include <zephyr/arch/cpu.h>
#include <string.h>

#define DT_DRV_COMPAT andestech_atcpit100

/* register definitions */
#define REG_IDR   0x00 /* ID and Revision Reg.   */
#define REG_CFG   0x10 /* Configuration Reg.     */
#define REG_INTE  0x14 /* Interrupt Enable Reg.  */
#define REG_ISTA  0x18 /* Interrupt Status Reg.  */
#define REG_CHEN  0x1C /* Channel Enable Reg.    */
#define REG_CTRL0 0x20 /* Channel 0 Control Reg. */
#define REG_RELD0 0x24 /* Channel 0 Reload Reg.  */
#define REG_CNTR0 0x28 /* Channel 0 Counter Reg. */
#define REG_CTRL1 0x30 /* Channel 1 Control Reg. */
#define REG_RELD1 0x34 /* Channel 1 Reload Reg.  */
#define REG_CNTR1 0x38 /* Channel 1 Counter Reg. */
#define REG_CTRL2 0x40 /* Channel 2 Control Reg. */
#define REG_RELD2 0x44 /* Channel 2 Reload Reg.  */
#define REG_CNTR2 0x48 /* Channel 2 Counter Reg. */
#define REG_CTRL3 0x50 /* Channel 3 Control Reg. */
#define REG_RELD3 0x54 /* Channel 3 Reload Reg.  */
#define REG_CNTR3 0x58 /* Channel 3 Counter Reg. */

#define PIT_BASE (((const struct atcpit100_config *)(dev)->config)->base)
#define PIT_INTE(dev) (PIT_BASE + REG_INTE)
#define PIT_ISTA(dev) (PIT_BASE + REG_ISTA)
#define PIT_CHEN(dev) (PIT_BASE + REG_CHEN)
#define PIT_CH_CTRL(dev, ch) (PIT_BASE + REG_CTRL0 + (ch << 4))
#define PIT_CH_RELD(dev, ch) (PIT_BASE + REG_RELD0 + (ch << 4))
#define PIT_CH_CNTR(dev, ch) (PIT_BASE + REG_CNTR0 + (ch << 4))

#define CTRL_CH_SRC_PCLK BIT(3)
#define CTRL_CH_MODE_32BIT BIT(0)

#define CHANNEL_NUM (4)
#define CH_NUM_PER_COUNTER (CHANNEL_NUM - 1)
#define TIMER0_CHANNEL(ch) BIT(((ch) * CHANNEL_NUM))

typedef void (*atcpit100_cfg_func_t)(void);

struct atcpit100_config {
	struct counter_config_info info;
	uint32_t base;
	uint32_t divider;
	uint32_t irq_num;
	atcpit100_cfg_func_t cfg_func;
};

struct counter_atcpit100_ch_data {
	counter_alarm_callback_t alarm_callback;
	void *alarm_user_data;
};

struct atcpit100_data {
	counter_top_callback_t top_callback;
	void *top_user_data;
	uint32_t guard_period;
	struct k_spinlock lock;
	struct counter_atcpit100_ch_data ch_data[CH_NUM_PER_COUNTER];
};

static inline uint32_t get_current_tick(const struct device *dev, uint32_t ch)
{
	const struct atcpit100_config *config = dev->config;
	uint32_t top, now_cnt;

	/* Preload cycles is reload register + 1 */
	top = sys_read32(PIT_CH_RELD(dev, ch)) + 1;
	now_cnt = top - sys_read32(PIT_CH_CNTR(dev, ch));

	return (now_cnt / config->divider);
}

static void atcpit100_irq_handler(void *arg)
{
	struct device *dev = (struct device *)arg;
	struct atcpit100_data *data = dev->data;
	counter_alarm_callback_t cb;
	uint32_t int_status, int_enable, ch_enable, cur_ticks;
	uint8_t i;

	ch_enable = sys_read32(PIT_CHEN(dev));
	int_enable = sys_read32(PIT_INTE(dev));
	int_status = sys_read32(PIT_ISTA(dev));

	if (int_status & TIMER0_CHANNEL(3)) {
		if (data->top_callback) {
			data->top_callback(dev, data->top_user_data);
		}
	}

	for (i = 0; i < CH_NUM_PER_COUNTER; i++) {
		if (int_status & TIMER0_CHANNEL(i)) {
			int_enable &= ~TIMER0_CHANNEL(i);
			ch_enable &= ~TIMER0_CHANNEL(i);
		}
	}

	/* Disable channel and interrupt */
	sys_write32(int_enable, PIT_INTE(dev));
	sys_write32(ch_enable, PIT_CHEN(dev));

	/* Clear interrupt status */
	sys_write32(int_status, PIT_ISTA(dev));

	for (i = 0; i < CH_NUM_PER_COUNTER; i++) {
		if (int_status & TIMER0_CHANNEL(i)) {
			cur_ticks = get_current_tick(dev, 3);
			cb = data->ch_data[i].alarm_callback;
			data->ch_data[i].alarm_callback = NULL;
			if (cb != NULL) {
				cb(dev, i, cur_ticks, data->ch_data[i].alarm_user_data);
			}
		}
	}
}

static int counter_atcpit100_init(const struct device *dev)
{
	const struct atcpit100_config *config = dev->config;
	uint32_t reg;

	/* Disable all channels */
	sys_write32(0, PIT_CHEN(dev));

	/* Channel 0 ~ 3, 32 bits timer, PCLK source */
	reg = CTRL_CH_MODE_32BIT | CTRL_CH_SRC_PCLK;
	sys_write32(reg, PIT_CH_CTRL(dev, 0));
	sys_write32(reg, PIT_CH_CTRL(dev, 1));
	sys_write32(reg, PIT_CH_CTRL(dev, 2));
	sys_write32(reg, PIT_CH_CTRL(dev, 3));

	/* Disable all interrupt and clear all pending interrupt */
	sys_write32(0, PIT_INTE(dev));
	sys_write32(UINT32_MAX, PIT_ISTA(dev));

	/* Select channel 3 as default counter and set max top value */
	reg = config->info.max_top_value * config->divider;

	/* Set cycle - 1 to reload register */
	sys_write32((reg - 1), PIT_CH_RELD(dev, 3));

	config->cfg_func();

	irq_enable(config->irq_num);

	return 0;
}

static int atcpit100_start(const struct device *dev)
{
	struct atcpit100_data *data = dev->data;
	k_spinlock_key_t key;
	uint32_t reg;

	key = k_spin_lock(&data->lock);

	/* Enable channel */
	reg = sys_read32(PIT_CHEN(dev));
	reg |= TIMER0_CHANNEL(3);
	sys_write32(reg, PIT_CHEN(dev));

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int atcpit100_stop(const struct device *dev)
{
	struct atcpit100_data *data = dev->data;
	k_spinlock_key_t key;
	uint32_t reg;

	key = k_spin_lock(&data->lock);

	/* Disable channel interrupt */
	reg = sys_read32(PIT_INTE(dev));
	reg &= ~TIMER0_CHANNEL(3);
	sys_write32(reg, PIT_INTE(dev));

	/* Disable channel */
	reg = sys_read32(PIT_CHEN(dev));
	reg &= ~TIMER0_CHANNEL(3);
	sys_write32(reg, PIT_CHEN(dev));

	/* Clear interrupt status */
	sys_write32(TIMER0_CHANNEL(3), PIT_ISTA(dev));

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int atcpit100_get_value(const struct device *dev, uint32_t *ticks)
{
	struct atcpit100_data *data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);

	*ticks = get_current_tick(dev, 3);

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int atcpit100_set_alarm(const struct device *dev, uint8_t chan_id,
			       const struct counter_alarm_cfg *alarm_cfg)
{
	const struct atcpit100_config *config = dev->config;
	struct atcpit100_data *data = dev->data;
	uint32_t top, now_cnt, remain_cnt, alarm_cnt, flags, reg;
	k_spinlock_key_t key;
	int err = 0;

	if (chan_id >= CH_NUM_PER_COUNTER) {
		return -ENOTSUP;
	}

	if (!alarm_cfg->callback) {
		return -EINVAL;
	}

	if (data->ch_data[chan_id].alarm_callback) {
		return -EBUSY;
	}

	key = k_spin_lock(&data->lock);

	/* Preload cycles is reload register + 1 */
	top = sys_read32(PIT_CH_RELD(dev, 3)) + 1;
	remain_cnt = sys_read32(PIT_CH_CNTR(dev, 3));
	alarm_cnt = alarm_cfg->ticks * config->divider;

	if (alarm_cnt > top) {
		err = -EINVAL;
		goto out;
	}

	flags = alarm_cfg->flags;
	data->ch_data[chan_id].alarm_callback = alarm_cfg->callback;
	data->ch_data[chan_id].alarm_user_data = alarm_cfg->user_data;

	if (flags & COUNTER_ALARM_CFG_ABSOLUTE) {
		uint32_t irq_on_late, max_rel_val;

		now_cnt = top - remain_cnt;
		max_rel_val = top - (data->guard_period * config->divider);
		irq_on_late = flags & COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE;

		if (now_cnt < alarm_cnt) {
			/* Absolute alarm is in this round counting */
			reg = alarm_cnt - now_cnt;
			irq_on_late = 0;
		} else {
			/* Absolute alarm is in the next round counting */
			reg = alarm_cnt + remain_cnt;
		}

		if (reg > max_rel_val) {
			/* Absolute alarm is in the guard period */
			err = -ETIME;
			if (!irq_on_late) {
				data->ch_data[chan_id].alarm_callback = NULL;
				goto out;
			}
		}

		if (irq_on_late) {
			/* Trigger interrupt immediately */
			reg = 1;
		}
	} else {
		/* Round up decreasing counter to tick boundary */
		now_cnt = remain_cnt + config->divider - 1;
		now_cnt = (now_cnt / config->divider) * config->divider;

		/* Adjusting relative alarm counter to tick boundary */
		reg = alarm_cnt - (now_cnt - remain_cnt);
	}

	/* Set cycle - 1 to reload register */
	sys_write32((reg - 1), PIT_CH_RELD(dev, chan_id));

	/* Enable channel interrupt */
	reg = sys_read32(PIT_INTE(dev));
	reg |= TIMER0_CHANNEL(chan_id);
	sys_write32(reg, PIT_INTE(dev));

	/* Enable channel */
	reg = sys_read32(PIT_CHEN(dev));
	reg |= TIMER0_CHANNEL(chan_id);
	sys_write32(reg, PIT_CHEN(dev));

out:
	k_spin_unlock(&data->lock, key);

	return err;
}

static int atcpit100_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	struct atcpit100_data *data = dev->data;
	k_spinlock_key_t key;
	uint32_t reg;

	if (chan_id >= CH_NUM_PER_COUNTER) {
		return -ENOTSUP;
	}

	key = k_spin_lock(&data->lock);

	/* Disable channel interrupt */
	reg = sys_read32(PIT_INTE(dev));
	reg &= ~TIMER0_CHANNEL(chan_id);
	sys_write32(reg, PIT_INTE(dev));

	/* Disable channel */
	reg = sys_read32(PIT_CHEN(dev));
	reg &= ~TIMER0_CHANNEL(chan_id);
	sys_write32(reg, PIT_CHEN(dev));

	/* Clear interrupt status */
	sys_write32(TIMER0_CHANNEL(chan_id), PIT_ISTA(dev));

	data->ch_data[chan_id].alarm_callback = NULL;

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int atcpit100_set_top_value(const struct device *dev,
				   const struct counter_top_cfg *cfg)
{
	const struct atcpit100_config *config = dev->config;
	struct atcpit100_data *data = dev->data;
	uint32_t ticks, reg, reset_counter = 1;
	k_spinlock_key_t key;
	int err = 0;
	uint8_t i;

	for (i = 0; i < counter_get_num_of_channels(dev); i++) {
		if (data->ch_data[i].alarm_callback) {
			return -EBUSY;
		}
	}

	if (cfg->ticks > config->info.max_top_value) {
		return -ENOTSUP;
	}

	key = k_spin_lock(&data->lock);

	if (cfg->callback) {
		/* Disable channel interrupt */
		reg = sys_read32(PIT_INTE(dev));
		reg &= ~TIMER0_CHANNEL(3);
		sys_write32(reg, PIT_INTE(dev));

		data->top_callback = cfg->callback;
		data->top_user_data = cfg->user_data;

		/* Enable channel interrupt */
		reg = sys_read32(PIT_INTE(dev));
		reg |= TIMER0_CHANNEL(3);
		sys_write32(reg, PIT_INTE(dev));
	}

	if (cfg->flags & COUNTER_TOP_CFG_DONT_RESET) {
		/* Don't reset counter */
		reset_counter = 0;
		ticks = get_current_tick(dev, 3);
		if (ticks >= cfg->ticks) {
			err = -ETIME;
			if (cfg->flags & COUNTER_TOP_CFG_RESET_WHEN_LATE) {
				/* Reset counter if current is late */
				reset_counter = 1;
			}
		}
	}

	/* Set cycle - 1 to reload register */
	reg = cfg->ticks * config->divider;
	sys_write32((reg - 1), PIT_CH_RELD(dev, 3));

	if (reset_counter) {
		/* Disable channel */
		reg = sys_read32(PIT_CHEN(dev));
		reg &= ~TIMER0_CHANNEL(3);
		sys_write32(reg, PIT_CHEN(dev));

		/* Clear interrupt status */
		sys_write32(TIMER0_CHANNEL(3), PIT_ISTA(dev));

		/* Enable channel interrupt */
		reg = sys_read32(PIT_INTE(dev));
		reg |= TIMER0_CHANNEL(3);
		sys_write32(reg, PIT_INTE(dev));

		/* Enable channel */
		reg = sys_read32(PIT_CHEN(dev));
		reg |= TIMER0_CHANNEL(3);
		sys_write32(reg, PIT_CHEN(dev));
	}

	k_spin_unlock(&data->lock, key);

	return err;
}

static uint32_t atcpit100_get_pending_int(const struct device *dev)
{
	uint32_t reg = sys_read32(PIT_ISTA(dev));

	reg &= (TIMER0_CHANNEL(0) | TIMER0_CHANNEL(1) |
		TIMER0_CHANNEL(2) | TIMER0_CHANNEL(3));

	return !(!reg);
}

static uint32_t atcpit100_get_top_value(const struct device *dev)
{
	const struct atcpit100_config *config = dev->config;
	uint32_t top = sys_read32(PIT_CH_RELD(dev, 3)) + 1;

	return (top / config->divider);
}

static uint32_t atcpit100_get_guard_period(const struct device *dev,
					   uint32_t flags)
{
	struct atcpit100_data *data = dev->data;

	return data->guard_period;
}

static int atcpit100_set_guard_period(const struct device *dev,
				      uint32_t ticks, uint32_t flags)
{
	const struct atcpit100_config *config = dev->config;
	struct atcpit100_data *data = dev->data;
	uint32_t top = sys_read32(PIT_CH_RELD(dev, 3)) + 1;

	if ((ticks * config->divider) > top) {
		return -EINVAL;
	}

	data->guard_period = ticks;

	return 0;
}

static DEVICE_API(counter, atcpit100_driver_api) = {
	.start = atcpit100_start,
	.stop = atcpit100_stop,
	.get_value = atcpit100_get_value,
	.set_alarm = atcpit100_set_alarm,
	.cancel_alarm = atcpit100_cancel_alarm,
	.set_top_value = atcpit100_set_top_value,
	.get_pending_int = atcpit100_get_pending_int,
	.get_top_value = atcpit100_get_top_value,
	.get_guard_period = atcpit100_get_guard_period,
	.set_guard_period = atcpit100_set_guard_period,
};

#define COUNTER_ATCPIT100_INIT(n)					\
	static void counter_atcpit100_cfg_##n(void);			\
	static struct atcpit100_data atcpit100_data_##n;		\
									\
	static const struct atcpit100_config atcpit100_config_##n = {	\
		.info = {						\
			.max_top_value =				\
				(UINT32_MAX/DT_INST_PROP(n, prescaler)),\
			.freq = (DT_INST_PROP(n, clock_frequency) /	\
				DT_INST_PROP(n, prescaler)),		\
			.flags = COUNTER_CONFIG_INFO_COUNT_UP,		\
			.channels = CH_NUM_PER_COUNTER,			\
		},							\
		.base = DT_INST_REG_ADDR(n),				\
		.divider = DT_INST_PROP(n, prescaler),			\
		.irq_num = DT_INST_IRQN(n),				\
		.cfg_func = counter_atcpit100_cfg_##n,			\
	};								\
									\
	DEVICE_DT_INST_DEFINE(n,					\
		counter_atcpit100_init,					\
		NULL,							\
		&atcpit100_data_##n,					\
		&atcpit100_config_##n,					\
		PRE_KERNEL_1,						\
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,			\
		&atcpit100_driver_api);					\
									\
	static void counter_atcpit100_cfg_##n(void)			\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n),				\
			    DT_INST_IRQ(n, priority),			\
			    atcpit100_irq_handler,			\
			    DEVICE_DT_INST_GET(n),			\
			    0);						\
	}

DT_INST_FOREACH_STATUS_OKAY(COUNTER_ATCPIT100_INIT)
