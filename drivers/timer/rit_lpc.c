/*
 * Private Porting
 * by David Hor - Xtooltech 2025
 * david.hor@xtooltech.com
 *
 * Copyright (c) 2024 VCI Development
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpc_rit

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/timer/lpc_rit.h>
#include <zephyr/logging/log.h>
#include <soc.h>

LOG_MODULE_REGISTER(rit_lpc, CONFIG_LOG_DEFAULT_LEVEL);

/* RIT registers */
struct rit_regs {
	uint32_t COMPVAL;      /* Compare register LSB */
	uint32_t MASK;         /* Mask register LSB */
	uint32_t CTRL;         /* Control register */
	uint32_t COUNTER;      /* Counter register LSB */
	uint32_t COMPVAL_H;    /* Compare register MSB */
	uint32_t MASK_H;       /* Mask register MSB */
	uint32_t reserved[1];
	uint32_t COUNTER_H;    /* Counter register MSB */
};

/* Control register bits - use SDK definitions from PERI_RIT.h */
/* RIT_CTRL_RITINT_MASK    - Interrupt flag */
/* RIT_CTRL_RITENCLR_MASK  - Timer enable clear */
/* RIT_CTRL_RITENBR_MASK   - Timer enable for debug */
/* RIT_CTRL_RITEN_MASK     - Timer enable */

struct rit_data {
	rit_timer_callback_t callback;
	void *user_data;
	uint32_t freq;
};

struct rit_config {
	struct rit_regs *base;
	void (*irq_config_func)(const struct device *dev);
};

static int rit_configure(const struct device *dev, const struct rit_timer_cfg *cfg)
{
	const struct rit_config *config = dev->config;
	struct rit_data *data = dev->data;
	struct rit_regs *base = config->base;
	uint32_t ctrl = 0;

	if (!cfg) {
		return -EINVAL;
	}

	/* Stop timer */
	base->CTRL &= ~RIT_CTRL_RITEN_MASK;

	/* Clear interrupt flag */
	base->CTRL |= RIT_CTRL_RITINT_MASK;

	/* Set compare value */
	base->COMPVAL = (uint32_t)(cfg->period & 0xFFFFFFFF);
	base->COMPVAL_H = (uint32_t)(cfg->period >> 32);

	/* Clear mask (no masking) */
	base->MASK = 0;
	base->MASK_H = 0;

	/* Configure control register */
	if (cfg->auto_clear) {
		ctrl |= RIT_CTRL_RITENCLR_MASK;
	}
	if (cfg->run_in_debug) {
		ctrl |= RIT_CTRL_RITENBR_MASK;
	}

	base->CTRL = ctrl;

	/* Save callback */
	data->callback = cfg->callback;
	data->user_data = cfg->user_data;

	LOG_DBG("RIT configured: period=0x%llx, auto_clear=%d", cfg->period, cfg->auto_clear);

	return 0;
}

static int rit_start(const struct device *dev)
{
	const struct rit_config *config = dev->config;
	struct rit_regs *base = config->base;

	/* Clear counter */
	base->COUNTER = 0;
	base->COUNTER_H = 0;

	/* Clear interrupt flag */
	base->CTRL |= RIT_CTRL_RITINT_MASK;

	/* Enable timer */
	base->CTRL |= RIT_CTRL_RITEN_MASK;

	LOG_DBG("RIT started");

	return 0;
}

static int rit_stop(const struct device *dev)
{
	const struct rit_config *config = dev->config;
	struct rit_regs *base = config->base;

	/* Disable timer */
	base->CTRL &= ~RIT_CTRL_RITEN_MASK;

	/* Clear interrupt flag */
	base->CTRL |= RIT_CTRL_RITINT_MASK;

	LOG_DBG("RIT stopped");

	return 0;
}

static int rit_get_value(const struct device *dev, uint64_t *value)
{
	const struct rit_config *config = dev->config;
	struct rit_regs *base = config->base;
	uint32_t low, high, high2;

	if (!value) {
		return -EINVAL;
	}

	/* Read with overflow protection */
	do {
		high = base->COUNTER_H;
		low = base->COUNTER;
		high2 = base->COUNTER_H;
	} while (high != high2);

	*value = ((uint64_t)high << 32) | low;

	return 0;
}

static int rit_set_mask(const struct device *dev, uint64_t mask)
{
	const struct rit_config *config = dev->config;
	struct rit_regs *base = config->base;

	base->MASK = (uint32_t)(mask & 0xFFFFFFFF);
	base->MASK_H = (uint32_t)(mask >> 32);

	LOG_DBG("RIT mask set to 0x%llx", mask);

	return 0;
}

static uint32_t rit_get_frequency(const struct device *dev)
{
	struct rit_data *data = dev->data;

	return data->freq;
}

static void rit_isr(const struct device *dev)
{
	const struct rit_config *config = dev->config;
	struct rit_data *data = dev->data;
	struct rit_regs *base = config->base;

	/* Clear interrupt flag */
	base->CTRL |= RIT_CTRL_RITINT_MASK;

	/* Call user callback */
	if (data->callback) {
		data->callback(dev, data->user_data);
	}
}

static const struct rit_driver_api rit_api = {
	.configure = rit_configure,
	.start = rit_start,
	.stop = rit_stop,
	.get_value = rit_get_value,
	.set_mask = rit_set_mask,
	.get_frequency = rit_get_frequency,
};

static int rit_init(const struct device *dev)
{
	const struct rit_config *config = dev->config;
	struct rit_data *data = dev->data;
	struct rit_regs *base = config->base;

	/* RIT runs from the system clock */
	/* For LPC54S018, this is typically 96MHz FRO */
	data->freq = 96000000; /* 96 MHz */

	/* Reset and disable timer */
	base->CTRL = 0;
	base->COUNTER = 0;
	base->COUNTER_H = 0;
	base->COMPVAL = 0;
	base->COMPVAL_H = 0;
	base->MASK = 0;
	base->MASK_H = 0;

	/* Configure interrupts */
	config->irq_config_func(dev);

	LOG_DBG("RIT initialized, freq=%u Hz", data->freq);

	return 0;
}

#define RIT_INIT(n)                                                    \
	static void rit_##n##_irq_config(const struct device *dev)    \
	{                                                              \
		IRQ_CONNECT(DT_INST_IRQN(n),                          \
			    DT_INST_IRQ(n, priority),                 \
			    rit_isr,                                  \
			    DEVICE_DT_INST_GET(n),                    \
			    0);                                       \
		irq_enable(DT_INST_IRQN(n));                         \
	}                                                              \
									\
	static struct rit_data rit_##n##_data;                        \
									\
	static const struct rit_config rit_##n##_config = {           \
		.base = (struct rit_regs *)DT_INST_REG_ADDR(n),      \
		.irq_config_func = rit_##n##_irq_config,             \
	};                                                             \
									\
	DEVICE_DT_INST_DEFINE(n,                                       \
			      rit_init,                                \
			      NULL,                                    \
			      &rit_##n##_data,                         \
			      &rit_##n##_config,                       \
			      POST_KERNEL,                             \
			      CONFIG_TIMER_INIT_PRIORITY,              \
			      &rit_api);

DT_INST_FOREACH_STATUS_OKAY(RIT_INIT)