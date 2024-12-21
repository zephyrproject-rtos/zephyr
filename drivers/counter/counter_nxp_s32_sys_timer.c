/*
 * Copyright 2022-2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_s32_sys_timer

#include <zephyr/kernel.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/clock_control.h>
#if defined(CONFIG_GIC)
#include <zephyr/drivers/interrupt_controller/gic.h>
#endif /* CONFIG_GIC */
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>

LOG_MODULE_REGISTER(nxp_s32_sys_timer, CONFIG_COUNTER_LOG_LEVEL);

/* System Timer Module (STM) register definitions */
/* Control */
#define STM_CR           0x0
#define STM_CR_TEN_MASK  BIT(0)
#define STM_CR_TEN(v)    FIELD_PREP(STM_CR_TEN_MASK, (v))
#define STM_CR_FRZ_MASK  BIT(1)
#define STM_CR_FRZ(v)    FIELD_PREP(STM_CR_FRZ_MASK, (v))
#define STM_CR_CPS_MASK  GENMASK(15, 8)
#define STM_CR_CPS(v)    FIELD_PREP(STM_CR_CPS_MASK, (v))
/* Count */
#define STM_CNT          0x4
#define STM_CNT_CNT_MASK GENMASK(31, 0)
#define STM_CNT_CNT(v)   FIELD_PREP(STM_CNT_CNT_MASK, (v))
/* Channel Control */
#define STM_CCR(n)       (0x10 + 0x10 * (n))
#define STM_CCR_CEN_MASK BIT(0)
#define STM_CCR_CEN(v)   FIELD_PREP(STM_CCR_CEN_MASK, (v))
/* Channel Interrupt */
#define STM_CIR(n)       (0x14 + 0x10 * (n))
#define STM_CIR_CIF_MASK BIT(0)
#define STM_CIR_CIF(v)   FIELD_PREP(STM_CIR_CIF_MASK, (v))
/* Channel Compare */
#define STM_CMP(n)       (0x18 + 0x10 * (n))
#define STM_CMP_CMP_MASK GENMASK(31, 0)
#define STM_CMP_CMP(v)   FIELD_PREP(STM_CMP_CMP_MASK, (v))

/* Handy accessors */
#define REG_READ(r)      sys_read32(config->base + (r))
#define REG_WRITE(r, v)  sys_write32((v), config->base + (r))

#define SYS_TIMER_MAX_VALUE     0xFFFFFFFFU
#define SYS_TIMER_NUM_CHANNELS  4

struct nxp_s32_sys_timer_chan_data {
	counter_alarm_callback_t callback;
	void *user_data;
};

struct nxp_s32_sys_timer_data {
	struct nxp_s32_sys_timer_chan_data ch_data[SYS_TIMER_NUM_CHANNELS];
	uint32_t guard_period;
	atomic_t irq_pending;
};

struct nxp_s32_sys_timer_config {
	struct counter_config_info info;
	mem_addr_t base;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	uint8_t prescaler;
	bool freeze;
	unsigned int irqn;
};

static ALWAYS_INLINE void irq_set_pending(unsigned int irq)
{
#if defined(CONFIG_GIC)
	arm_gic_irq_set_pending(irq);
#else
	NVIC_SetPendingIRQ(irq);
#endif /* CONFIG_GIC */
}

static uint32_t ticks_add(uint32_t val1, uint32_t val2, uint32_t top)
{
	uint32_t to_top;

	if (likely(IS_BIT_MASK(top))) {
		return (val1 + val2) & top;
	}

	/* top is not 2^n-1 */
	to_top = top - val1;

	return (val2 <= to_top) ? val1 + val2 : val2 - to_top;
}

static uint32_t ticks_sub(uint32_t val, uint32_t old, uint32_t top)
{
	if (likely(IS_BIT_MASK(top))) {
		return (val - old) & top;
	}

	/* top is not 2^n-1 */
	return (val >= old) ? (val - old) : val + top + 1 - old;
}

static ALWAYS_INLINE void stm_disable_channel(const struct nxp_s32_sys_timer_config *config,
					      uint8_t channel)
{
	REG_WRITE(STM_CCR(channel), STM_CCR_CEN(0U));
	REG_WRITE(STM_CIR(channel), STM_CIR_CIF(1U));
}

static int stm_set_alarm(const struct device *dev, uint8_t channel, uint32_t ticks, uint32_t flags)
{
	const struct nxp_s32_sys_timer_config *config = dev->config;
	struct nxp_s32_sys_timer_data *data = dev->data;
	struct nxp_s32_sys_timer_chan_data *ch_data = &data->ch_data[channel];
	const uint32_t now = REG_READ(STM_CNT);
	const uint32_t top_val = config->info.max_top_value;
	int err = 0;
	uint32_t diff;
	uint32_t max_rel_val;
	bool irq_on_late;

	if (flags & COUNTER_ALARM_CFG_ABSOLUTE) {
		__ASSERT_NO_MSG(data->guard_period < top_val);
		max_rel_val = top_val - data->guard_period;
		irq_on_late = !!(flags & COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE);
	} else {
		/*
		 * If relative value is smaller than half of the counter range it is assumed
		 * that there is a risk of setting value too late and late detection algorithm
		 * must be applied. When late setting is detected, interrupt shall be
		 * triggered for immediate expiration of the timer. Detection is performed
		 * by limiting relative distance between CMP and CNT.
		 *
		 * Note that half of counter range is an arbitrary value.
		 */
		irq_on_late = ticks < (top_val / 2);
		/* Limit max to detect short relative being set too late */
		max_rel_val = irq_on_late ? top_val / 2 : top_val;
		ticks = ticks_add(now, ticks, top_val);
	}

	/* Disable the channel before loading the new value so that it takes effect immediately */
	stm_disable_channel(config, channel);
	REG_WRITE(STM_CMP(channel), ticks);
	REG_WRITE(STM_CCR(channel), STM_CCR_CEN(1U));

	/*
	 * Decrement value to detect also case when ticks == CNT. Otherwise, condition would need
	 * to include comparing diff against 0.
	 */
	diff = ticks_sub(ticks - 1, REG_READ(STM_CNT), top_val);
	if (diff > max_rel_val) {
		if (flags & COUNTER_ALARM_CFG_ABSOLUTE) {
			err = -ETIME;
		}

		/*
		 * Interrupt is triggered always for relative alarm and for absolute depending
		 * on the flag
		 */
		if (irq_on_late) {
			atomic_or(&data->irq_pending, BIT(channel));
			irq_set_pending(config->irqn);
		} else {
			ch_data->callback = NULL;
		}
	}

	return err;
}

static void stm_isr(const struct device *dev)
{
	const struct nxp_s32_sys_timer_config *config = dev->config;
	struct nxp_s32_sys_timer_data *data = dev->data;
	struct nxp_s32_sys_timer_chan_data *ch_data = NULL;
	counter_alarm_callback_t cb = NULL;
	void *cb_args = NULL;
	uint8_t channel;
	bool pending;

	for (channel = 0; channel < SYS_TIMER_NUM_CHANNELS; ++channel) {
		pending = FIELD_GET(STM_CCR_CEN_MASK, REG_READ(STM_CCR(channel))) &&
			  FIELD_GET(STM_CIR_CIF_MASK, REG_READ(STM_CIR(channel)));

		if (pending || atomic_test_bit(&data->irq_pending, channel)) {
			stm_disable_channel(config, channel);
			atomic_and(&data->irq_pending, ~BIT(channel));

			ch_data = &data->ch_data[channel];
			if (ch_data->callback) {
				cb = ch_data->callback;
				cb_args = ch_data->user_data;
				ch_data->callback = NULL;
				ch_data->user_data = NULL;
				cb(dev, channel, REG_READ(STM_CNT), cb_args);
			}
		}
	}
}

static int nxp_s32_sys_timer_start(const struct device *dev)
{
	const struct nxp_s32_sys_timer_config *config = dev->config;

	REG_WRITE(STM_CNT, 0U);
	REG_WRITE(STM_CR, REG_READ(STM_CR) | STM_CR_TEN(1U));

	return 0;
}

static int nxp_s32_sys_timer_stop(const struct device *dev)
{
	const struct nxp_s32_sys_timer_config *config = dev->config;

	REG_WRITE(STM_CR, REG_READ(STM_CR) & ~STM_CR_TEN_MASK);

	return 0;
}

static int nxp_s32_sys_timer_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct nxp_s32_sys_timer_config *config = dev->config;

	*ticks = REG_READ(STM_CNT);

	return 0;
}

static int nxp_s32_sys_timer_set_alarm(const struct device *dev, uint8_t channel,
				       const struct counter_alarm_cfg *alarm_cfg)
{
	const struct nxp_s32_sys_timer_config *config = dev->config;
	struct nxp_s32_sys_timer_data *data = dev->data;
	struct nxp_s32_sys_timer_chan_data *ch_data = &data->ch_data[channel];

	if (ch_data->callback) {
		return -EBUSY;
	}

	if (alarm_cfg->ticks > config->info.max_top_value) {
		LOG_ERR("Invalid ticks value %d", alarm_cfg->ticks);
		return -EINVAL;
	}

	ch_data->callback = alarm_cfg->callback;
	ch_data->user_data = alarm_cfg->user_data;

	return stm_set_alarm(dev, channel, alarm_cfg->ticks, alarm_cfg->flags);
}

static int nxp_s32_sys_timer_cancel_alarm(const struct device *dev, uint8_t channel)
{
	const struct nxp_s32_sys_timer_config *config = dev->config;
	struct nxp_s32_sys_timer_data *data = dev->data;
	struct nxp_s32_sys_timer_chan_data *ch_data = &data->ch_data[channel];

	stm_disable_channel(config, channel);
	ch_data->callback = NULL;
	ch_data->user_data = NULL;

	return 0;
}

static uint32_t nxp_s32_sys_timer_get_pending_int(const struct device *dev)
{
	const struct nxp_s32_sys_timer_config *config = dev->config;
	uint8_t i;

	for (i = 0; i < counter_get_num_of_channels(dev); i++) {
		if (REG_READ(STM_CIR(i)) & STM_CIR_CIF_MASK) {
			return 1;
		}
	}

	return 0;
}

static int nxp_s32_sys_timer_set_top_value(const struct device *dev,
					   const struct counter_top_cfg *cfg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cfg);

	/* Overflow is fixed and cannot be changed */
	return -ENOTSUP;
}

static uint32_t nxp_s32_sys_timer_get_top_value(const struct device *dev)
{
	const struct nxp_s32_sys_timer_config *config = dev->config;

	return config->info.max_top_value;
}

static int nxp_s32_sys_timer_set_guard_period(const struct device *dev, uint32_t guard,
					      uint32_t flags)
{
	struct nxp_s32_sys_timer_data *data = dev->data;

	ARG_UNUSED(flags);

	__ASSERT_NO_MSG(guard < nxp_s32_sys_timer_get_top_value(dev));
	data->guard_period = guard;

	return 0;
}

static uint32_t nxp_s32_sys_timer_get_guard_period(const struct device *dev, uint32_t flags)
{
	struct nxp_s32_sys_timer_data *data = dev->data;

	ARG_UNUSED(flags);

	return data->guard_period;
}

static uint32_t nxp_s32_sys_timer_get_frequency(const struct device *dev)
{
	const struct nxp_s32_sys_timer_config *config = dev->config;
	uint32_t clock_rate;

	if (clock_control_get_rate(config->clock_dev, config->clock_subsys, &clock_rate)) {
		LOG_ERR("Failed to get clock frequency");
		return 0;
	}

	return clock_rate / (config->prescaler + 1U);
}

static int nxp_s32_sys_timer_init(const struct device *dev)
{
	const struct nxp_s32_sys_timer_config *config = dev->config;
	struct nxp_s32_sys_timer_data *data = dev->data;
	struct nxp_s32_sys_timer_chan_data *ch_data;
	int i;
	int err;

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("Clock control device not ready");
		return -ENODEV;
	}

	err = clock_control_on(config->clock_dev, config->clock_subsys);
	if (err) {
		LOG_ERR("Failed to enable clock");
		return err;
	}

	REG_WRITE(STM_CNT, 0U);
	REG_WRITE(STM_CR,
		  STM_CR_FRZ(config->freeze) |
		  STM_CR_CPS(config->prescaler) |
		  STM_CR_TEN(1U));

	for (i = 0; i < counter_get_num_of_channels(dev); i++) {
		ch_data = &data->ch_data[i];
		ch_data->callback = NULL;
		ch_data->user_data = NULL;

		REG_WRITE(STM_CCR(i), STM_CCR_CEN(0U));
		REG_WRITE(STM_CIR(i), STM_CIR_CIF(1U));
		REG_WRITE(STM_CMP(i), 0U);
	}

	return 0;
}

static const struct counter_driver_api nxp_s32_sys_timer_driver_api = {
	.start = nxp_s32_sys_timer_start,
	.stop = nxp_s32_sys_timer_stop,
	.get_value = nxp_s32_sys_timer_get_value,
	.set_alarm = nxp_s32_sys_timer_set_alarm,
	.cancel_alarm = nxp_s32_sys_timer_cancel_alarm,
	.set_top_value = nxp_s32_sys_timer_set_top_value,
	.get_top_value = nxp_s32_sys_timer_get_top_value,
	.set_guard_period = nxp_s32_sys_timer_set_guard_period,
	.get_guard_period = nxp_s32_sys_timer_get_guard_period,
	.get_pending_int = nxp_s32_sys_timer_get_pending_int,
	.get_freq = nxp_s32_sys_timer_get_frequency
};

#define SYS_TIMER_INIT_DEVICE(n)							\
	static int nxp_s32_sys_timer_##n##_init(const struct device *dev)		\
	{										\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),			\
			    stm_isr, DEVICE_DT_INST_GET(n),				\
			    COND_CODE_1(DT_INST_IRQ_HAS_CELL(n, flags),			\
					(DT_INST_IRQ(n, flags)), (0)));			\
		irq_enable(DT_INST_IRQN(n));						\
											\
		return nxp_s32_sys_timer_init(dev);					\
	}										\
											\
	static struct nxp_s32_sys_timer_data nxp_s32_sys_timer_data_##n;		\
											\
	static const struct nxp_s32_sys_timer_config nxp_s32_sys_timer_config_##n = {	\
		.info = {								\
			.max_top_value = SYS_TIMER_MAX_VALUE,				\
			.channels = SYS_TIMER_NUM_CHANNELS,				\
			.flags = COUNTER_CONFIG_INFO_COUNT_UP,				\
		},									\
		.base = DT_INST_REG_ADDR(n),						\
		.freeze = DT_INST_PROP(n, freeze),					\
		.prescaler = DT_INST_PROP(n, prescaler) - 1,				\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),			\
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),	\
		.irqn = DT_INST_IRQN(n),						\
	};										\
											\
	DEVICE_DT_INST_DEFINE(n,							\
			 nxp_s32_sys_timer_##n##_init,					\
			 NULL,								\
			 &nxp_s32_sys_timer_data_##n,					\
			 &nxp_s32_sys_timer_config_##n,					\
			 POST_KERNEL,							\
			 CONFIG_COUNTER_INIT_PRIORITY,					\
			 &nxp_s32_sys_timer_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SYS_TIMER_INIT_DEVICE)
