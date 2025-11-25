/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rcar_cmt_counter

#include <stdlib.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/renesas_cpg_mssr.h>

/* Compare match timer start register channel n */
#define CMSTR_OFFSET (0x0000u)

/* Compare match timer control/status register channel n */
#define CMCSR_OFFSET (0x0010u)
#define BIT_CMF      BIT(15u)      /* Compare match flag */
#define BIT_OVF      BIT(14u)      /* Overflow flag */
#define BIT_WRFLG    BIT(13u)      /* Write state flag */
#define BIT_CMS      BIT(9u)       /* Compare match counter size */
#define BIT_CMM      BIT(8u)       /* Compare match mode */
#define BIT_DBGIVD   BIT(3u)       /* Debug mode operation */
#define BIT_CMR_MASK GENMASK(5, 4) /* Compare match request interrupt mask */
#define BIT_CKS_MASK GENMASK(2, 0) /* Clock Select */

/* Compare match timer counter channel n */
#define CMCNT_OFFSET (0x0014u)

/* Compare match timer constant register channel n */
#define CMCOR_OFFSET (0x0018u)

/* Compare match timer H control/status register channel n */
#define CMCSRH_OFFSET (0x0020u)

/* Compare match timer counter H0 channel n */
#define CMCNTH_OFFSET (0x0024u)

/* Compare match timer constant H0 register channel n */
#define CMCORH_OFFSET (0x0028u)

/* CLK Enable register, base address and offset get from devicetree */
#define BIT_CLKE_MASK GENMASK(7, 0) /* Channel clock mask */

struct rcar_cmt_config {
	struct counter_config_info counter_info;
	uint32_t clk_en_base;
	uint32_t cnt_base;
	uint32_t channel_index;
	const struct device *clk_dev;
	const struct rcar_cpg_clk mod_clk;
	const uint32_t int_id;
	void (*cfg_func)(void);
};

struct channel_callback {
	bool is_active;
	counter_alarm_callback_t alarm_callback_func;
	void *user_data;
};

struct rcar_cmt_data {
	struct channel_callback cbs;
	uint32_t guard_value;
	uint32_t guard_flags;
	uint32_t top_value;
};

struct rcar_cmt_irq_param {
	const struct device *dev;
};

struct k_spinlock cmt_lock;

/* Wait for a number of CMT input clock cycles */
static void wait_for_clk_cycle(const struct device *dev, uint32_t cycles)
{
	const struct rcar_cmt_config *config = (const struct rcar_cmt_config *)dev->config;
	const uint32_t clk_freq = config->counter_info.freq;
	uint32_t usec_to_wait = (uint32_t)(cycles * 1000000ULL / clk_freq) + 1; /* Round up */

	k_busy_wait(usec_to_wait);
}

/* Wait until Write Flag bit is clear, or return error if the expected timeout reached */
static int poll_wrflg_bit(const struct device *dev)
{
	const struct rcar_cmt_config *config = (const struct rcar_cmt_config *)dev->config;
	const uint32_t cmcsr = config->cnt_base + CMCSR_OFFSET;
	uint32_t wait_cycle = 0;

	/* WRFLG must be cleared in at most 2 input clock cycles from previous write access */
	while (((sys_read32(cmcsr) & BIT_WRFLG) != 0) && (wait_cycle < 2)) {
		wait_for_clk_cycle(dev, 1u);
		wait_cycle++;
	}

	return ((sys_read32(cmcsr) & BIT_WRFLG) != 0) ? -ETIMEDOUT : 0;
}

/* Set counter register value for the specified channel */
static int set_cmcnt(const struct device *dev, uint32_t value)
{
	const struct rcar_cmt_config *config = (const struct rcar_cmt_config *)dev->config;
	uint32_t cmcnt = config->cnt_base + CMCNT_OFFSET;
	uint32_t cmcnth = config->cnt_base + CMCNTH_OFFSET;

	/* Wait for WRFLG bit to 0 */
	if (poll_wrflg_bit(dev) != 0) {
		return -ETIMEDOUT;
	}

	sys_write32(value, cmcnt);
	/* Clear the high counter, allow 32-bit only */
	sys_write32(0u, cmcnth);

	return 0;
}

static int rcar_cmt_get_value(const struct device *dev, uint32_t *ticks)
{
	if (dev == NULL || ticks == NULL) {
		return -EINVAL;
	}
	const struct rcar_cmt_config *config = (const struct rcar_cmt_config *)dev->config;
	uint32_t cmcnt = config->cnt_base + CMCNT_OFFSET;

	*ticks = sys_read32(cmcnt);
	return 0;
}

static int rcar_cmt_cancel_channel_alarm(const struct device *dev, uint8_t channel)
{
	if (dev == NULL) {
		return -EINVAL;
	}

	const struct rcar_cmt_config *config = (const struct rcar_cmt_config *)dev->config;
	struct rcar_cmt_data *data = (struct rcar_cmt_data *)dev->data;
	uint32_t cmcsr = config->cnt_base + CMCSR_OFFSET;
	uint32_t cmstr = config->cnt_base + CMSTR_OFFSET;
	uint32_t cmcor = config->cnt_base + CMCOR_OFFSET;
	uint32_t value;

	if (channel > 0 || sys_read32(cmstr) == 0x00u) {
		return -ENOTSUP;
	}

	/* Disable alarm interrupt */
	value = sys_read32(cmcsr) & ~BIT_CMR_MASK;
	sys_write32(value, cmcsr);

	/* Clear callback data */
	data->cbs.alarm_callback_func = NULL;
	data->cbs.user_data = NULL;
	data->cbs.is_active = false;

	/* Set match value to the top value to avoid
	 * counter restart when reaching previous match
	 */
	sys_write32(UINT32_MAX, cmcor);
	data->top_value = UINT32_MAX;

	return 0;
}

static int rcar_cmt_set_channel_alarm(const struct device *dev, uint8_t channel,
				      const struct counter_alarm_cfg *alarm_cfg)
{
	if (dev == NULL || alarm_cfg == NULL || alarm_cfg->callback == NULL) {
		return -EINVAL;
	}
	const struct rcar_cmt_config *config = (const struct rcar_cmt_config *)dev->config;

	if (channel > 0u) {
		return -ENOTSUP;
	}

	struct rcar_cmt_data *data = (struct rcar_cmt_data *)dev->data;
	uint32_t cmcor = config->cnt_base + CMCOR_OFFSET;
	uint32_t cmcorh = config->cnt_base + CMCORH_OFFSET;
	uint32_t cmcsr = config->cnt_base + CMCSR_OFFSET;
	uint32_t current_tick = 0;
	bool is_late = false;
	bool is_relative = false;
	uint32_t value;

	/* Check if the current channel is still active */
	value = sys_read32(cmcsr);
	if ((value & BIT_CMR_MASK) != 0 || data->cbs.is_active) {
		return -EBUSY;
	}

	/* Use absolute counting */
	rcar_cmt_get_value(dev, &current_tick);
	if ((alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) != 0u) {
		if (data->guard_flags & COUNTER_GUARD_PERIOD_LATE_TO_SET) {
			if (current_tick - (alarm_cfg->ticks - data->guard_value) <
			    data->guard_value) {
				/* The current tick did not pass the target but too close */
				is_late = true;
			} else if (alarm_cfg->ticks < current_tick) {
				/* The current tick passed the target but not yet wrapped around */
				is_late = true;
			} else {
				is_late = false;
			}
		} else {
			is_late = false;
		}
	} else { /* Use relative counting */
		is_relative = true;
	}

	if (is_late && (alarm_cfg->flags & COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE) == 0u) {
		return -ETIME;
	}

	/* Disable interrupt during setting */
	value = sys_read32(cmcsr) & (~BIT_CMR_MASK);
	sys_write32(value, cmcsr);

	/* Set callback */
	data->cbs.alarm_callback_func = alarm_cfg->callback;
	data->cbs.user_data = alarm_cfg->user_data;
	data->cbs.is_active = true;

	/* Set match value */
	sys_write32(is_relative ? alarm_cfg->ticks + current_tick : alarm_cfg->ticks, cmcor);
	sys_write32(0u, cmcorh);
	/* This top value is set to alarm value because
	 * the counter will turn around on match
	 */
	data->top_value = sys_read32(cmcor);

	/* Enable alarm interrupt */
	value = sys_read32(cmcsr);
	value = (value & ~BIT_CMR_MASK) | FIELD_PREP(BIT_CMR_MASK, 2u);
	/* Clear compare match & overflow flag, don't set write flag */
	value &= ~(BIT_CMF | BIT_OVF | BIT_WRFLG);
	sys_write32(value, cmcsr);

	/* If allow to expire, make it expire in the next count */
	if (is_late && (alarm_cfg->flags & COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE) != 0u) {
		/* At least 2 input clock cycle needed for the setting value
		 * to take effect. So, we set the expire value to next 3 cycles.
		 */
		rcar_cmt_get_value(dev, &current_tick);
		sys_write32(current_tick + 3u, cmcor);
		data->top_value = sys_read32(cmcor);
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

	if (flags & COUNTER_GUARD_PERIOD_LATE_TO_SET) {
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

static uint32_t rcar_cmt_get_pending_int(const struct device *dev)
{
	if (dev == NULL) {
		return 0;
	}

	const struct rcar_cmt_config *config = (const struct rcar_cmt_config *)dev->config;
	uint32_t cmcsr = config->cnt_base + CMCSR_OFFSET;

	if ((sys_read32(cmcsr) & BIT_CMR_MASK) != 0 && irq_is_enabled(config->int_id)) {
		return 1;
	}
	return 0;
}

static int rcar_cmt_reset(const struct device *dev)
{
	if (dev == NULL) {
		return -EINVAL;
	}

	if (set_cmcnt(dev, 0) != 0) {
		return -ETIMEDOUT;
	}

	return 0;
}

static uint32_t rcar_cmt_get_top_value(const struct device *dev)
{
	struct rcar_cmt_data *data = (struct rcar_cmt_data *)dev->data;

	return data->top_value;
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

	const struct rcar_cmt_config *config = (const struct rcar_cmt_config *)dev->config;
	uint32_t cmstr = config->cnt_base + CMSTR_OFFSET;

	sys_write32((start ? 0x01u : 0x00u), cmstr);

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
	const struct device *dev = param->dev;
	const struct rcar_cmt_config *config = (const struct rcar_cmt_config *)dev->config;
	struct rcar_cmt_data *data = (struct rcar_cmt_data *)dev->data;
	uint32_t cmcsr = config->cnt_base + CMCSR_OFFSET;
	uint32_t cmcor = config->cnt_base + CMCOR_OFFSET;
	uint32_t value = sys_read32(cmcsr);

	/* Check if the IRQ is top or alarm */
	if ((value & BIT_OVF) != 0) { /* Overflow flag */
		/* Run callback */
		rcar_cmt_overflow_callback(dev);
		/* Clear overflow flags */
		value = sys_read32(cmcsr) & ~BIT_OVF;
		sys_write32(value, cmcsr);
	}
	if ((value & BIT_CMF) != 0) { /* Compare match flag */
		counter_alarm_callback_t callback = data->cbs.alarm_callback_func;
		void *user_data = data->cbs.user_data;

		data->cbs.alarm_callback_func = NULL;
		data->cbs.user_data = NULL;
		data->cbs.is_active = false;

		/* Clear match flag and disable CMT interrupt */
		value = sys_read32(cmcsr);
		value &= ~(BIT_CMF | BIT_CMR_MASK);
		sys_write32(value, cmcsr);

		/* Run user callback */
		callback(dev, 0u, sys_read32(cmcor), user_data);
	}
}

static int rcar_cmt_init(const struct device *dev)
{
	const struct rcar_cmt_config *config = (const struct rcar_cmt_config *)dev->config;
	struct rcar_cmt_data *data = (struct rcar_cmt_data *)dev->data;
	uint32_t clken = config->clk_en_base;
	uint32_t cnt_base = config->cnt_base;
	uint32_t cmcsr = cnt_base + CMCSR_OFFSET;
	uint32_t cmcsrh = cnt_base + CMCSRH_OFFSET;
	uint32_t cmstr = cnt_base + CMSTR_OFFSET;
	uint32_t cmcor = cnt_base + CMCOR_OFFSET;
	uint32_t cmcorh = cnt_base + CMCORH_OFFSET;
	uint32_t value = 0;
	int ret;

	/* Check for valid configurations */
	if (config->counter_info.freq == 0u) {
		return -EINVAL;
	}

	/* Configure clock and interrupts */
	irq_disable(config->int_id);
	config->cfg_func();

	if (!device_is_ready(config->clk_dev)) {
		return -EBUSY;
	}
	ret = clock_control_on(config->clk_dev, (const clock_control_subsys_t)&config->mod_clk);
	if (ret != 0) {
		return ret;
	}

	/* Stop the counter */
	sys_write32(0u, cmstr);
	/* Enable clock for this channels */
	K_SPINLOCK(&cmt_lock) {
		value = (sys_read32(clken) | BIT(config->channel_index)) & BIT_CLKE_MASK;
		sys_write32(value, clken);
	}
	/* Need to wait for the clock to take effect */
	wait_for_clk_cycle(dev, 3);

	/* Default value: 32-bit counter, disable compare interrupt, clock division: CPEX/8 */
	value = 0u;
	/* Compare mode: Free running */
	value |= BIT_CMM;
	/* Run the counter in debug mode */
	value |= BIT_DBGIVD;
	sys_write32(value, cmcsr);

	/* Clear CMCSRH register for 32-bit counter, CPEX/8 clock division setting */
	value = 0;
	sys_write32(value, cmcsrh);

	/* Reset counter and match value */
	ret = rcar_cmt_reset(dev);
	if (ret != 0) {
		return ret;
	}
	sys_write32(UINT32_MAX, cmcor);
	sys_write32(0u, cmcorh);

	/* Clear device data */
	data->cbs.alarm_callback_func = NULL;
	data->cbs.user_data = NULL;
	data->cbs.is_active = false;
	data->top_value = UINT32_MAX;

	irq_enable(config->int_id);

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

#define RCAR_CMT_NODE(idx) DT_INST_PARENT(idx)

#define RCAR_CMT_INIT(n)                                                                           \
	const struct rcar_cmt_irq_param irq_param_##n = {.dev = DEVICE_DT_INST_GET(n)};            \
	static void rcar_cmt_config_##n(void)                                                      \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQ(n, irq), IRQ_DEFAULT_PRIORITY, rcar_cmt_irq_handler,       \
			    &irq_param_##n, DT_INST_IRQ(n, flags));                                \
	}                                                                                          \
	static const struct rcar_cmt_config config_##n = {                                         \
		.counter_info = {.max_top_value = (uint32_t)UINT32_MAX,                            \
				 .freq = DT_PROP(RCAR_CMT_NODE(n), clock_frequency),               \
				 .flags = COUNTER_CONFIG_INFO_COUNT_UP,                            \
				 .channels = 1},                                                   \
		.clk_en_base = DT_REG_ADDR_BY_IDX(RCAR_CMT_NODE(n), 1),                            \
		.cnt_base =                                                                        \
			DT_REG_ADDR_BY_IDX(RCAR_CMT_NODE(n), 0) + DT_INST_REG_ADDR_BY_IDX(n, 0),   \
		.channel_index = DT_INST_PROP(n, channel_index),                                   \
		.clk_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR(RCAR_CMT_NODE(n))),                        \
		.mod_clk =                                                                         \
			{                                                                          \
				.module = DT_CLOCKS_CELL(RCAR_CMT_NODE(n), module),                \
				.domain = DT_CLOCKS_CELL(RCAR_CMT_NODE(n), domain),                \
			},                                                                         \
		.int_id = DT_INST_IRQN(n),                                                         \
		.cfg_func = rcar_cmt_config_##n};                                                  \
	static struct rcar_cmt_data cmt_data_##n;                                                  \
	DEVICE_DT_INST_DEFINE(n,                                                                   \
			      rcar_cmt_init,                                                       \
			      NULL,                                                                \
			      &cmt_data_##n,                                                       \
			      &config_##n,                                                         \
			      POST_KERNEL,                                                         \
			      CONFIG_COUNTER_INIT_PRIORITY,                                        \
			      &rcar_cmt_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RCAR_CMT_INIT)
