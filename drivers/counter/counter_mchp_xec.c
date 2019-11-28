/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(counter_mchp_xec, CONFIG_COUNTER_LOG_LEVEL);

#include <counter.h>
#include <soc.h>
#include <errno.h>

struct counter_xec_config {
	struct counter_config_info info;
	void (*config_func)(void);
	u32_t base_address;
	u16_t prescaler;
	u8_t girq_id;
	u8_t girq_bit;
};

struct counter_xec_data {
	counter_alarm_callback_t alarm_cb;
	counter_top_callback_t top_cb;
	void *user_data;
};

#define COUNTER_XEC_REG_BASE(_dev)			\
	((BTMR_Type *)					\
	 ((const struct counter_xec_config * const)	\
	  _dev->config->config_info)->base_address)

#define COUNTER_XEC_CONFIG(_dev)			\
	(((const struct counter_xec_config * const)	\
	  _dev->config->config_info))

#define COUNTER_XEC_DATA(_dev)				\
	((struct counter_xec_data *)dev->driver_data)

static int counter_xec_start(struct device *dev)
{
	BTMR_Type *counter = COUNTER_XEC_REG_BASE(dev);

	if (counter->CTRL & MCHP_BTMR_CTRL_ENABLE) {
		return -EALREADY;
	}

	counter->CTRL |= MCHP_BTMR_CTRL_ENABLE;

	LOG_DBG("%p Counter started", dev);

	return 0;
}

static int counter_xec_stop(struct device *dev)
{
	BTMR_Type *counter = COUNTER_XEC_REG_BASE(dev);
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
	counter->CNT = 0;

	LOG_DBG("%p Counter stopped", dev);

	return 0;
}

static u32_t counter_xec_read(struct device *dev)
{
	BTMR_Type *counter = COUNTER_XEC_REG_BASE(dev);

	return counter->CNT;
}

static int counter_xec_set_alarm(struct device *dev, u8_t chan_id,
				 const struct counter_alarm_cfg *alarm_cfg)
{
	BTMR_Type *counter = COUNTER_XEC_REG_BASE(dev);
	const struct counter_xec_config *counter_cfg = COUNTER_XEC_CONFIG(dev);
	struct counter_xec_data *data = COUNTER_XEC_DATA(dev);
	u32_t ticks;

	if (chan_id != 0) {
		LOG_ERR("Invalid channel id %u", chan_id);
		return -ENOTSUP;
	}

	if (!(counter->CTRL & MCHP_BTMR_CTRL_ENABLE)) {
		return -EIO;
	}

	if (counter->CTRL & MCHP_BTMR_CTRL_START) {
		return -EBUSY;
	}

	if (!alarm_cfg->callback) {
		return -EINVAL;
	}

	ticks = alarm_cfg->ticks;

	if (counter_cfg->info.max_top_value == UINT16_MAX) {
		if (ticks > UINT16_MAX) {
			return -EINVAL;
		}
	}

	if (!(alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE)) {
		u64_t abs_cnt = ticks + counter->CNT;

		ticks = (u32_t) abs_cnt % counter_cfg->info.max_top_value;
	}

	counter->CNT = ticks;

	data->alarm_cb = alarm_cfg->callback;
	data->user_data = alarm_cfg->user_data;

	counter->IEN = MCHP_BTMR_INTEN;

	LOG_DBG("%p Counter alarm set to %u ticks", dev, ticks);

	counter->CTRL |= MCHP_BTMR_CTRL_START;

	return 0;
}


static int counter_xec_cancel_alarm(struct device *dev, u8_t chan_id)
{
	BTMR_Type *counter = COUNTER_XEC_REG_BASE(dev);
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

static u32_t counter_xec_get_pending_int(struct device *dev)
{
	BTMR_Type *counter = COUNTER_XEC_REG_BASE(dev);

	return counter->STS;
}

static u32_t counter_xec_get_top_value(struct device *dev)
{
	BTMR_Type *counter = COUNTER_XEC_REG_BASE(dev);

	return counter->PRLD;
}

static int counter_xec_set_top_value(struct device *dev,
				     const struct counter_top_cfg *cfg)
{
	BTMR_Type *counter = COUNTER_XEC_REG_BASE(dev);
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

	restart = (counter->CTRL && MCHP_BTMR_CTRL_START);

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

static u32_t counter_xec_get_max_relative_alarm(struct device *dev)
{
	const struct counter_xec_config *counter_cfg = COUNTER_XEC_CONFIG(dev);

	return counter_cfg->info.max_top_value;
}

static void counter_xec_isr(struct device *dev)
{
	BTMR_Type *counter = COUNTER_XEC_REG_BASE(dev);
	const struct counter_xec_config *counter_cfg = COUNTER_XEC_CONFIG(dev);
	struct counter_xec_data *data = COUNTER_XEC_DATA(dev);

	counter->STS = MCHP_BTMR_STS_ACTIVE;
	MCHP_GIRQ_SRC(counter_cfg->girq_id) = BIT(counter_cfg->girq_bit);

	LOG_DBG("%p Counter ISR", dev);

	if (data->alarm_cb) {
		data->alarm_cb(dev, 0, counter->CNT, data->user_data);
	} else if (data->top_cb) {
		data->top_cb(dev, data->user_data);
	}
}

static const struct counter_driver_api counter_xec_api = {
		.start = counter_xec_start,
		.stop = counter_xec_stop,
		.read = counter_xec_read,
		.set_alarm = counter_xec_set_alarm,
		.cancel_alarm = counter_xec_cancel_alarm,
		.set_top_value = counter_xec_set_top_value,
		.get_pending_int = counter_xec_get_pending_int,
		.get_top_value = counter_xec_get_top_value,
		.get_max_relative_alarm = counter_xec_get_max_relative_alarm,
};

static int counter_xec_init(struct device *dev)
{
	BTMR_Type *counter = COUNTER_XEC_REG_BASE(dev);
	const struct counter_xec_config *counter_cfg = COUNTER_XEC_CONFIG(dev);

	counter_xec_stop(dev);

	counter->CTRL &= ~MCHP_BTMR_CTRL_COUNT_UP;
	counter->CTRL |= (counter_cfg->prescaler << MCHP_BTMR_CTRL_PRESCALE_POS) &
		MCHP_BTMR_CTRL_PRESCALE_MASK;

	MCHP_GIRQ_ENSET(counter_cfg->girq_id) = BIT(counter_cfg->girq_bit);

	counter_cfg->config_func();

	return 0;
}

#if defined(DT_COUNTER_MCHP_XEC_0)

static void counder_xec_irq_config_0(void);

static struct counter_xec_data counter_xec_dev_data_0;

static struct counter_xec_config counter_xec_dev_config_0 = {
	.info = {
		.max_top_value = DT_COUNTEX_MCHP_XEC_0_MAX_VALUE,
		.freq = DT_COUNTER_MCHP_XEC_0_CLOCK_FREQUENCY /
			(1 << DT_COUNTER_MCHP_XEC_0_PRESCALER),
		.flags = 0,
		.channels = 1,
	},

	.config_func = counder_xec_irq_config_0,
	.base_address = DT_COUNTER_MCHP_XEC_0_BASE_ADDR,
	.prescaler = DT_COUNTER_MCHP_XEC_0_PRESCALER,
	.girq_id = MCHP_B16TMR0_GIRQ,
	.girq_bit = MCHP_B16TMR0_GIRQ_POS,
};

DEVICE_AND_API_INIT(counter_xec_0, DT_COUNTER_MCHP_XEC_0_LABEL,
		    counter_xec_init, &counter_xec_dev_data_0,
		    &counter_xec_dev_config_0,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &counter_xec_api);

static void counder_xec_irq_config_0(void)
{
	IRQ_CONNECT(DT_COUNTER_MCHP_XEC_0_IRQ,
		    DT_COUNTER_MCHP_XEC_0_IRQ_PRIORITY,
		    counter_xec_isr, DEVICE_GET(counter_xec_0), 0);
	irq_enable(DT_COUNTER_MCHP_XEC_0_IRQ);
}

#endif /* DT_COUNTER_MCHP_XEC_0 */

#if defined(DT_COUNTER_MCHP_XEC_1)

static void counder_xec_irq_config_1(void);

static struct counter_xec_data counter_xec_dev_data_1;

static struct counter_xec_config counter_xec_dev_config_1 = {
	.info = {
		.max_top_value = DT_COUNTEX_MCHP_XEC_1_MAX_VALUE,
		.freq = DT_COUNTER_MCHP_XEC_1_CLOCK_FREQUENCY /
			(1 << DT_COUNTER_MCHP_XEC_1_PRESCALER),
		.flags = 0,
		.channels = 1,
	},

	.config_func = counder_xec_irq_config_1,
	.base_address = DT_COUNTER_MCHP_XEC_1_BASE_ADDR,
	.prescaler = DT_COUNTER_MCHP_XEC_1_PRESCALER,
	.girq_id = MCHP_B16TMR1_GIRQ,
	.girq_bit = MCHP_B16TMR1_GIRQ_POS,
};

DEVICE_AND_API_INIT(counter_xec_1, DT_COUNTER_MCHP_XEC_1_LABEL,
		    counter_xec_init, &counter_xec_dev_data_1,
		    &counter_xec_dev_config_1,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &counter_xec_api);

static void counder_xec_irq_config_1(void)
{
	IRQ_CONNECT(DT_COUNTER_MCHP_XEC_1_IRQ,
		    DT_COUNTER_MCHP_XEC_1_IRQ_PRIORITY,
		    counter_xec_isr, DEVICE_GET(counter_xec_1), 0);
	irq_enable(DT_COUNTER_MCHP_XEC_1_IRQ);
}

#endif /* DT_COUNTER_MCHP_XEC_1 */

#if defined(DT_COUNTER_MCHP_XEC_3)

static void counder_xec_irq_config_3(void);

static struct counter_xec_data counter_xec_dev_data_3;

static struct counter_xec_config counter_xec_dev_config_3 = {
	.info = {
		.max_top_value = DT_COUNTEX_MCHP_XEC_3_MAX_VALUE,
		.freq = DT_COUNTER_MCHP_XEC_3_CLOCK_FREQUENCY /
			(1 << DT_COUNTER_MCHP_XEC_3_PRESCALER),
		.flags = 0,
		.channels = 1,
	},

	.config_func = counder_xec_irq_config_3,
	.base_address = DT_COUNTER_MCHP_XEC_3_BASE_ADDR,
	.prescaler = DT_COUNTER_MCHP_XEC_3_PRESCALER,
	.girq_id = MCHP_B32TMR1_GIRQ,
	.girq_bit = MCHP_B32TMR1_GIRQ_POS,
};

DEVICE_AND_API_INIT(counter_xec_3, DT_COUNTER_MCHP_XEC_3_LABEL,
		    counter_xec_init, &counter_xec_dev_data_3,
		    &counter_xec_dev_config_3,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &counter_xec_api);

static void counder_xec_irq_config_3(void)
{
	IRQ_CONNECT(DT_COUNTER_MCHP_XEC_3_IRQ,
		    DT_COUNTER_MCHP_XEC_3_IRQ_PRIORITY,
		    counter_xec_isr, DEVICE_GET(counter_xec_3), 0);
	irq_enable(DT_COUNTER_MCHP_XEC_3_IRQ);
}

#endif /* DT_COUNTER_MCHP_XEC_3 */
