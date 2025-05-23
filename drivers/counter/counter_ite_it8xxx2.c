/*
 * Copyright (c) 2025 ITE Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it8xxx2_counter

#include <soc.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(counter_it8xxx2, CONFIG_COUNTER_LOG_LEVEL);

/* IT8XXX2 Timer registers offsets */
#define ET7CTRL   0x00
#define ET7PSR    0x01
#define ET7CNTLLR 0x04
#define ET8CTRL   0x08
#define ET8PSR    0x09
#define ET8CNTLLR 0x0C
#define ET7CNTOLR 0x28
#define ET8CNTOLR 0x2C

/* ETnCTLR bit definitions (for n = 7 ~ 8) */
#define ET_COMB BIT(3) /* only defined in ET7CTRL */
#define ET_TC   BIT(2)
#define ET_RST  BIT(1)
#define ET_EN   BIT(0)

/* ETnPSR bit definitions (for n = 7 ~ 8) */
#define ETnPSR_32768HZ 0x00

struct counter_it8xxx2_config {
	struct counter_config_info info;
	mm_reg_t base;
	/* alarm timer irq */
	int alarm_irq;
	/* top timer irq */
	int top_irq;
	void (*irq_config_func)(const struct device *dev);
};

struct counter_it8xxx2_data {
	counter_top_callback_t top_callback;
	void *top_user_data;
	counter_alarm_callback_t alarm_callback;
	void *alarm_user_data;
};

static inline uint32_t counter_it8xxx2_read8(const struct device *dev, mm_reg_t offset)
{
	const struct counter_it8xxx2_config *config = dev->config;

	return sys_read8(config->base + offset);
}

static inline uint32_t counter_it8xxx2_read32(const struct device *dev, mm_reg_t offset)
{
	const struct counter_it8xxx2_config *config = dev->config;

	return sys_read32(config->base + offset);
}

static inline void counter_it8xxx2_write8(const struct device *dev, uint32_t value, mm_reg_t offset)
{
	const struct counter_it8xxx2_config *config = dev->config;

	sys_write8(value, config->base + offset);
}

static inline void counter_it8xxx2_write32(const struct device *dev, uint32_t value,
					   mm_reg_t offset)
{
	const struct counter_it8xxx2_config *config = dev->config;

	sys_write32(value, config->base + offset);
}

static inline void counter_it8xxx2_alarm_timer_disable(const struct device *dev)
{
	const struct counter_it8xxx2_config *config = dev->config;

	irq_disable(config->alarm_irq);
	counter_it8xxx2_write8(dev, counter_it8xxx2_read8(dev, ET7CTRL) & ~ET_EN, ET7CTRL);
	ite_intc_isr_clear(config->alarm_irq);
}

static int counter_it8xxx2_start(const struct device *dev)
{
	LOG_DBG("starting top timer");

	counter_it8xxx2_write8(dev, ET_EN | ET_RST, ET8CTRL);

	return 0;
}

static int counter_it8xxx2_stop(const struct device *dev)
{
	LOG_DBG("stopping timer");

	counter_it8xxx2_write8(dev, counter_it8xxx2_read8(dev, ET8CTRL) & ~ET_EN, ET8CTRL);

	return 0;
}

static int counter_it8xxx2_get_value(const struct device *dev, uint32_t *ticks)
{
	*ticks = counter_it8xxx2_read32(dev, ET8CNTOLR);

	return 0;
}

static int counter_it8xxx2_set_alarm(const struct device *dev, uint8_t chan_id,
				     const struct counter_alarm_cfg *alarm_cfg)
{
	const struct counter_it8xxx2_config *config = dev->config;
	struct counter_it8xxx2_data *data = dev->data;

	ARG_UNUSED(chan_id);

	/* Interrupts are only triggered when the counter reaches 0.
	 * So only relative alarms are supported.
	 */
	if (alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) {
		return -ENOTSUP;
	}

	if (data->alarm_callback != NULL) {
		return -EBUSY;
	}

	if (alarm_cfg->callback == NULL) {
		return -EINVAL;
	}

	if (alarm_cfg->ticks > counter_it8xxx2_read32(dev, ET8CNTLLR)) {
		return -EINVAL;
	}

	LOG_DBG("triggering alarm in 0x%08x ticks", alarm_cfg->ticks);

	irq_disable(config->alarm_irq);

	counter_it8xxx2_write32(dev, alarm_cfg->ticks, ET7CNTLLR);

	data->alarm_callback = alarm_cfg->callback;
	data->alarm_user_data = alarm_cfg->user_data;

	LOG_DBG("%p Counter alarm set to %u ticks", dev, alarm_cfg->ticks);

	counter_it8xxx2_write8(dev, counter_it8xxx2_read8(dev, ET7CTRL) | ET_EN | ET_RST, ET7CTRL);

	ite_intc_isr_clear(config->alarm_irq);

	irq_enable(config->alarm_irq);

	return 0;
}

static int counter_it8xxx2_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	struct counter_it8xxx2_data *data = dev->data;

	if (chan_id != 0) {
		LOG_ERR("Invalid channel id %u", chan_id);
		return -ENOTSUP;
	}

	counter_it8xxx2_alarm_timer_disable(dev);

	data->alarm_callback = NULL;
	data->alarm_user_data = NULL;

	LOG_DBG("%p Counter alarm canceled", dev);

	return 0;
}

static int counter_it8xxx2_set_top_value(const struct device *dev,
					 const struct counter_top_cfg *top_cfg)
{
	const struct counter_it8xxx2_config *config = dev->config;
	struct counter_it8xxx2_data *data = dev->data;

	if (top_cfg == NULL) {
		LOG_ERR("Invalid top value configuration");
		return -EINVAL;
	}

	if (top_cfg->ticks == 0) {
		return -EINVAL;
	}

	if (top_cfg->ticks > config->info.max_top_value) {
		return -ENOTSUP;
	}

	if (data->alarm_callback) {
		return -EBUSY;
	}

	/* top value cannot be updated without reset */
	if (top_cfg->flags & COUNTER_TOP_CFG_DONT_RESET) {
		LOG_ERR("Updating top value without reset is not supported");
		return -ENOTSUP;
	}

	LOG_DBG("setting top value to 0x%08x", top_cfg->ticks);

	data->top_callback = top_cfg->callback;
	data->top_user_data = top_cfg->user_data;

	irq_disable(config->top_irq);

	/* set new top value */
	counter_it8xxx2_write32(dev, top_cfg->ticks, ET8CNTLLR);

	/* re-enable and reset timer */
	counter_it8xxx2_write8(dev, counter_it8xxx2_read8(dev, ET8CTRL) | ET_EN | ET_RST, ET8CTRL);

	ite_intc_isr_clear(config->top_irq);

	irq_enable(config->top_irq);

	return 0;
}

static uint32_t counter_it8xxx2_get_top_value(const struct device *dev)
{
	return counter_it8xxx2_read32(dev, ET8CNTLLR);
}

static void counter_it8xxx2_alarm_isr(const struct device *dev)
{
	struct counter_it8xxx2_data *data = dev->data;
	counter_alarm_callback_t alarm_cb;
	void *user_data;
	uint32_t ticks;

	LOG_DBG("%p alarm timer ISR", dev);

	/* Alarm is one-shot, so disable interrupt and callback */
	if (data->alarm_callback) {
		alarm_cb = data->alarm_callback;
		data->alarm_callback = NULL;
		user_data = data->alarm_user_data;

		ticks = counter_it8xxx2_read32(dev, ET8CNTOLR);

		alarm_cb(dev, 0, ticks, user_data);
	}

	counter_it8xxx2_alarm_timer_disable(dev);
}

static void counter_it8xxx2_top_isr(const struct device *dev)
{
	const struct counter_it8xxx2_config *config = dev->config;
	struct counter_it8xxx2_data *data = dev->data;

	LOG_DBG("%p top timer ISR", dev);

	if (data->top_callback) {
		data->top_callback(dev, data->top_user_data);
	}

	/* read clear timer 8 terminal count */
	counter_it8xxx2_read8(dev, ET8CTRL);

	ite_intc_isr_clear(config->top_irq);
}

static int counter_it8xxx2_init(const struct device *dev)
{
	const struct counter_it8xxx2_config *config = dev->config;

	LOG_DBG("max top value = 0x%08x", config->info.max_top_value);
	LOG_DBG("frequency = %d", config->info.freq);
	LOG_DBG("channels = %d", config->info.channels);

	/* set the top value of top timer  */
	counter_it8xxx2_write32(dev, config->info.max_top_value, ET8CNTLLR);

	/* set the frequencies of alarm timer and top timer */
	counter_it8xxx2_write8(dev, ETnPSR_32768HZ, ET7PSR);
	counter_it8xxx2_write8(dev, ETnPSR_32768HZ, ET8PSR);

	config->irq_config_func(dev);

	return 0;
}

static DEVICE_API(counter, counter_it8xxx2_driver_api) = {
	.start = counter_it8xxx2_start,
	.stop = counter_it8xxx2_stop,
	.get_value = counter_it8xxx2_get_value,
	.set_alarm = counter_it8xxx2_set_alarm,
	.cancel_alarm = counter_it8xxx2_cancel_alarm,
	.set_top_value = counter_it8xxx2_set_top_value,
	.get_top_value = counter_it8xxx2_get_top_value,
};

#define COUNTER_IT8XXX2_INIT(n)                                                                    \
	static void counter_it8xxx2_cfg_func_##n(const struct device *dev)                         \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN_BY_IDX(n, 0), 0, counter_it8xxx2_alarm_isr,               \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		IRQ_CONNECT(DT_INST_IRQN_BY_IDX(n, 1), 0, counter_it8xxx2_top_isr,                 \
			    DEVICE_DT_INST_GET(n), 0);                                             \
	}                                                                                          \
                                                                                                   \
	static struct counter_it8xxx2_config counter_it8xxx2_config_##n = {                        \
		.info =                                                                            \
			{                                                                          \
				.max_top_value = UINT32_MAX,                                       \
				.freq = 32768,                                                     \
				.flags = 0,                                                        \
				.channels = 1,                                                     \
			},                                                                         \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.alarm_irq = DT_INST_IRQN_BY_IDX(n, 0),                                            \
		.top_irq = DT_INST_IRQN_BY_IDX(n, 1),                                              \
		.irq_config_func = counter_it8xxx2_cfg_func_##n,                                   \
	};                                                                                         \
                                                                                                   \
	static struct counter_it8xxx2_data counter_it8xxx2_data_##n;                               \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &counter_it8xxx2_init, NULL, &counter_it8xxx2_data_##n,           \
			      &counter_it8xxx2_config_##n, POST_KERNEL,                            \
			      CONFIG_COUNTER_INIT_PRIORITY, &counter_it8xxx2_driver_api);

DT_INST_FOREACH_STATUS_OKAY(COUNTER_IT8XXX2_INIT)
BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "only one ite,it8xxx2-counter compatible node can be supported");
