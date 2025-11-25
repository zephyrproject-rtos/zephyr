/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rcar_cmt_counter

#include <zephyr/arch/cpu.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/renesas_cpg_mssr.h>

/* Offset of register of the channel */
#define CHANNEL_OFFSET(n) (0x100u * n)

/* Compare match timer start register channel n */
#define CMSTR_OFFSET(n) (CHANNEL_OFFSET(n) + 0x0000u)

/* Compare match timer control/status register channel n */
#define CMCSR_OFFSET(n) (CHANNEL_OFFSET(n) + 0x0010u)
#define BIT_CMF(n)      ((n & 0x01u) << 15u) /* Compare match flag */
#define BIT_OVF(n)      ((n & 0x01u) << 14u) /* Overflow flag */
#define BIT_WRFLG(n)    ((n & 0x01u) << 13u) /* Write state flag */
#define BIT_CMS(n)      ((n & 0x01u) << 9u)  /* Compare match counter size */
#define BIT_CMM(n)      ((n & 0x01u) << 8u)  /* Compare match mode */
#define BIT_CMR(n)      ((n & 0x03u) << 4u)  /* Compare match request interrupt */
#define BIT_DBGIVD(n)   ((n & 0x01u) << 3u)  /* Debug mode operation */
#define BIT_CKS(n)      ((n & 0x07u) << 0u)  /* Clock Select */

/* Compare match timer counter channel n */
#define CMCNT_OFFSET(n) (CHANNEL_OFFSET(n) + 0x0014u)

/* Compare match timer constant register channel n */
#define CMCOR_OFFSET(n) (CHANNEL_OFFSET(n) + 0x0018u)

/* Compare match timer H control/status register channel n */
#define CMCSRH_OFFSET(n) (CHANNEL_OFFSET(n) + 0x0020u)
#define BIT_CMSH(n)      ((n & 0x01u) << 9u) /* Compare match size (high) */
#define BIT_CKSH(n)      ((n & 0x01u) << 0u) /* Clock select (high) */

/* Compare match timer counter H0 channel n */
#define CMCNTH_OFFSET(n) (CHANNEL_OFFSET(n) + 0x0024u)

/* Compare match timer constant H0 register channel n */
#define CMCORH_OFFSET(n) (CHANNEL_OFFSET(n) + 0x0028u)

/* CLK Enable register, base address and offset get from devicetree */
#define BIT_CLKE(n) ((n & 0xFFu) << 0u) /* Channel clock mask */

/* Maximum channels the counter have */
#define CHANNELS_MAX 2u

struct rcar_cmt_config {
	struct counter_config_info counter_info;
	uint32_t clk_en_base;
	uint32_t cnt_base;
	void (*cfg_func)(void);
};

struct channel_callback {
	void (*alarm_callback_func)(void *arg);
	void *alarm_callback_arg;
};

struct rcar_cmt_data {
	struct channel_callback cbs[CHANNELS_MAX];
	uint32_t guard_value;
	uint32_t guard_flags;
};

struct rcar_cmt_irq_param {
	const struct device *dev;
	const uint8_t channel;
	const uint32_t irq_id;
	const uint32_t irq_flags;
};

/* Wait for a number of CMT input clock cycles */
static void wait_for_clk_cycle(const struct device *dev, uint32_t cycles)
{
	const struct rcar_cmt_config *config = (const struct rcar_cmt_config *)dev->config;
	const uint32_t clk_freq = config->counter_info.freq;
	uint32_t usec_to_wait = (cycles * 1000000 / clk_freq) + 1; /* Round up */

	k_busy_wait(usec_to_wait);
}

/* Set counter register value for the specified channel */
static void set_cmcnt(const struct device *dev, uint8_t channel, uint32_t value)
{
	const struct rcar_cmt_config *config = (const struct rcar_cmt_config *)dev->config;
	uint32_t cmcsr = config->cnt_base + CMCSR_OFFSET(channel);
	uint32_t cmcnt = config->cnt_base + CMCNT_OFFSET(channel);
	uint32_t cmcnth = config->cnt_base + CMCNTH_OFFSET(channel);

	/* Wait for WRFLG bit to 0 */
	while ((sys_read32(cmcsr) & BIT_WRFLG(1u)) != 0) {
	}

	sys_write32(value, cmcnt);
	/* Clear the high counter, allow 32-bit only */
	sys_write32(0u, cmcnth);
}

/* Read channel 0 only. All channels are expected to operate the same */
static int rcar_cmt_get_value(const struct device *dev, uint32_t *ticks)
{
	if (dev == NULL || ticks == NULL) {
		return -EINVAL;
	}
	const struct rcar_cmt_config *cfg = (const struct rcar_cmt_config *)dev->config;
	uint32_t cmcnt = cfg->cnt_base + CMCNT_OFFSET(0);

	*ticks = sys_read32(cmcnt);
	return 0;
}

static int rcar_cmt_cancel_channel_alarm(const struct device *dev, uint8_t channel)
{
	if (dev == NULL) {
		return -EINVAL;
	}
	const struct rcar_cmt_config *config = (const struct rcar_cmt_config *)dev->config;
	uint32_t cmcsr = config->cnt_base + CMCSR_OFFSET(channel);
	uint32_t cmstr = config->cnt_base + CMSTR_OFFSET(channel);
	uint32_t value;

	if (channel > config->counter_info.channels - 1 || sys_read32(cmstr) == 0x00u) {
		return -ENOTSUP;
	}

	/* Disable alarm interrupt */
	value = sys_read32(cmcsr);
	sys_write32(value & ~BIT_CMR(3u), cmcsr);

	return 0;
}

static int rcar_cmt_set_channel_alarm(const struct device *dev, uint8_t channel,
				      const struct counter_alarm_cfg *alarm_cfg)
{
	if (dev == NULL) {
		return -EINVAL;
	}
	const struct rcar_cmt_config *config = (const struct rcar_cmt_config *)dev->config;
	struct rcar_cmt_data *data = (struct rcar_cmt_data *)dev->data;
	uint32_t cmcor = config->cnt_base + CMCOR_OFFSET(channel);
	uint32_t cmcorh = config->cnt_base + CMCORH_OFFSET(channel);
	uint32_t cmcsr = config->cnt_base + CMCSR_OFFSET(channel);
	uint32_t current_tick = 0;
	bool is_late = false;
	uint32_t value;

	if (dev == NULL || alarm_cfg == NULL || channel > config->counter_info.channels - 1) {
		return -ENOTSUP;
	}

	/* Use absolute counting */
	if ((alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) != 0u) {
		rcar_cmt_get_value(dev, &current_tick);
		if (data->guard_flags == COUNTER_GUARD_PERIOD_LATE_TO_SET &&
		    alarm_cfg->ticks - current_tick < data->guard_value) {
			is_late = true;
		} else if (current_tick > alarm_cfg->ticks) {
			is_late = true;
		} else {
			is_late = false;
		}
	} else { /* Use relative counting */
		set_cmcnt(dev, channel, 0u);
	}

	/* Disable interrupt during setting */
	value = sys_read32(cmcsr) & (~BIT_CMR(3u));
	sys_write32(value, cmcsr);

	/* Set callback */
	data->cbs[channel].alarm_callback_func = (void *)alarm_cfg->callback;
	data->cbs[channel].alarm_callback_arg = alarm_cfg->user_data;

	/* Set match value */
	sys_write32(alarm_cfg->ticks, cmcor);
	sys_write32(0u, cmcorh);
	/* Enable alarm interrupt */
	value = sys_read32(cmcsr);
	value = (value & ~BIT_CMR(3u)) | BIT_CMR(2u);
	/* Clear compare match & overflow flag, don't set write flag */
	value &= ~(BIT_CMF(1u) | BIT_OVF(1u) | BIT_WRFLG(1u));
	sys_write32(value, cmcsr);

	if (is_late) {
		/* If allow to expire, make it expire in the next count */
		if ((alarm_cfg->flags & COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE) != 0u) {
			set_cmcnt(dev, channel, alarm_cfg->ticks - 2u);
		}
		return -ETIME;
	}

	return 0;
}

static uint32_t rcar_cmt_get_guard_period(const struct device *dev, uint32_t flags)
{
	if (dev == NULL) {
		return 0;
	}

	const struct rcar_cmt_data *data = (const struct rcar_cmt_data *)dev->data;

	if (flags == COUNTER_GUARD_PERIOD_LATE_TO_SET) {
		return data->guard_value;
	}
	return 0;
}

static int rcar_cmt_set_guard_period(const struct device *dev, uint32_t ticks, uint32_t flags)
{
	if (dev == NULL) {
		return -EINVAL;
	}

	struct rcar_cmt_data *data = (struct rcar_cmt_data *)dev->data;

	data->guard_value = ticks;
	data->guard_flags = flags;
	return 0;
}

/* Limitation: API only returns 1 or 0. No indication for which channel interrupt is
 * or any reading error.
 */
static uint32_t rcar_cmt_get_pending_int(const struct device *dev)
{
	if (dev == NULL) {
		return 0;
	}

	const struct rcar_cmt_config *config = (const struct rcar_cmt_config *)dev->config;
	uint32_t cmcsr = 0;

	for (int ch = 0; ch < config->counter_info.channels; ch++) {
		cmcsr = config->cnt_base + CMCSR_OFFSET(ch);
		if ((sys_read32(cmcsr) & BIT_CMR(3u)) != 0) {
			return 1;
		}
	}
	return 0;
}

static int rcar_cmt_reset(const struct device *dev)
{
	if (dev == NULL) {
		return -EINVAL;
	}

	const struct rcar_cmt_config *config = (const struct rcar_cmt_config *)dev->config;

	for (int ch = 0; ch < config->counter_info.channels; ch++) {
		set_cmcnt(dev, ch, 0);
	}
	return 0;
}

static uint32_t rcar_cmt_get_top_value(const struct device *dev)
{
	return UINT32_MAX;
}

static int rcar_cmt_set_top_value(const struct device *dev, const struct counter_top_cfg *cfg)
{
	return -ENOTSUP;
}

static int rcar_start_stop_cmt(const struct device *dev, bool start)
{
	if (dev == NULL) {
		return -EINVAL;
	}

	const struct rcar_cmt_config *cfg = (const struct rcar_cmt_config *)dev->config;
	uint32_t cmstr = 0;
	uint32_t cmcsr = 0;
	uint32_t val;

	if (dev == NULL) {
		return -ENOTSUP;
	}

	/* All channels are started/stopped */
	for (int ch = 0; ch < cfg->counter_info.channels; ch++) {
		cmstr = cfg->cnt_base + CMSTR_OFFSET(ch);
		cmcsr = cfg->cnt_base + CMCSR_OFFSET(ch);
		sys_write32((start ? 0x01u : 0x00u), cmstr);
		/* Clear all flags */
		val = sys_read32(cmcsr) & ~(BIT_CMF(1u) | BIT_OVF(1u) | BIT_WRFLG(1u));
		sys_write32(val, cmcsr);
	}
	return 0;
}

static int rcar_cmt_start(const struct device *dev)
{
	return rcar_start_stop_cmt(dev, true);
}

static int rcar_cmt_stop(const struct device *dev)
{
	return rcar_start_stop_cmt(dev, false);
}

static int rcar_cmt_overflow_callback(const struct device *dev)
{
	/* Do not support top value callback feature.
	 * This empty function is for future debugging
	 */
	return 0;
}

static void rcar_cmt_irq_handler(const struct rcar_cmt_irq_param *param)
{
	uint8_t channel = param->channel;
	const struct device *dev = param->dev;
	const struct rcar_cmt_config *cfg = (const struct rcar_cmt_config *)dev->config;
	const struct rcar_cmt_data *data = (const struct rcar_cmt_data *)dev->data;
	uint32_t cmcsr = cfg->cnt_base + CMCSR_OFFSET(channel);
	uint32_t value = sys_read32(cmcsr);
	uint32_t ticks = 0;

	/* Check if the IRQ is top or alarm */
	if ((value & ~BIT_OVF(1u)) != 0) { /* Overflow flag */
		/* Run callback */
		rcar_cmt_overflow_callback(dev);
		/* Clear overflow flags */
		value = sys_read32(cmcsr) & ~BIT_OVF(1u);
		sys_write32(value, cmcsr);
	}
	if ((value & ~BIT_CMF(1u)) != 0) { /* Compare match flag */
		/* Disable CMT interrupt */
		value = sys_read32(cmcsr) & ~BIT_CMR(3u);
		sys_write32(value, cmcsr);

		/* Run user callback */
		counter_alarm_callback_t cb =
			(counter_alarm_callback_t)data->cbs[channel].alarm_callback_func;
		if (cb != NULL) {
			rcar_cmt_get_value(dev, &ticks);
			cb(dev, channel, ticks, data->cbs[channel].alarm_callback_arg);
		}

		value = sys_read32(cmcsr);
		/* Clear match flag */
		value &= ~BIT_CMF(1u);
		sys_write32(value, cmcsr);
	}
}

static int rcar_cmt_init(const struct device *dev)
{
	const struct rcar_cmt_config *config = (const struct rcar_cmt_config *)dev->config;
	uint32_t clken = config->clk_en_base;
	uint32_t cnt_base = config->cnt_base;

	/* Configure clock and interrupts */
	config->cfg_func();

	for (uint8_t channel = 0; channel < config->counter_info.channels; channel++) {
		uint32_t value = 0;
		uint32_t cmcsr = cnt_base + CMCSR_OFFSET(channel);
		uint32_t cmcsrh = cnt_base + CMCSRH_OFFSET(channel);
		uint32_t cmstr = cnt_base + CMSTR_OFFSET(channel);

		/* Stop the counter */
		sys_write32(0u, cmstr);
		/* Enable clock to all channels */
		sys_write32(BIT_CLKE(0xFFu), clken);
		/* Need to wait for the clock to take effect */
		wait_for_clk_cycle(dev, 3);

		/* Configure as 32-bit counter */
		value &= ~BIT_CMS(1u);
		/* Compare mode: Free running */
		value |= BIT_CMM(1u);
		/* Run the counter in debug mode */
		value |= BIT_DBGIVD(1u);
		/* Clock division: CPEX/8 */
		value &= ~BIT_CKS(7u);
		sys_write32(value, cmcsr);

		/* Config CMCSRH register with the same setting */
		value = BIT_CMSH(0u) | BIT_CKSH(0u);
		sys_write32(value, cmcsrh);
	}

	return 0;
}

static DEVICE_API(counter, rcar_cmt_driver_api) = {
	.start = rcar_cmt_start,
	.stop = rcar_cmt_stop,
	.reset = rcar_cmt_reset,
	.get_value = rcar_cmt_get_value,
	.set_alarm = rcar_cmt_set_channel_alarm,
	.cancel_alarm = rcar_cmt_cancel_channel_alarm,
	.get_pending_int = rcar_cmt_get_pending_int,
	.get_top_value = rcar_cmt_get_top_value,
	.set_top_value = rcar_cmt_set_top_value,
	.get_guard_period = rcar_cmt_get_guard_period,
	.set_guard_period = rcar_cmt_set_guard_period,
};

#define RCAR_CMT_INIT(n)                                                                        \
	const struct rcar_cmt_irq_param irq_param_##n[] = {                                     \
		{ .dev = DEVICE_DT_INST_GET(n), .channel = 0, },                                \
		{ .dev = DEVICE_DT_INST_GET(n), .channel = 1, },                                \
		{ .dev = DEVICE_DT_INST_GET(n), .channel = 2, },                                \
		{ .dev = DEVICE_DT_INST_GET(n), .channel = 3, },                                \
	};                                                                                      \
	static void rcar_cmt_irq_init_##n(void)                                                 \
	{                                                                                       \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, 0, irq),                                      \
			    IRQ_DEFAULT_PRIORITY,                                               \
			    rcar_cmt_irq_handler,                                               \
			    &irq_param_##n[0],                                                  \
			    DT_INST_IRQ_BY_IDX(n, 0, flags)                                     \
		);                                                                              \
		irq_enable(DT_INST_IRQN_BY_IDX(n, 0));                                          \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, 1, irq),                                      \
			    IRQ_DEFAULT_PRIORITY,                                               \
			    rcar_cmt_irq_handler,                                               \
			    &irq_param_##n[1],                                                  \
			    DT_INST_IRQ_BY_IDX(n, 1, flags)                                     \
		);                                                                              \
		irq_enable(DT_INST_IRQN_BY_IDX(n, 1));                                          \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, 2, irq),                                      \
			    IRQ_DEFAULT_PRIORITY,                                               \
			    rcar_cmt_irq_handler,                                               \
			    &irq_param_##n[2],                                                  \
			    DT_INST_IRQ_BY_IDX(n, 2, flags)                                     \
		);                                                                              \
		irq_enable(DT_INST_IRQN_BY_IDX(n, 2));                                          \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, 3, irq),                                      \
			    IRQ_DEFAULT_PRIORITY,                                               \
			    rcar_cmt_irq_handler,                                               \
			    &irq_param_##n[3],                                                  \
			    DT_INST_IRQ_BY_IDX(n, 3, flags)                                     \
		);                                                                              \
		irq_enable(DT_INST_IRQN_BY_IDX(n, 3));                                          \
	}                                                                                       \
	static void rcar_cmt_config_##n(void)                                                   \
	{                                                                                       \
		const struct device *clk;                                                       \
		struct rcar_cpg_clk mod_clk = {                                                 \
			.module = DT_INST_CLOCKS_CELL(n, module),                               \
			.domain = DT_INST_CLOCKS_CELL(n, domain),                               \
		};                                                                              \
		clk = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n));                                    \
		clock_control_on(clk, (clock_control_subsys_t)&mod_clk);                        \
		rcar_cmt_irq_init_##n();                                                        \
	}                                                                                       \
	static struct rcar_cmt_config config_##n = {                                            \
		.counter_info = { .max_top_value = (uint32_t)UINT32_MAX,                        \
				  .freq = DT_INST_PROP(n, clock_frequency),                     \
				  .flags = COUNTER_CONFIG_INFO_COUNT_UP,                        \
				  .channels =  DT_INST_NUM_IRQS(n)                              \
		},                                                                              \
		.clk_en_base = DT_INST_REG_ADDR_BY_IDX(n, 1),                                   \
		.cnt_base = DT_INST_REG_ADDR_BY_IDX(n, 0),                                      \
		.cfg_func = rcar_cmt_config_##n                                                 \
	};                                                                                      \
	static struct rcar_cmt_data cmt_data_##n;                                               \
	DEVICE_DT_INST_DEFINE(n,                                                                \
		rcar_cmt_init,                                                                  \
		NULL,                                                                           \
		&cmt_data_##n,                                                                  \
		&config_##n,                                                                    \
		POST_KERNEL,                                                                    \
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                                            \
		&rcar_cmt_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RCAR_CMT_INIT)
