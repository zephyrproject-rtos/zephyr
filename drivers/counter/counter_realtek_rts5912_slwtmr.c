/*
 * Copyright (c) 2025 Realtek Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT realtek_rts5912_slwtimer

/**
 * @file
 * @brief Realtek RTS5912 Counter driver
 *
 * This is the driver for the 32-bit counters on the Realtek SoCs.
 *
 * Notes:
 * - The counters are running in down counting mode.
 * - Interrupts are triggered (if enabled) when the counter
 *   reaches zero.
 * - These are not free running counters where there are separate
 *   compare values for interrupts. When setting single shot alarms,
 *   the counter values are changed so that interrupts are triggered
 *   when the counters reach zero.
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_rts5912.h>
#include <zephyr/logging/log.h>
#include "reg/reg_slwtmr.h"
LOG_MODULE_REGISTER(counter_realtek_rts5912_slwtmr, CONFIG_COUNTER_LOG_LEVEL);

struct counter_rts5912_config {
	struct counter_config_info info;
	void (*config_func)(void);
	volatile struct slwtmr_type *base_address;
	uint16_t prescaler;
#if (CONFIG_CLOCK_CONTROL)
	const struct device *clk_dev;
	struct rts5912_sccon_subsys sccon_cfg;
#endif
};

struct counter_rts5912_data {
	counter_alarm_callback_t alarm_cb;
	counter_top_callback_t top_cb;
	void *user_data;
};

static int counter_rts5912_start(const struct device *dev)
{
	const struct counter_rts5912_config *config = dev->config;
	volatile struct slwtmr_type *counter = config->base_address;

	if (counter->ctrl & SLWTMR_CTRL_EN) {
		return -EALREADY;
	}

	counter->ctrl |= SLWTMR_CTRL_EN;

	LOG_DBG("%p Counter started", dev);

	return 0;
}

static int counter_rts5912_stop(const struct device *dev)
{
	const struct counter_rts5912_config *config = dev->config;
	volatile struct slwtmr_type *counter = config->base_address;

	if (!(counter->ctrl & SLWTMR_CTRL_EN)) {
		/* Already stopped, nothing to do */
		return 0;
	}

	/* disable timer and disable interrupt. */
	counter->ctrl = 0;
	counter->ldcnt = counter->cnt;

	/* w1c interrupt pending status */
	counter->insts |= SLWTMR_INTSTS_STS;

	LOG_DBG("%p Counter stopped", dev);

	return 0;
}

static int counter_rts5912_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct counter_rts5912_config *config = dev->config;
	volatile struct slwtmr_type *counter = config->base_address;

	*ticks = counter->cnt;
	return 0;
}

static int counter_rts5912_set_alarm(const struct device *dev, uint8_t chan_id,
				     const struct counter_alarm_cfg *alarm_cfg)
{
	const struct counter_rts5912_config *config = dev->config;
	volatile struct slwtmr_type *counter = config->base_address;
	struct counter_rts5912_data *data = dev->data;

	if (chan_id != 0) {
		LOG_ERR("Invalid channel id %u", chan_id);
		return -ENOTSUP;
	}

	/* Interrupts are only triggered when the counter reaches 0.
	 * So only relative alarms are supported.
	 */
	if (alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) {
		return -ENOTSUP;
	}

	if (data->alarm_cb != NULL) {
		return -EBUSY;
	}

	if (!alarm_cfg->callback) {
		return -EINVAL;
	}

	if (alarm_cfg->ticks > config->info.max_top_value) {
		return -EINVAL;
	}
	/* disable timer and disable interrupt */
	counter->ctrl = 0;

	counter->ldcnt = alarm_cfg->ticks;

	data->alarm_cb = alarm_cfg->callback;
	data->user_data = alarm_cfg->user_data;
	/* w1c interrupt status */
	counter->insts |= SLWTMR_INTSTS_STS;
	/* enable interrupt */
	counter->ctrl |= SLWTMR_CTRL_INTEN_EN;

	LOG_DBG("%p Counter alarm set to %u ticks", dev, alarm_cfg->ticks);
	/* enable timer and re-load PRcnt to cnt */
	counter->ctrl |= SLWTMR_CTRL_EN;

	return 0;
}

static int counter_rts5912_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	const struct counter_rts5912_config *config = dev->config;
	volatile struct slwtmr_type *counter = config->base_address;
	struct counter_rts5912_data *data = dev->data;

	if (chan_id != 0) {
		LOG_ERR("Invalid channel id %u", chan_id);
		return -ENOTSUP;
	}

	counter->ctrl = 0;

	data->alarm_cb = NULL;
	data->user_data = NULL;

	LOG_DBG("%p Counter alarm canceled", dev);

	return 0;
}

static uint32_t counter_rts5912_get_pending_int(const struct device *dev)
{
	const struct counter_rts5912_config *config = dev->config;
	volatile struct slwtmr_type *counter = config->base_address;

	return counter->insts;
}

static uint32_t counter_rts5912_get_top_value(const struct device *dev)
{
	const struct counter_rts5912_config *config = dev->config;
	volatile struct slwtmr_type *counter = config->base_address;

	return counter->ldcnt;
}

static int counter_rts5912_set_top_value(const struct device *dev,
					 const struct counter_top_cfg *cfg)
{
	const struct counter_rts5912_config *config = dev->config;
	volatile struct slwtmr_type *counter = config->base_address;
	struct counter_rts5912_data *data = dev->data;
	int ret = 0;

	if (data->alarm_cb) {
		return -EBUSY;
	}

	if (cfg->ticks > config->info.max_top_value) {
		return -EINVAL;
	}

	counter->ctrl = 0;

	counter->ldcnt = cfg->ticks;

	data->top_cb = cfg->callback;
	data->user_data = cfg->user_data;

	if (data->top_cb) {
		/* w1c interrupt status */
		counter->insts |= SLWTMR_INTSTS_STS;
		/* enable interrupt */
		counter->ctrl |= SLWTMR_CTRL_INTEN_EN;
		/* enable periodic alarm mode */
		counter->ctrl |= SLWTMR_CTRL_MDSELS_PERIOD;
	} else {
		/* disable interrupt */
		counter->ctrl &= ~SLWTMR_CTRL_INTEN_EN;
	}

	LOG_DBG("%p Counter top value was set to %u", dev, cfg->ticks);

	counter->ctrl |= SLWTMR_CTRL_EN;
	return ret;
}

static void counter_rts5912_isr(const struct device *dev)
{
	const struct counter_rts5912_config *config = dev->config;
	volatile struct slwtmr_type *counter = config->base_address;
	struct counter_rts5912_data *data = dev->data;
	counter_alarm_callback_t alarm_cb;
	void *user_data;
	/* disable timer */
	counter->ctrl &= ~SLWTMR_CTRL_EN;
	/* disable interrupt */
	counter->ctrl &= ~SLWTMR_CTRL_INTEN_EN;
	/* clear interrupt pending status */
	counter->insts |= SLWTMR_INTSTS_STS;

	LOG_DBG("%p Counter ISR", dev);

	if (data->alarm_cb) {
		/* Alarm is one-shot, so disable callback */
		alarm_cb = data->alarm_cb;
		data->alarm_cb = NULL;
		user_data = data->user_data;

		alarm_cb(dev, 0, counter->cnt, user_data);
	} else if (data->top_cb) {
		data->top_cb(dev, data->user_data);
		/* periodic alarm mode, enable interrupt */
		counter->ctrl |= SLWTMR_CTRL_INTEN_EN;
		/* enable timer again */
		counter->ctrl |= SLWTMR_CTRL_EN;
	}
}

static uint32_t counter_rts5912_get_freq(const struct device *dev)
{
	const struct counter_rts5912_config *config = dev->config;

	return config->info.freq;
}

static DEVICE_API(counter, counter_rts5912_api) = {
	.start = counter_rts5912_start,
	.stop = counter_rts5912_stop,
	.get_value = counter_rts5912_get_value,
	.set_alarm = counter_rts5912_set_alarm,
	.cancel_alarm = counter_rts5912_cancel_alarm,
	.set_top_value = counter_rts5912_set_top_value,
	.get_pending_int = counter_rts5912_get_pending_int,
	.get_top_value = counter_rts5912_get_top_value,
	.get_freq = counter_rts5912_get_freq,
};

static int counter_rts5912_init(const struct device *dev)
{
	const struct counter_rts5912_config *config = dev->config;
	volatile struct slwtmr_type *counter = config->base_address;
	int ret;

#if defined(CONFIG_CLOCK_CONTROL)
	if (!device_is_ready(config->clk_dev)) {
		return -ENODEV;
	}
	ret = clock_control_on(config->clk_dev, (clock_control_subsys_t)&config->sccon_cfg);
	if (ret != 0) {
		return ret;
	}
#endif

	counter_rts5912_stop(dev);
	/* Set preload and actually pre-load the counter */
	counter->ldcnt = config->info.max_top_value;
	counter->cnt = config->info.max_top_value;

	config->config_func();
	LOG_DBG("Init Complete");
	return 0;
}

#define DEV_CONFIG_CLK_DEV_INIT(n)                                                                 \
	.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                          \
	.sccon_cfg = {                                                                             \
		.clk_grp = DT_INST_CLOCKS_CELL_BY_NAME(n, slwtmr, clk_grp),                        \
		.clk_idx = DT_INST_CLOCKS_CELL_BY_NAME(n, slwtmr, clk_idx),                        \
	}

#define COUNTER_RTS5912_INIT(inst)                                                                 \
	static void counter_rts5912_irq_config_##inst(void);                                       \
                                                                                                   \
	static struct counter_rts5912_data counter_rts5912_dev_data_##inst;                        \
                                                                                                   \
	static struct counter_rts5912_config counter_rts5912_dev_config_##inst = {                 \
		.info =                                                                            \
			{                                                                          \
				.max_top_value = DT_INST_PROP(inst, max_value),                    \
				.freq = DT_INST_PROP(inst, clock_frequency) /                      \
					(1 << DT_INST_PROP(inst, prescaler)),                      \
				.flags = 0,                                                        \
				.channels = 1,                                                     \
			},                                                                         \
                                                                                                   \
		.config_func = counter_rts5912_irq_config_##inst,                                  \
		.base_address = (struct slwtmr_type *)DT_INST_REG_ADDR(inst),                      \
		.prescaler = DT_INST_PROP(inst, prescaler),                                        \
		DEV_CONFIG_CLK_DEV_INIT(inst)};                                                    \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, counter_rts5912_init, NULL, &counter_rts5912_dev_data_##inst,  \
			      &counter_rts5912_dev_config_##inst, POST_KERNEL,                     \
			      CONFIG_COUNTER_INIT_PRIORITY, &counter_rts5912_api);                 \
                                                                                                   \
	static void counter_rts5912_irq_config_##inst(void)                                        \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority), counter_rts5912_isr,  \
			    DEVICE_DT_INST_GET(inst), 0);                                          \
		irq_enable(DT_INST_IRQN(inst));                                                    \
	}

DT_INST_FOREACH_STATUS_OKAY(COUNTER_RTS5912_INIT)
