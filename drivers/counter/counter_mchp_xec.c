/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_xec_timer

/**
 * @file
 * @brief Microchip XEC Counter driver
 *
 * This is the driver for the 16/32-bit counters on the Microchip SoCs.
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

#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(counter_mchp_xec, CONFIG_COUNTER_LOG_LEVEL);

#include <zephyr/drivers/counter.h>
#include <soc.h>
#include <errno.h>
#include <stdbool.h>

struct counter_xec_config {
	struct counter_config_info info;
	void (*config_func)(void);
	uint32_t base_address;
	uint16_t prescaler;
	uint8_t girq_id;
	uint8_t girq_bit;
};

struct counter_xec_data {
	counter_alarm_callback_t alarm_cb;
	counter_top_callback_t top_cb;
	void *user_data;
};

#define COUNTER_XEC_REG_BASE(_dev)			\
	((struct btmr_regs *)					\
	 ((const struct counter_xec_config * const)	\
	  _dev->config)->base_address)

#define COUNTER_XEC_CONFIG(_dev)			\
	(((const struct counter_xec_config * const)	\
	  _dev->config))

#define COUNTER_XEC_DATA(_dev)				\
	((struct counter_xec_data *)dev->data)

static int counter_xec_start(const struct device *dev)
{
	struct btmr_regs *counter = COUNTER_XEC_REG_BASE(dev);

	if (counter->CTRL & MCHP_BTMR_CTRL_ENABLE) {
		return -EALREADY;
	}

	counter->CTRL |= (MCHP_BTMR_CTRL_ENABLE | MCHP_BTMR_CTRL_START);

	LOG_DBG("%p Counter started", dev);

	return 0;
}

static int counter_xec_stop(const struct device *dev)
{
	struct btmr_regs *counter = COUNTER_XEC_REG_BASE(dev);
	uint32_t reg;

	if (!(counter->CTRL & MCHP_BTMR_CTRL_ENABLE)) {
		/* Already stopped, nothing to do */
		return 0;
	}

	reg = counter->CTRL;
	reg &= ~MCHP_BTMR_CTRL_ENABLE;
	reg &= ~MCHP_BTMR_CTRL_START;
	reg &= ~MCHP_BTMR_CTRL_HALT;
	reg &= ~MCHP_BTMR_CTRL_RELOAD;
	reg &= ~MCHP_BTMR_CTRL_AUTO_RESTART;
	counter->CTRL = reg;

	counter->IEN = MCHP_BTMR_INTDIS;
	counter->CNT = counter->PRLD;

	LOG_DBG("%p Counter stopped", dev);

	return 0;
}

static int counter_xec_get_value(const struct device *dev, uint32_t *ticks)
{
	struct btmr_regs *counter = COUNTER_XEC_REG_BASE(dev);

	*ticks = counter->CNT;
	return 0;
}

static int counter_xec_set_alarm(const struct device *dev, uint8_t chan_id,
				 const struct counter_alarm_cfg *alarm_cfg)
{
	struct btmr_regs *counter = COUNTER_XEC_REG_BASE(dev);
	struct counter_xec_data *data = COUNTER_XEC_DATA(dev);

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

	if (alarm_cfg->ticks > counter->PRLD) {
		return -EINVAL;
	}

	counter->CNT = alarm_cfg->ticks;

	data->alarm_cb = alarm_cfg->callback;
	data->user_data = alarm_cfg->user_data;

	counter->IEN = MCHP_BTMR_INTEN;

	LOG_DBG("%p Counter alarm set to %u ticks", dev, alarm_cfg->ticks);

	counter->CTRL |= MCHP_BTMR_CTRL_START;

	return 0;
}


static int counter_xec_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	struct btmr_regs *counter = COUNTER_XEC_REG_BASE(dev);
	struct counter_xec_data *data = COUNTER_XEC_DATA(dev);

	if (chan_id != 0) {
		LOG_ERR("Invalid channel id %u", chan_id);
		return -ENOTSUP;
	}

	counter->CTRL &= ~MCHP_BTMR_CTRL_START;
	counter->IEN = MCHP_BTMR_INTDIS;

	data->alarm_cb = NULL;
	data->user_data = NULL;

	LOG_DBG("%p Counter alarm canceled", dev);

	return 0;
}

static uint32_t counter_xec_get_pending_int(const struct device *dev)
{
	struct btmr_regs *counter = COUNTER_XEC_REG_BASE(dev);

	return counter->STS;
}

static uint32_t counter_xec_get_top_value(const struct device *dev)
{
	struct btmr_regs *counter = COUNTER_XEC_REG_BASE(dev);

	return counter->PRLD;
}

static int counter_xec_set_top_value(const struct device *dev,
				     const struct counter_top_cfg *cfg)
{
	struct btmr_regs *counter = COUNTER_XEC_REG_BASE(dev);
	const struct counter_xec_config *counter_cfg = COUNTER_XEC_CONFIG(dev);
	struct counter_xec_data *data = COUNTER_XEC_DATA(dev);
	int ret = 0;
	bool restart;

	if (data->alarm_cb) {
		return -EBUSY;
	}

	if (cfg->ticks > counter_cfg->info.max_top_value) {
		return -EINVAL;
	}

	restart = ((counter->CTRL & MCHP_BTMR_CTRL_START) != 0U);

	counter->CTRL &= ~MCHP_BTMR_CTRL_START;

	if (cfg->flags & COUNTER_TOP_CFG_DONT_RESET) {
		if (counter->CNT > cfg->ticks) {
			ret = -ETIME;

			if (cfg->flags & COUNTER_TOP_CFG_RESET_WHEN_LATE) {
				counter->CNT = cfg->ticks;
			}
		}
	} else {
		counter->CNT = cfg->ticks;
	}

	counter->PRLD = cfg->ticks;

	data->top_cb = cfg->callback;
	data->user_data = cfg->user_data;

	if (data->top_cb) {
		counter->IEN = MCHP_BTMR_INTEN;
		counter->CTRL |= MCHP_BTMR_CTRL_AUTO_RESTART;
	} else {
		counter->IEN = MCHP_BTMR_INTDIS;
		counter->CTRL &= ~MCHP_BTMR_CTRL_AUTO_RESTART;
	}

	LOG_DBG("%p Counter top value was set to %u", dev, cfg->ticks);

	if (restart) {
		counter->CTRL |= MCHP_BTMR_CTRL_START;
	}

	return ret;
}

static void counter_xec_isr(const struct device *dev)
{
	struct btmr_regs *counter = COUNTER_XEC_REG_BASE(dev);
	const struct counter_xec_config *counter_cfg = COUNTER_XEC_CONFIG(dev);
	struct counter_xec_data *data = COUNTER_XEC_DATA(dev);
	counter_alarm_callback_t alarm_cb;
	void *user_data;

	counter->STS = MCHP_BTMR_STS_ACTIVE;

#if defined(CONFIG_SOC_MEC172X_NSZ)
	mchp_soc_ecia_girq_src_clr(counter_cfg->girq_id, counter_cfg->girq_bit);
#else
	MCHP_GIRQ_SRC(counter_cfg->girq_id) = BIT(counter_cfg->girq_bit);
#endif

	LOG_DBG("%p Counter ISR", dev);

	if (data->alarm_cb) {
		/* Alarm is one-shot, so disable interrupt and callback */
		counter->IEN = MCHP_BTMR_INTDIS;

		alarm_cb = data->alarm_cb;
		data->alarm_cb = NULL;
		user_data = data->user_data;

		alarm_cb(dev, 0, counter->CNT, user_data);
	} else if (data->top_cb) {
		data->top_cb(dev, data->user_data);
	}
}

static const struct counter_driver_api counter_xec_api = {
		.start = counter_xec_start,
		.stop = counter_xec_stop,
		.get_value = counter_xec_get_value,
		.set_alarm = counter_xec_set_alarm,
		.cancel_alarm = counter_xec_cancel_alarm,
		.set_top_value = counter_xec_set_top_value,
		.get_pending_int = counter_xec_get_pending_int,
		.get_top_value = counter_xec_get_top_value,
};

static int counter_xec_init(const struct device *dev)
{
	struct btmr_regs *counter = COUNTER_XEC_REG_BASE(dev);
	const struct counter_xec_config *counter_cfg = COUNTER_XEC_CONFIG(dev);

	counter_xec_stop(dev);

	counter->CTRL &= ~MCHP_BTMR_CTRL_COUNT_UP;
	counter->CTRL |= (counter_cfg->prescaler << MCHP_BTMR_CTRL_PRESCALE_POS) &
		MCHP_BTMR_CTRL_PRESCALE_MASK;

	/* Set preload and actually pre-load the counter */
	counter->PRLD = counter_cfg->info.max_top_value;
	counter->CNT = counter_cfg->info.max_top_value;

#if defined(CONFIG_SOC_MEC172X_NSZ)
	mchp_soc_ecia_girq_src_en(counter_cfg->girq_id, counter_cfg->girq_bit);
#else
	MCHP_GIRQ_ENSET(counter_cfg->girq_id) = BIT(counter_cfg->girq_bit);
#endif

	counter_cfg->config_func();

	return 0;
}

#define COUNTER_XEC_INIT(inst)						\
	static void counter_xec_irq_config_##inst(void);		\
									\
	static struct counter_xec_data counter_xec_dev_data_##inst;	\
									\
	static struct counter_xec_config counter_xec_dev_config_##inst = { \
		.info = {						\
			.max_top_value = DT_INST_PROP(inst, max_value),	\
			.freq = DT_INST_PROP(inst, clock_frequency) /	\
			(1 << DT_INST_PROP(inst, prescaler)),		\
			.flags = 0,					\
			.channels = 1,					\
		},							\
									\
		.config_func = counter_xec_irq_config_##inst,		\
		.base_address = DT_INST_REG_ADDR(inst),			\
		.prescaler = DT_INST_PROP(inst, prescaler),		\
		.girq_id = DT_INST_PROP_BY_IDX(0, girqs, 0),		\
		.girq_bit = DT_INST_PROP_BY_IDX(0, girqs, 1),		\
	};								\
									\
	DEVICE_DT_INST_DEFINE(inst,					\
			    counter_xec_init,				\
			    NULL,					\
			    &counter_xec_dev_data_##inst,		\
			    &counter_xec_dev_config_##inst,		\
			    POST_KERNEL,				\
			    CONFIG_COUNTER_INIT_PRIORITY,		\
			    &counter_xec_api);				\
									\
	static void counter_xec_irq_config_##inst(void)			\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(inst),				\
			    DT_INST_IRQ(inst, priority),		\
			    counter_xec_isr,				\
			    DEVICE_DT_INST_GET(inst), 0);		\
		irq_enable(DT_INST_IRQN(inst));				\
	}

DT_INST_FOREACH_STATUS_OKAY(COUNTER_XEC_INIT)
