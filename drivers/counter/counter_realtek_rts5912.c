/*
 * Copyright (c) 2025 Realtek Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT realtek_rts5912_timer

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
#include "reg/reg_timer.h"
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(counter_realtek_rts5912, CONFIG_COUNTER_LOG_LEVEL);
struct counter_rts5912_config {
	struct counter_config_info info;
	void (*config_func)(void);
	volatile struct timer32_type *base_address;
	uint16_t prescaler;
	uint32_t clk_grp;
	uint32_t clk_idx;
	const struct device *clk_dev;
};

struct counter_rts5912_data {
	counter_alarm_callback_t alarm_cb;
	counter_top_callback_t top_cb;
	void *user_data;
};

#define COUNTER_RTS5912_REG_BASE(_dev)                                                             \
	((const struct counter_rts5912_config *const)_dev->config)->base_address

static int counter_rts5912_start(const struct device *dev)
{
	const struct counter_rts5912_config *config = dev->config;
	volatile struct timer32_type *counter = config->base_address;

	if (counter->ctrl & TIMER32_CTRL_EN) {
		return 0;
	}

	counter->ctrl |= (TIMER32_CTRL_EN);

	LOG_DBG("%p Counter started", dev);

	return 0;
}

static int counter_rts5912_stop(const struct device *dev)
{
	const struct counter_rts5912_config *config = dev->config;
	volatile struct timer32_type *counter = config->base_address;

	if (!(counter->ctrl & TIMER32_CTRL_EN)) {
		/* Already stopped, nothing to do */
		return 0;
	}
	/* disable timer and disable interrupt. */
	counter->ctrl = TIMER32_CTRL_INTEN_DIS;
	counter->cnt = counter->ldcnt;
	/* w1c interrupt pending status */
	counter->intclr |= TIMER32_INTCLR_INTCLR;

	LOG_DBG("%p Counter stopped", dev);

	return 0;
}

static int counter_rts5912_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct counter_rts5912_config *config = dev->config;
	volatile struct timer32_type *counter = config->base_address;

	*ticks = counter->cnt + 1;
	return 0;
}

static int counter_rts5912_set_alarm(const struct device *dev, uint8_t chan_id,
				     const struct counter_alarm_cfg *alarm_cfg)
{
	struct counter_rts5912_data *data = dev->data;
	const struct counter_rts5912_config *counter_cfg = dev->config;
	volatile struct timer32_type *counter = counter_cfg->base_address;
	uint32_t value;

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

	if (alarm_cfg->ticks > counter_cfg->info.max_top_value) {
		return -EINVAL;
	}
	/* disable timer */
	counter->ctrl &= ~TIMER32_CTRL_EN;
	/* disable interrupt */
	counter->ctrl |= TIMER32_CTRL_INTEN_DIS;
	/* set in one-shot mode */
	counter->ctrl &= ~TIMER32_CTRL_MDSELS_PERIOD;
	/* set load counter */
	counter->ldcnt = alarm_cfg->ticks;

	data->alarm_cb = alarm_cfg->callback;
	data->user_data = alarm_cfg->user_data;
	/* w1c interrupt status */
	counter->intclr |= TIMER32_INTCLR_INTCLR;
	/* enable interrupt */
	counter->ctrl &= ~TIMER32_CTRL_INTEN_DIS;

	LOG_DBG("%p Counter alarm set to %u ticks", dev, alarm_cfg->ticks);
	/* enable timer and re-load PRCNT to CNT */
	counter->ctrl |= TIMER32_CTRL_EN;
	/* read count value to update register */
	value = counter->cnt;

	return 0;
}

static int counter_rts5912_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	struct counter_rts5912_data *data = dev->data;
	const struct counter_rts5912_config *config = dev->config;
	volatile struct timer32_type *counter = config->base_address;

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
	volatile struct timer32_type *counter = config->base_address;

	return counter->intsts;
}

static uint32_t counter_rts5912_get_top_value(const struct device *dev)
{
	const struct counter_rts5912_config *config = dev->config;
	volatile struct timer32_type *counter = config->base_address;

	return counter->ldcnt;
}

static int counter_rts5912_set_top_value(const struct device *dev,
					 const struct counter_top_cfg *cfg)
{
	const struct counter_rts5912_config *counter_cfg = dev->config;
	struct counter_rts5912_data *data = dev->data;
	volatile struct timer32_type *counter = counter_cfg->base_address;
	uint32_t value;
	int ret = 0;

	if (data->alarm_cb) {
		return -EBUSY;
	}

	if (cfg->ticks > counter_cfg->info.max_top_value) {
		return -EINVAL;
	}

	counter->ctrl &= ~TIMER32_CTRL_EN;
	counter->ctrl |= TIMER32_CTRL_INTEN_DIS;

	counter->ldcnt = cfg->ticks;

	data->top_cb = cfg->callback;
	data->user_data = cfg->user_data;

	if (data->top_cb) {
		/* w1c interrupt status */
		counter->intclr |= TIMER32_INTCLR_INTCLR;
		/* enable interrupt */
		counter->ctrl &= ~TIMER32_CTRL_INTEN_DIS;
		/* enable periodic alarm mode */
		counter->ctrl |= TIMER32_CTRL_MDSELS_PERIOD;
	} else {
		counter->ctrl = TIMER32_CTRL_INTEN_DIS;
	}

	LOG_DBG("%p Counter top value was set to %u", dev, cfg->ticks);

	counter->ctrl |= TIMER32_CTRL_EN;
	/* read count value to update register */
	value = counter->cnt;

	return ret;
}

static void counter_rts5912_isr(const struct device *dev)
{
	struct counter_rts5912_data *data = dev->data;
	const struct counter_rts5912_config *config = dev->config;
	volatile struct timer32_type *counter = config->base_address;
	counter_alarm_callback_t alarm_cb;
	void *user_data;
	uint32_t value;

	/* disable timer */
	counter->ctrl &= ~TIMER32_CTRL_EN;
	/* disable interrupt */
	counter->ctrl |= TIMER32_CTRL_INTEN_DIS;
	/* clear interrupt pending status */
	counter->intclr |= TIMER32_INTCLR_INTCLR;

	LOG_DBG("%p Counter ISR", dev);

	if (data->alarm_cb) {
		/* Alarm is one-shot, so disable callback */
		alarm_cb = data->alarm_cb;
		data->alarm_cb = NULL;
		user_data = data->user_data;

		alarm_cb(dev, 0, counter->cnt + 1, user_data);
	} else if (data->top_cb) {
		data->top_cb(dev, data->user_data);
		/* periodic alarm mode, enable interrupt */
		counter->ctrl &= ~TIMER32_CTRL_INTEN_DIS;
		/* enable timer again */
		counter->ctrl |= TIMER32_CTRL_EN;
		/* read count value to update register */
		value = counter->cnt;
	}
}

static uint32_t counter_rts5912_get_freq(const struct device *dev)
{
	const struct counter_rts5912_config *counter_cfg = dev->config;

	return counter_cfg->info.freq;
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
	const struct counter_rts5912_config *counter_cfg = dev->config;
	volatile struct timer32_type *counter = counter_cfg->base_address;
	int rc;
	struct rts5912_sccon_subsys sccon_subsys = {
		.clk_grp = counter_cfg->clk_grp,
		.clk_idx = counter_cfg->clk_idx,
	};

	if (!device_is_ready(counter_cfg->clk_dev)) {
		LOG_ERR("device is not ready");
		return -ENODEV;
	}

	rc = clock_control_on(counter_cfg->clk_dev, (clock_control_subsys_t)&sccon_subsys);
	if (rc != 0) {
		LOG_ERR("clock power on fail");
		return rc;
	}

	counter_rts5912_stop(dev);

	/* Set preload and actually pre-load the counter */
	counter->ldcnt = counter_cfg->info.max_top_value;
	counter->cnt = counter_cfg->info.max_top_value;

	counter_cfg->config_func();
	LOG_DBG("Init complete!");
	return 0;
}

#define DEV_CONFIG_CLK_DEV_INIT(n)                                                                 \
	.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                          \
	.clk_grp = DT_INST_CLOCKS_CELL_BY_NAME(n, tmr32, clk_grp),                                 \
	.clk_idx = DT_INST_CLOCKS_CELL_BY_NAME(n, tmr32, clk_idx),

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
		.base_address = (struct timer32_type *)DT_INST_REG_ADDR(inst),                     \
		.prescaler = DT_INST_PROP(inst, prescaler),                                        \
		DEV_CONFIG_CLK_DEV_INIT(inst)};                                                    \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, counter_rts5912_init, NULL, &counter_rts5912_dev_data_##inst,  \
			      &counter_rts5912_dev_config_##inst, PRE_KERNEL_1,                    \
			      CONFIG_COUNTER_INIT_PRIORITY, &counter_rts5912_api);                 \
                                                                                                   \
	static void counter_rts5912_irq_config_##inst(void)                                        \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority), counter_rts5912_isr,  \
			    DEVICE_DT_INST_GET(inst), 0);                                          \
		irq_enable(DT_INST_IRQN(inst));                                                    \
	}

DT_INST_FOREACH_STATUS_OKAY(COUNTER_RTS5912_INIT)
