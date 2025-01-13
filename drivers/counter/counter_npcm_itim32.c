/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_npcm_itim32

#include <zephyr/drivers/counter.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/npcm_clock.h>
#include <zephyr/sys_clock.h>
#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <soc.h>

LOG_MODULE_REGISTER(npcm_itim32, CONFIG_COUNTER_LOG_LEVEL);

#define NUM_CHANNELS		1
#define MAX_PRESCALER		256
#define NPCM_ITIM_LFCLK		32768

struct counter_npcm_itim32_data {
	uint32_t freq;
	uint32_t setup_cycles;
	counter_top_callback_t top_callback;
	void *top_user_data;
	counter_alarm_callback_t alarm_callback;
	void *alarm_user_data;
};

struct counter_npcm_itim32_config {
	struct counter_config_info info;
	/* register base */
	uintptr_t base;
	/* Clock configuration */
	uint32_t clk_cfg;
	/* prescaler that use to divide input source frequency */
	uint8_t prescaler;
	void (*irq_config_func)(const struct device *dev);
};

/* Driver convenience defines */
#define HAL_INSTANCE(dev) \
	((struct itim32_reg *)((const struct counter_npcm_itim32_config *)(dev)->config)->base)

static int counter_npcm_itim32_stop(const struct device *dev)
{
	struct itim32_reg *const inst = HAL_INSTANCE(dev);

	/* Disable itim32 timer */
	inst->ITCTS &= ~BIT(NPCM_ITCTS_ITEN);
	/* Wait until itim32 timer disable */
	while (IS_BIT_SET(inst->ITCTS, NPCM_ITCTS_ITEN));

	return 0;
}

static int counter_npcm_itim32_start(const struct device *dev)
{
	struct itim32_reg *const inst = HAL_INSTANCE(dev);
	struct counter_npcm_itim32_data *data = dev->data;

	if (IS_BIT_SET(inst->ITCTS, NPCM_ITCTS_TO_STS)) {
		inst->ITCTS |= BIT(NPCM_ITCTS_TO_STS);
	}

	/* Disable itim32 timer */
	if (IS_BIT_SET(inst->ITCTS, NPCM_ITCTS_ITEN)) {
		counter_npcm_itim32_stop(dev);
	}

	/* Config itim32 timer cycle */
	inst->ITCNT32 = data->setup_cycles;

	/* Enable itim32 timer/counter */
	inst->ITCTS |= BIT(NPCM_ITCTS_ITEN);

	/* Wait until itim32 timer enable */
	while (!IS_BIT_SET(inst->ITCTS, NPCM_ITCTS_ITEN));

	return 0;
}

static int counter_npcm_itim32_get_value(const struct device *dev, uint32_t *ticks)
{
	struct itim32_reg *const inst = HAL_INSTANCE(dev);
	struct counter_npcm_itim32_data *data = dev->data;
	uint32_t overflow;
	uint32_t count;

	overflow = 0;

	if (IS_BIT_SET(inst->ITCTS, NPCM_ITCTS_TO_STS)) {
		overflow = data->setup_cycles;
	}

	count = inst->ITCNT32;

	*ticks = ((data->setup_cycles - inst->ITCNT32) + overflow);

	return 0;
}

static uint32_t counter_npcm_itim32_get_top_value(const struct device *dev)
{
	const struct counter_npcm_itim32_config *config = dev->config;

	return config->info.max_top_value;
}

static int counter_npcm_itim32_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	ARG_UNUSED(chan_id);
	struct itim32_reg *const inst = HAL_INSTANCE(dev);
	struct counter_npcm_itim32_data *data = dev->data;

	counter_npcm_itim32_stop(dev);

	data->alarm_callback = NULL;
	data->alarm_user_data = NULL;
	/* Restore default cycles */
	data->setup_cycles = data->freq;

	/* Set default timeout cycles */
	inst->ITCNT32 = data->setup_cycles;

	return 0;
}

static int counter_npcm_itim32_set_alarm(const struct device *dev, uint8_t chan_id,
				   const struct counter_alarm_cfg *alarm_cfg)
{
	ARG_UNUSED(chan_id);
	struct counter_npcm_itim32_data *data = dev->data;
	uint32_t max_top_value = counter_npcm_itim32_get_top_value(dev);

	if (alarm_cfg->ticks > max_top_value) {
		LOG_ERR("alarm ticks(%d) exceed top value(%d)",
					alarm_cfg->ticks, max_top_value);
		return -EINVAL;
	}

	/* Cancel itim32 timer */
	counter_npcm_itim32_cancel_alarm(dev, chan_id);

	/* Setup callback function and data */
	data->alarm_callback = alarm_cfg->callback;
	data->alarm_user_data = alarm_cfg->user_data;
	data->setup_cycles = alarm_cfg->ticks;

	counter_npcm_itim32_start(dev);

	return 0;
}

static int counter_npcm_itim32_set_top_value(const struct device *dev,
					 const struct counter_top_cfg *cfg)
{
	struct counter_npcm_itim32_data *data = dev->data;
	uint32_t max_top_value = counter_npcm_itim32_get_top_value(dev);

	if (cfg->ticks > max_top_value) {
		LOG_ERR("top ticks(%d) exceed top value(%d)",
					cfg->ticks, max_top_value);
                return -EINVAL;
        }

	/* Cancel itim32 timer */
	counter_npcm_itim32_stop(dev);

	/* Setup callback function and data */
	data->top_callback = cfg->callback;
        data->top_user_data = cfg->user_data;
	data->setup_cycles = cfg->ticks;

	counter_npcm_itim32_start(dev);

	return 0;
}

static uint32_t counter_npcm_itim32_get_pending_int(const struct device *dev)
{
	struct itim32_reg *const inst = HAL_INSTANCE(dev);

	if (IS_BIT_SET(inst->ITCTS, NPCM_ITCTS_TO_STS)) {
		return 1;
	} else {
		return 0;
	}
}

static uint32_t counter_npcm_itim32_get_freq(const struct device *dev)
{
	struct counter_npcm_itim32_data *data = dev->data;

	return data->freq;
}

static void counter_npcm_itim32_isr(const struct device *dev)
{
	struct itim32_reg *const inst = HAL_INSTANCE(dev);
	struct counter_npcm_itim32_data *data = dev->data;

	if (data->alarm_callback != NULL) {
		counter_alarm_callback_t alarm_callback =
			data->alarm_callback;
		void *alarm_user_data = data->alarm_user_data;
		uint32_t ticks;

		counter_npcm_itim32_get_value(dev, &ticks);

		data->alarm_callback = NULL;
		data->alarm_user_data = NULL;

		/* Disable itim32 timer */
		counter_npcm_itim32_stop(dev);

		alarm_callback(dev, 0x0, ticks, alarm_user_data);
	}

	if (data->top_callback != NULL) {
		data->top_callback(dev, data->top_user_data);
	}

	/* Clear event timeout */
	if (IS_BIT_SET(inst->ITCTS, NPCM_ITCTS_TO_STS)) {
		inst->ITCTS |= BIT(NPCM_ITCTS_TO_STS);
	}
}

static int counter_npcm_itim32_init(const struct device *dev)
{
	const struct counter_npcm_itim32_config *config = dev->config;
	struct counter_npcm_itim32_data *data = dev->data;
	const struct device *const clk_dev = DEVICE_DT_GET(DT_NODELABEL(pcc));
	struct itim32_reg *const inst = HAL_INSTANCE(dev);
	uint8_t itcts;
	int ret;

	if (!device_is_ready(clk_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	/* Turn on device clock first and get source clock freq. */
	ret = clock_control_on(clk_dev,
				(clock_control_subsys_t)config->clk_cfg);
	if (ret < 0) {
		LOG_ERR("Turn on FIU clock fail %d", ret);
		return ret;
	}

	ret = clock_control_get_rate(clk_dev, (clock_control_subsys_t)config->clk_cfg,
				     &data->freq);
	if (ret < 0) {
		LOG_ERR("Get ITIM32 clock source rate error %d", ret);
		return ret;
	}

	/* Disable itim32 timer */
	if (IS_BIT_SET(inst->ITCTS, NPCM_ITCTS_ITEN)) {
		counter_npcm_itim32_stop(dev);
	}

	/* Config prescaler */
	if (config->prescaler <= MAX_PRESCALER) {
		inst->ITPRE = (config->prescaler - 1);
	} else {
		return -EINVAL;
	}

	/* Enable wakeup event and interrupt, clear status timeout event */
	itcts = BIT(NPCM_ITCTS_TO_WUE) | BIT(NPCM_ITCTS_TO_IE) |
		BIT(NPCM_ITCTS_TO_STS);

	/* Select low-frequency input clock source and change src freq to LF */
	if (data->freq == NPCM_ITIM_LFCLK) {
		itcts |= BIT(NPCM_ITCTS_CKSEL);
	} else {
		itcts &= ~BIT(NPCM_ITCTS_CKSEL);
	}

	/* Store actual frequency value */
	data->freq = data->freq / config->prescaler;

	data->top_callback = NULL;
	data->top_user_data = NULL;
	data->alarm_callback = NULL;
	data->alarm_user_data = NULL;
	data->setup_cycles = data->freq;

	/* Set default timeout cycles */
	inst->ITCNT32 = data->setup_cycles;

	/* Config itim control and status */
	inst->ITCTS = itcts;

	config->irq_config_func(dev);

	return 0;
}

static const struct counter_driver_api counter_npcm_itim32_driver_api = {
	.start = counter_npcm_itim32_start,
	.stop = counter_npcm_itim32_stop,
	.get_value = counter_npcm_itim32_get_value,
	.set_alarm = counter_npcm_itim32_set_alarm,
	.cancel_alarm = counter_npcm_itim32_cancel_alarm,
	.set_top_value = counter_npcm_itim32_set_top_value,
	.get_pending_int = counter_npcm_itim32_get_pending_int,
	.get_top_value = counter_npcm_itim32_get_top_value,
	.get_freq = counter_npcm_itim32_get_freq,
};

#define COUNTER_NPCM_ITIM32(id)                                                                     \
	static void counter_npcm_itim32_irq_config_##id(const struct device *dev);                  \
	static struct counter_npcm_itim32_config counter_npcm_itim32_config_##id = {                \
		.info = {                                                                           \
			.max_top_value = UINT32_MAX,                                                \
			.channels = NUM_CHANNELS,                                                   \
		},                                                                                  \
		.base = DT_INST_REG_ADDR(id),                                                       \
		.clk_cfg = DT_INST_PHA(id, clocks, clk_cfg),                                        \
		.prescaler = DT_INST_PROP(id, prescaler),				            \
		.irq_config_func = counter_npcm_itim32_irq_config_##id,	                            \
	};                                                                                          \
	static struct counter_npcm_itim32_data counter_npcm_itim32_data_##id;                       \
	DEVICE_DT_INST_DEFINE(id, &counter_npcm_itim32_init, NULL, &counter_npcm_itim32_data_##id,  \
			      &counter_npcm_itim32_config_##id, POST_KERNEL,                        \
			      CONFIG_COUNTER_INIT_PRIORITY, &counter_npcm_itim32_driver_api);       \
	static void counter_npcm_itim32_irq_config_##id(const struct device *dev)                   \
	{                                                                                           \
		IRQ_CONNECT(DT_INST_IRQN(id), DT_INST_IRQ(id, priority), counter_npcm_itim32_isr,   \
			    DEVICE_DT_INST_GET(id), 0);                                             \
		irq_enable(DT_INST_IRQN(id));                                                       \
	}

DT_INST_FOREACH_STATUS_OKAY(COUNTER_NPCM_ITIM32)
