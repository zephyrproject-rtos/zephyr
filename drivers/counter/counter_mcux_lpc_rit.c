/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpc_rit

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>

#include <fsl_clock.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(counter_mcux_lpc_rit, CONFIG_COUNTER_LOG_LEVEL);

/*
 * The RIT counts up to a single 48 bit compare value and can clear itself on
 * the match. That one compare register is what produces the wrap, so it backs
 * the counter top value and the device has no alarm channels.
 *
 * The upper 16 bits of the match are masked, so only the low 32 bits of the
 * counter take part: a counter that is past the top value wraps through zero
 * at 2^32 and matches again, like any other 32 bit counter.
 *
 * RITINT in CTRL is write-one-to-clear and bits 31:4 are reserved (write
 * zero), so a read-modify-write of CTRL keeps only the enable bits and sets
 * RITINT only to clear a pending match.
 */
struct counter_mcux_lpc_rit_config {
	struct counter_config_info info;
	RIT_Type *base;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	void (*irq_config_func)(const struct device *dev);
};

struct counter_mcux_lpc_rit_data {
	counter_top_callback_t top_callback;
	void *top_user_data;
	uint32_t freq;
};

#define COUNTER_MCUX_LPC_RIT_CTRL_KEEP                                                             \
	(RIT_CTRL_RITENCLR_MASK | RIT_CTRL_RITENBR_MASK | RIT_CTRL_RITEN_MASK)

static void counter_mcux_lpc_rit_ctrl_update(RIT_Type *base, uint32_t clear, uint32_t set)
{
	base->CTRL = (base->CTRL & COUNTER_MCUX_LPC_RIT_CTRL_KEEP & ~clear) | set;
}

static void counter_mcux_lpc_rit_clear_int(RIT_Type *base)
{
	counter_mcux_lpc_rit_ctrl_update(base, 0U, RIT_CTRL_RITINT_MASK);
}

/* The counter may only be reloaded while it is stopped (UM10912, RI timer operation). */
static void counter_mcux_lpc_rit_reset(RIT_Type *base)
{
	uint32_t enabled = base->CTRL & RIT_CTRL_RITEN_MASK;

	counter_mcux_lpc_rit_ctrl_update(base, RIT_CTRL_RITEN_MASK, 0U);
	base->COUNTER = 0U;
	base->COUNTER_H = 0U;
	counter_mcux_lpc_rit_ctrl_update(base, 0U, enabled);
}

static int counter_mcux_lpc_rit_start(const struct device *dev)
{
	const struct counter_mcux_lpc_rit_config *config = dev->config;

	counter_mcux_lpc_rit_ctrl_update(config->base, 0U, RIT_CTRL_RITEN_MASK);

	return 0;
}

static int counter_mcux_lpc_rit_stop(const struct device *dev)
{
	const struct counter_mcux_lpc_rit_config *config = dev->config;

	counter_mcux_lpc_rit_ctrl_update(config->base, RIT_CTRL_RITEN_MASK, 0U);

	return 0;
}

static int counter_mcux_lpc_rit_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct counter_mcux_lpc_rit_config *config = dev->config;

	*ticks = config->base->COUNTER;

	return 0;
}

static int counter_mcux_lpc_rit_set_top_value(const struct device *dev,
					      const struct counter_top_cfg *cfg)
{
	const struct counter_mcux_lpc_rit_config *config = dev->config;
	struct counter_mcux_lpc_rit_data *data = dev->data;
	RIT_Type *base = config->base;

	data->top_callback = cfg->callback;
	data->top_user_data = cfg->user_data;

	if ((cfg->flags & COUNTER_TOP_CFG_DONT_RESET) == 0U) {
		uint32_t enabled = base->CTRL & RIT_CTRL_RITEN_MASK;

		/* Reload top and counter while stopped, so no match falls in between. */
		counter_mcux_lpc_rit_ctrl_update(base, RIT_CTRL_RITEN_MASK, 0U);
		base->COMPVAL = cfg->ticks;
		base->COUNTER = 0U;
		base->COUNTER_H = 0U;
		counter_mcux_lpc_rit_clear_int(base);
		counter_mcux_lpc_rit_ctrl_update(base, 0U, enabled);
		return 0;
	}

	/* The timer keeps running; only the low compare word takes part in the match. */
	base->COMPVAL = cfg->ticks;

	/*
	 * A match after the compare write clears the counter, so it then reads
	 * below the new top and is not reported as late.
	 */
	if (base->COUNTER > cfg->ticks) {
		if ((cfg->flags & COUNTER_TOP_CFG_RESET_WHEN_LATE) != 0U) {
			counter_mcux_lpc_rit_reset(base);
		}
		return -ETIME;
	}

	return 0;
}

static uint32_t counter_mcux_lpc_rit_get_top_value(const struct device *dev)
{
	const struct counter_mcux_lpc_rit_config *config = dev->config;

	return config->base->COMPVAL;
}

static uint32_t counter_mcux_lpc_rit_get_pending_int(const struct device *dev)
{
	const struct counter_mcux_lpc_rit_config *config = dev->config;

	return ((config->base->CTRL & RIT_CTRL_RITINT_MASK) != 0U) ? 1U : 0U;
}

static uint32_t counter_mcux_lpc_rit_get_freq(const struct device *dev)
{
	const struct counter_mcux_lpc_rit_data *data = dev->data;

	return data->freq;
}

static void counter_mcux_lpc_rit_isr(const struct device *dev)
{
	const struct counter_mcux_lpc_rit_config *config = dev->config;
	struct counter_mcux_lpc_rit_data *data = dev->data;

	counter_mcux_lpc_rit_clear_int(config->base);

	if (data->top_callback != NULL) {
		data->top_callback(dev, data->top_user_data);
	}
}

static int counter_mcux_lpc_rit_init(const struct device *dev)
{
	const struct counter_mcux_lpc_rit_config *config = dev->config;
	struct counter_mcux_lpc_rit_data *data = dev->data;
	RIT_Type *base = config->base;
	int ret;

	if (!device_is_ready(config->clock_dev)) {
		return -ENODEV;
	}

	ret = clock_control_get_rate(config->clock_dev, config->clock_subsys, &data->freq);
	if (ret != 0) {
		return ret;
	}

	CLOCK_EnableClock(kCLOCK_Rit);

	/*
	 * The RIT has no reset line, so a warm restart finds it as the previous
	 * image left it: stop it, then set up clear-on-match, the 32 bit match
	 * mask, the top value and a cleared counter and flag. RITENBR keeps its
	 * reset value, so the timer halts while a debugger halts the CPU.
	 */
	base->CTRL = RIT_CTRL_RITENCLR_MASK | RIT_CTRL_RITENBR_MASK | RIT_CTRL_RITINT_MASK;
	base->MASK = 0U;
	base->MASK_H = 0xFFFFU;
	base->COMPVAL = config->info.max_top_value;
	base->COMPVAL_H = 0U;
	base->COUNTER = 0U;
	base->COUNTER_H = 0U;

	config->irq_config_func(dev);

	return 0;
}

static DEVICE_API(counter, counter_mcux_lpc_rit_api) = {
	.start = counter_mcux_lpc_rit_start,
	.stop = counter_mcux_lpc_rit_stop,
	.get_value = counter_mcux_lpc_rit_get_value,
	.set_top_value = counter_mcux_lpc_rit_set_top_value,
	.get_top_value = counter_mcux_lpc_rit_get_top_value,
	.get_pending_int = counter_mcux_lpc_rit_get_pending_int,
	.get_freq = counter_mcux_lpc_rit_get_freq,
};

#define COUNTER_MCUX_LPC_RIT_INIT(n)                                                               \
	static void counter_mcux_lpc_rit_irq_config_##n(const struct device *dev)                  \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), counter_mcux_lpc_rit_isr,   \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
                                                                                                   \
	static struct counter_mcux_lpc_rit_data counter_mcux_lpc_rit_data_##n;                     \
                                                                                                   \
	static const struct counter_mcux_lpc_rit_config counter_mcux_lpc_rit_config_##n = {        \
		.info = {                                                                          \
			.max_top_value = UINT32_MAX,                                               \
			.flags = COUNTER_CONFIG_INFO_COUNT_UP,                                     \
			.channels = 0,                                                             \
		},                                                                                 \
		.base = (RIT_Type *)DT_INST_REG_ADDR(n),                                           \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),              \
		.irq_config_func = counter_mcux_lpc_rit_irq_config_##n,                            \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, counter_mcux_lpc_rit_init, NULL, &counter_mcux_lpc_rit_data_##n,  \
			      &counter_mcux_lpc_rit_config_##n, POST_KERNEL,                       \
			      CONFIG_COUNTER_INIT_PRIORITY, &counter_mcux_lpc_rit_api);

DT_INST_FOREACH_STATUS_OKAY(COUNTER_MCUX_LPC_RIT_INIT)
