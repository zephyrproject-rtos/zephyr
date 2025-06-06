/*
 * Copyright (c) 2025 ITE Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it51xxx_counter

#include <soc.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(counter_it51xxx, CONFIG_COUNTER_LOG_LEVEL);

/* 0x30 (offset 0x00): External Timer 7 Control Register (used as alarm counter) */
#define REG_TIMER_ET7CTRL   0x00
#define TIMER_ETTC          BIT(2)
#define TIMER_RST_EN        (TIMER_ETRST | TIMER_ETEN)
#define TIMER_ETRST         BIT(1)
#define TIMER_ETEN          BIT(0)
/* 0x31 (offset 0x01): External Timer 7 Prescaler Register */
#define REG_TIMER_ET7PSR    0x01
#define TIMER_ETPSR_32768HZ 0x00
/* 0x34 (offset 0x04): External Timer 7 Counter Register */
#define REG_TIMER_ET7CNTLLR 0x04
/* 0x38 (offset 0x08): External Timer 8 Control Register (used as top counter) */
#define REG_TIMER_ET8CTRL   0x08
/* 0x39 (offset 0x09): External Timer 8 Prescaler Register */
#define REG_TIMER_ET8PSR    0x09
/* 0x3C (offset 0x0C): External Timer 8 Counter Register */
#define REG_TIMER_ET8CNTLLR 0x0C
/* 0x58 (offset 0x28): External Timer 7 Counter Observation Register */
#define REG_TIMER_ET7CNTOLR 0x28
/* 0x5C (offset 0x2C): External Timer 8 Counter Observation Register */
#define REG_TIMER_ET8CNTOLR 0x2C

struct counter_it51xxx_config {
	struct counter_config_info info;
	mm_reg_t base;
	/* Alarm timer irq number */
	int alarm_irq;
	/* Alarm timer irq trigger mode */
	int alarm_flag;
	/* Top timer irq number */
	int top_irq;
	/* Top timer irq trigger mode */
	int top_flag;
	void (*irq_config_func)(const struct device *dev);
};

struct counter_it51xxx_data {
	counter_top_callback_t top_callback;
	/* User data passed to top callback function */
	void *top_user_data;
	counter_alarm_callback_t alarm_callback;
	/* User data passed to alarm callback function */
	void *alarm_user_data;
};

static inline void counter_it51xxx_alarm_timer_disable(const struct device *dev)
{
	const struct counter_it51xxx_config *config = dev->config;

	irq_disable(config->alarm_irq);
	/* Read clear terminal count and disable alarm timer */
	sys_write8(sys_read8(config->base + REG_TIMER_ET7CTRL) & ~TIMER_ETEN,
		   config->base + REG_TIMER_ET7CTRL);
	ite_intc_isr_clear(config->alarm_irq);
}

static int counter_it51xxx_start(const struct device *dev)
{
	const struct counter_it51xxx_config *config = dev->config;

	LOG_DBG("Start top timer");

	sys_write8(TIMER_RST_EN, config->base + REG_TIMER_ET8CTRL);

	return 0;
}

static int counter_it51xxx_stop(const struct device *dev)
{
	const struct counter_it51xxx_config *config = dev->config;

	LOG_DBG("Stop top timer");

	sys_write8(sys_read8(config->base + REG_TIMER_ET8CTRL) & ~TIMER_ETEN,
		   config->base + REG_TIMER_ET8CTRL);

	return 0;
}

static int counter_it51xxx_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct counter_it51xxx_config *config = dev->config;

	/* Critical section */
	unsigned int key = irq_lock();

	/* Workaround for reading observation register latch issue */
	sys_read32(config->base + REG_TIMER_ET8CNTOLR);
	sys_read32(config->base + REG_TIMER_ET8PSR);
	*ticks = sys_read32(config->base + REG_TIMER_ET8CNTOLR);

	irq_unlock(key);

	return 0;
}

static int counter_it51xxx_set_alarm(const struct device *dev, uint8_t chan_id,
				     const struct counter_alarm_cfg *alarm_cfg)
{
	const struct counter_it51xxx_config *config = dev->config;
	struct counter_it51xxx_data *data = dev->data;

	ARG_UNUSED(chan_id);

	if (alarm_cfg->callback == NULL) {
		LOG_ERR("Alarm timer callback can't be NULL");
		return -EINVAL;
	}

	if (alarm_cfg->ticks > sys_read32(config->base + REG_TIMER_ET8CNTLLR)) {
		LOG_ERR("Alarm timer ticks can't be bigger than top ticks");
		return -EINVAL;
	}

	/* There is an active alarm timer, so alarm timer can't be updated. */
	if (data->alarm_callback != NULL) {
		return -EBUSY;
	}

	/*
	 * Interrupts are only triggered when ticks reaches 0,
	 * so only relative alarms are supported.
	 */
	if (alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) {
		LOG_ERR("COUNTER_ALARM_CFG_ABSOLUTE flag is not supported");
		return -ENOTSUP;
	}

	irq_disable(config->alarm_irq);

	/* Disable alarm timer */
	sys_write8(sys_read8(config->base + REG_TIMER_ET7CTRL) & ~TIMER_ETEN,
		   config->base + REG_TIMER_ET7CTRL);

	data->alarm_callback = alarm_cfg->callback;
	data->alarm_user_data = alarm_cfg->user_data;

	/* Set alarm timer ticks */
	sys_write32(alarm_cfg->ticks, config->base + REG_TIMER_ET7CNTLLR);
	LOG_DBG("Set alarm timer ticks 0x%x", alarm_cfg->ticks);

	ite_intc_isr_clear(config->alarm_irq);

	/* Read clear terminal count, enable, and reset alarm timer */
	sys_write8(sys_read8(config->base + REG_TIMER_ET7CTRL) | TIMER_RST_EN,
		   config->base + REG_TIMER_ET7CTRL);

	irq_enable(config->alarm_irq);

	return 0;
}

static int counter_it51xxx_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	struct counter_it51xxx_data *data = dev->data;

	if (chan_id != 0) {
		LOG_ERR("Invalid channel id %u", chan_id);
		return -ENOTSUP;
	}

	counter_it51xxx_alarm_timer_disable(dev);

	data->alarm_callback = NULL;
	data->alarm_user_data = NULL;

	LOG_DBG("Alarm timer is canceled");

	return 0;
}

static int counter_it51xxx_set_top_value(const struct device *dev,
					 const struct counter_top_cfg *top_cfg)
{
	const struct counter_it51xxx_config *config = dev->config;
	struct counter_it51xxx_data *data = dev->data;

	if (top_cfg == NULL) {
		LOG_ERR("Top timer configuration can't be NULL");
		return -EINVAL;
	}

	if (top_cfg->ticks == 0) {
		LOG_ERR("Top timer ticks can't be set to zero");
		return -EINVAL;
	}

	if (top_cfg->ticks > config->info.max_top_value) {
		LOG_ERR("Top timer ticks only support 32 bits");
		return -ENOTSUP;
	}

	/* There is an active alarm timer, so top timer can't be updated. */
	if (data->alarm_callback) {
		return -EBUSY;
	}

	/* Top timer ticks cannot be updated without reset */
	if (top_cfg->flags & COUNTER_TOP_CFG_DONT_RESET) {
		LOG_ERR("COUNTER_ALARM_CFG_ABSOLUTE flag is not supported");
		return -ENOTSUP;
	}

	irq_disable(config->top_irq);

	/* Disable top timer */
	sys_write8(sys_read8(config->base + REG_TIMER_ET8CTRL) & ~TIMER_ETEN,
		   config->base + REG_TIMER_ET8CTRL);

	data->top_callback = top_cfg->callback;
	data->top_user_data = top_cfg->user_data;

	/* Set top timer ticks */
	sys_write32(top_cfg->ticks, config->base + REG_TIMER_ET8CNTLLR);
	LOG_DBG("Set top timer ticks 0x%x", top_cfg->ticks);

	ite_intc_isr_clear(config->top_irq);

	/* Read clear terminal count, enable, and reset top timer */
	sys_write8(sys_read8(config->base + REG_TIMER_ET8CTRL) | TIMER_RST_EN,
		   config->base + REG_TIMER_ET8CTRL);

	irq_enable(config->top_irq);

	return 0;
}

static uint32_t counter_it51xxx_get_top_value(const struct device *dev)
{
	const struct counter_it51xxx_config *config = dev->config;

	return sys_read32(config->base + REG_TIMER_ET8CNTLLR);
}

static void counter_it51xxx_alarm_isr(const struct device *dev)
{
	struct counter_it51xxx_data *data = dev->data;
	counter_alarm_callback_t alarm_cb;
	void *user_data;
	uint32_t ticks;

	LOG_DBG("Alarm timer ISR");

	/* Alarm is one-shot, so disable interrupt and callback */
	if (data->alarm_callback) {
		alarm_cb = data->alarm_callback;
		data->alarm_callback = NULL;
		user_data = data->alarm_user_data;

		counter_it51xxx_get_value(dev, &ticks);

		alarm_cb(dev, 0, ticks, user_data);
	}

	counter_it51xxx_alarm_timer_disable(dev);
}

static void counter_it51xxx_top_isr(const struct device *dev)
{
	const struct counter_it51xxx_config *config = dev->config;
	struct counter_it51xxx_data *data = dev->data;

	LOG_DBG("Top timer ISR");

	if (data->top_callback) {
		data->top_callback(dev, data->top_user_data);
	}

	/* Read clear top timer terminal count */
	sys_read8(config->base + REG_TIMER_ET8CTRL);

	ite_intc_isr_clear(config->top_irq);
}

static int counter_it51xxx_init(const struct device *dev)
{
	const struct counter_it51xxx_config *config = dev->config;
	uint8_t et7_ctrl = sys_read8(config->base + REG_TIMER_ET7CTRL);
	uint8_t et8_ctrl = sys_read8(config->base + REG_TIMER_ET8CTRL);

	/* First time enable: enable and re-start timer -> disable timer */
	sys_write8(et7_ctrl | TIMER_RST_EN, config->base + REG_TIMER_ET7CTRL);
	sys_write8(et7_ctrl & ~TIMER_ETEN, config->base + REG_TIMER_ET7CTRL);
	sys_write8(et8_ctrl | TIMER_RST_EN, config->base + REG_TIMER_ET8CTRL);
	sys_write8(et8_ctrl & ~TIMER_ETEN, config->base + REG_TIMER_ET8CTRL);

	/* Set rising edge trigger of alarm timer and top timer */
	ite_intc_irq_polarity_set(config->alarm_irq, config->alarm_flag);
	ite_intc_irq_polarity_set(config->top_irq, config->top_flag);

	/* Clear interrupt status of alarm timer and top timer */
	ite_intc_isr_clear(config->alarm_irq);
	ite_intc_isr_clear(config->top_irq);

	/* Select clock source of alarm timer and top timer */
	sys_write8(TIMER_ETPSR_32768HZ, config->base + REG_TIMER_ET7PSR);
	sys_write8(TIMER_ETPSR_32768HZ, config->base + REG_TIMER_ET8PSR);

	/* Set top value ticks to top timer */
	sys_write32(config->info.max_top_value, config->base + REG_TIMER_ET8CNTLLR);

	config->irq_config_func(dev);

	LOG_DBG("Max top timer ticks = 0x%x", config->info.max_top_value);
	LOG_DBG("Clock frequency = %d", config->info.freq);
	LOG_DBG("Channels = %d", config->info.channels);

	return 0;
}

static DEVICE_API(counter, counter_it51xxx_driver_api) = {
	.start = counter_it51xxx_start,
	.stop = counter_it51xxx_stop,
	.get_value = counter_it51xxx_get_value,
	.set_alarm = counter_it51xxx_set_alarm,
	.cancel_alarm = counter_it51xxx_cancel_alarm,
	.set_top_value = counter_it51xxx_set_top_value,
	.get_top_value = counter_it51xxx_get_top_value,
};

#define COUNTER_IT51XXX_INIT(inst)                                                                 \
	static void counter_it51xxx_cfg_func_##inst(const struct device *dev)                      \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN_BY_IDX(inst, 0), 0, counter_it51xxx_alarm_isr,            \
			    DEVICE_DT_INST_GET(inst), 0);                                          \
		IRQ_CONNECT(DT_INST_IRQN_BY_IDX(inst, 1), 0, counter_it51xxx_top_isr,              \
			    DEVICE_DT_INST_GET(inst), 0);                                          \
	}                                                                                          \
                                                                                                   \
	static struct counter_it51xxx_config counter_it51xxx_config_##inst = {                     \
		.info =                                                                            \
			{                                                                          \
				.max_top_value = UINT32_MAX,                                       \
				.freq = 32768,                                                     \
				.flags = 0,                                                        \
				.channels = 1,                                                     \
			},                                                                         \
		.base = DT_INST_REG_ADDR(inst),                                                    \
		.alarm_irq = DT_INST_IRQN_BY_IDX(inst, 0),                                         \
		.alarm_flag = DT_INST_IRQ_BY_IDX(inst, 0, flags),                                  \
		.top_irq = DT_INST_IRQN_BY_IDX(inst, 1),                                           \
		.top_flag = DT_INST_IRQ_BY_IDX(inst, 1, flags),                                    \
		.irq_config_func = counter_it51xxx_cfg_func_##inst,                                \
	};                                                                                         \
                                                                                                   \
	static struct counter_it51xxx_data counter_it51xxx_data_##inst;                            \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &counter_it51xxx_init, NULL, &counter_it51xxx_data_##inst,     \
			      &counter_it51xxx_config_##inst, POST_KERNEL,                         \
			      CONFIG_COUNTER_INIT_PRIORITY, &counter_it51xxx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(COUNTER_IT51XXX_INIT)
BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "only one ite,it51xxx-counter compatible node can be supported");
