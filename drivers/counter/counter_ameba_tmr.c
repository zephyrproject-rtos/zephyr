/*
 * Copyright (c) 2024 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT realtek_ameba_counter

/* Include <soc.h> before <ameba_soc.h> to avoid redefining unlikely() macro */
#include <soc.h>
#include <ameba_soc.h>

#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/irq.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ameba_counter, CONFIG_COUNTER_LOG_LEVEL);

static void counter_ameba_isr(const struct device *dev);

struct counter_ameba_config {
	struct counter_config_info counter_info;
	RTIM_TypeDef *basic_timer;
	int irq_source;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	uint32_t clock_frequency;
	void (*irq_config_func)(const struct device *dev);
};

struct counter_ameba_data {
	struct counter_alarm_cfg alarm_cfg;
	struct counter_top_cfg top_cfg;
};

static int counter_ameba_init(const struct device *dev)
{
	const struct counter_ameba_config *cfg = dev->config;
	RTIM_TimeBaseInitTypeDef TIM_InitStruct;

	if (!cfg->clock_dev) {
		return -EINVAL;
	}

	if (!device_is_ready(cfg->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	if (clock_control_on(cfg->clock_dev, cfg->clock_subsys)) {
		LOG_ERR("Could not enable counter clock");
		return -EIO;
	}

	cfg->irq_config_func(dev);

	RTIM_TimeBaseStructInit(&TIM_InitStruct);
	RTIM_TimeBaseInit(cfg->basic_timer, &TIM_InitStruct, cfg->irq_source,
			  (IRQ_FUN)counter_ameba_isr, (uint32_t)&TIM_InitStruct);

	return 0;
}

static int counter_ameba_start(const struct device *dev)
{
	const struct counter_ameba_config *cfg = dev->config;

	RTIM_Cmd(cfg->basic_timer, ENABLE);
	return 0;
}

static int counter_ameba_stop(const struct device *dev)
{
	const struct counter_ameba_config *cfg = dev->config;

	RTIM_Cmd(cfg->basic_timer, DISABLE);
	return 0;
}

static int counter_ameba_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct counter_ameba_config *cfg = dev->config;

	*ticks = RTIM_GetCount(cfg->basic_timer);
	return 0;
}

static uint32_t counter_ameba_get_top_value(const struct device *dev)
{
	const struct counter_ameba_config *cfg = dev->config;

	return cfg->counter_info.max_top_value;
}

static int counter_ameba_set_alarm(const struct device *dev, uint8_t chan_id,
				   const struct counter_alarm_cfg *alarm_cfg)
{
	ARG_UNUSED(chan_id);
	struct counter_ameba_data *data = dev->data;
	const struct counter_ameba_config *cfg = dev->config;

	if (alarm_cfg->ticks > counter_ameba_get_top_value(dev)) {
		return -EINVAL;
	}

	if (data->alarm_cfg.callback) {
		return -EBUSY;
	}

	data->alarm_cfg.callback = alarm_cfg->callback;
	data->alarm_cfg.user_data = alarm_cfg->user_data;
	RTIM_ChangePeriodImmediate(cfg->basic_timer, alarm_cfg->ticks);
	RTIM_INTConfig(cfg->basic_timer, TIM_IT_Update, ENABLE);

	return 0;
}

static int counter_ameba_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	ARG_UNUSED(chan_id);
	struct counter_ameba_data *data = dev->data;
	const struct counter_ameba_config *cfg = dev->config;

	data->alarm_cfg.callback = NULL;

	RTIM_INTConfig(cfg->basic_timer, TIM_IT_Update, DISABLE);

	return 0;
}

static int counter_ameba_set_top_value(const struct device *dev, const struct counter_top_cfg *cfg)
{
	const struct counter_ameba_config *config = dev->config;
	struct counter_ameba_data *data = dev->data;

	data->top_cfg.user_data = cfg->user_data;
	data->top_cfg.ticks = cfg->ticks;
	data->top_cfg.callback = cfg->callback;
	RTIM_ChangePeriodImmediate(config->basic_timer, cfg->ticks);
	RTIM_INTConfig(config->basic_timer, TIM_IT_Update, ENABLE);
	if (data->top_cfg.ticks != config->counter_info.max_top_value) {
		return -ENOTSUP;
	} else {
		return 0;
	}
}

static uint32_t counter_ameba_get_pending_int(const struct device *dev)
{
	const struct counter_ameba_config *cfg = dev->config;

	return RTIM_GetINTStatus(cfg->basic_timer, TIM_IT_Update);
}

static uint32_t counter_ameba_get_freq(const struct device *dev)
{
	const struct counter_ameba_config *cfg = dev->config;

	return cfg->clock_frequency;
}

static DEVICE_API(counter, counter_api) = {
	.start = counter_ameba_start,
	.stop = counter_ameba_stop,
	.get_value = counter_ameba_get_value,
	.set_alarm = counter_ameba_set_alarm,
	.cancel_alarm = counter_ameba_cancel_alarm,
	.set_top_value = counter_ameba_set_top_value,
	.get_pending_int = counter_ameba_get_pending_int,
	.get_top_value = counter_ameba_get_top_value,
	.get_freq = counter_ameba_get_freq,
};

static void counter_ameba_isr(const struct device *dev)
{
	struct counter_ameba_data *data = dev->data;
	const struct counter_ameba_config *cfg = dev->config;
	counter_alarm_callback_t cb;
	uint32_t ticks;

	counter_ameba_get_value(dev, &ticks);
	cb = data->alarm_cfg.callback;
	data->alarm_cfg.callback = NULL;
	if (cb) {
		cb(dev, 0, ticks, data->alarm_cfg.user_data);
	}
	RTIM_INTClear(cfg->basic_timer);
}

#define TIMER_IRQ_CONFIG(n)                                                                        \
	static void irq_config_##n(const struct device *dev)                                       \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), counter_ameba_isr,          \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}

#define AMEBA_COUNTER_INIT(n)                                                                      \
	TIMER_IRQ_CONFIG(n)                                                                        \
	static struct counter_ameba_data counter_data_##n;                                         \
                                                                                                   \
	static const struct counter_ameba_config counter_config_##n = {                            \
		.counter_info = {.max_top_value = UINT32_MAX,                                      \
				 .flags = COUNTER_CONFIG_INFO_COUNT_UP,                            \
				 .channels = 1},                                                   \
		.basic_timer = (RTIM_TypeDef *)DT_INST_REG_ADDR(n),                                \
		.irq_source = DT_INST_IRQN(n),                                                     \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, idx),               \
		.clock_frequency = DT_INST_PROP(n, clock_frequency),                               \
		.irq_config_func = irq_config_##n};                                                \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, counter_ameba_init, NULL, &counter_data_##n, &counter_config_##n, \
			      PRE_KERNEL_1, CONFIG_COUNTER_INIT_PRIORITY, &counter_api);

DT_INST_FOREACH_STATUS_OKAY(AMEBA_COUNTER_INIT);
