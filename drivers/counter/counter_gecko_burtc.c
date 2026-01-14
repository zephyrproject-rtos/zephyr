/*
 * Copyright (c) 2026, Shontal Biton
 *
 * Based on:
 *  counter_gecko_rtcc.c Copyright (c) 2019, Piotr Mienkowski
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_burtc_counter

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/logging/log.h>

#include <em_cmu.h>
#include <em_burtc.h>

LOG_MODULE_REGISTER(counter_gecko_burtc, CONFIG_COUNTER_LOG_LEVEL);

#define BURTC_MAX_VALUE (_BURTC_CNT_MASK)
#define BURTC_ALARM_NUM 1

struct counter_gecko_config {
	struct counter_config_info info;
	void (*irq_config)(void);
	uint32_t prescaler;
};

struct counter_gecko_data {
#if defined(CONFIG_COUNTER_GECKO_BURTC_ALARM)
	counter_alarm_callback_t callback;
	void *user_data;
#else
	counter_top_callback_t top_callback;
	void *top_user_data;
#endif
};

static int counter_gecko_start(const struct device *dev)
{
	ARG_UNUSED(dev);

	BURTC_Start();
	return 0;
}

static int counter_gecko_stop(const struct device *dev)
{
	ARG_UNUSED(dev);

	BURTC_Stop();
	return 0;
}

static int counter_gecko_get_value(const struct device *dev, uint32_t *ticks)
{
	ARG_UNUSED(dev);

	*ticks = BURTC_CounterGet();
	return 0;
}

#if defined(CONFIG_COUNTER_GECKO_BURTC_ALARM)
static int counter_gecko_set_alarm(const struct device *dev, uint8_t chan_id,
				   const struct counter_alarm_cfg *alarm_cfg)
{
	struct counter_gecko_data *const dev_data = dev->data;
	uint32_t alarm_ticks = alarm_cfg->ticks;

	/* BURTC has only one channel */
	if (chan_id != 0) {
		return -EINVAL;
	}

	/* No need to validate top value because burtc doesn't support it. */
	if (dev_data->callback != NULL) {
		return -EINVAL;
	}

	if (!(alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE)) {
		alarm_ticks += BURTC_CounterGet();
	}

	BURTC_IntClear(BURTC_IF_COMP);

	dev_data->callback = alarm_cfg->callback;
	dev_data->user_data = alarm_cfg->user_data;

	BURTC_CompareSet(chan_id, alarm_ticks);
	LOG_DBG("set alarm: %u", alarm_ticks);

	/* Enable the compare interrupt */
	BURTC_IntEnable(BURTC_IF_COMP);
	return 0;
}

static int counter_gecko_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	struct counter_gecko_data *const dev_data = dev->data;

	/* BURTC has only one channel */
	if (chan_id != 0) {
		return -EINVAL;
	}

	/* Disable the compare interrupt */
	BURTC_IntDisable(BURTC_IF_COMP);
	BURTC_IntClear(BURTC_IF_COMP);

	dev_data->callback = NULL;
	dev_data->user_data = NULL;

	BURTC_CompareSet(chan_id, 0);
	LOG_DBG("cancel alarm");
	return 0;
}

static int counter_gecko_set_top_value(const struct device *dev, const struct counter_top_cfg *cfg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cfg);
	return -ENOTSUP;
}

static uint32_t counter_gecko_get_top_value(const struct device *dev)
{
	ARG_UNUSED(dev);
	return -ENOTSUP;
}
#else
static int counter_gecko_set_top_value(const struct device *dev, const struct counter_top_cfg *cfg)
{
	struct counter_gecko_data *const dev_data = dev->data;
	uint32_t ticks;
	uint32_t flags;
	int err = 0;

	BURTC_IntClear(BURTC_IF_COMP);

	dev_data->top_callback = cfg->callback;
	dev_data->top_user_data = cfg->user_data;
	ticks = cfg->ticks;
	flags = cfg->flags;

	if (!(flags & COUNTER_TOP_CFG_DONT_RESET)) {
		BURTC_CounterReset();
	} else if (BURTC_CounterGet() > ticks) {
		if (flags & COUNTER_TOP_CFG_RESET_WHEN_LATE) {
			BURTC_CounterReset();
		} else {
			err = -ETIME;
		}
	}

	BURTC_CompareSet(0, ticks);
	LOG_DBG("set top value: %u", ticks);

	/* Enable the compare interrupt */
	BURTC_IntEnable(BURTC_IF_COMP);
	return err;
}

static uint32_t counter_gecko_get_top_value(const struct device *dev)
{
	ARG_UNUSED(dev);
	return BURTC_CompareGet(0);
}

static int counter_gecko_set_alarm(const struct device *dev, uint8_t chan_id,
				   const struct counter_alarm_cfg *alarm_cfg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(chan_id);
	ARG_UNUSED(alarm_cfg);
	return -ENOTSUP;
}
static int counter_gecko_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(chan_id);
	return -ENOTSUP;
}
#endif

static uint32_t counter_gecko_get_pending_int(const struct device *dev)
{
	ARG_UNUSED(dev);
	return BURTC_IntGetEnabled() != 0;
}

static int counter_gecko_init(const struct device *dev)
{
	const struct counter_gecko_config *const dev_cfg = dev->config;

	BURTC_Init_TypeDef burtc_config = {
		false,              /* Don't start counting */
		false,              /* Disable RTC during debug halt. */
		dev_cfg->prescaler, /* Set clkdiv. */
#if defined(CONFIG_COUNTER_GECKO_BURTC_ALARM)
		false, /* Don't set compare value as the top value. */
		true,  /* Enable EM4 wakeup on compare match. */
		false, /* Disable EM4 wakeup on overflaow. */
#else
		true,  /* Set compare value as the top value. */
		false, /* Enable EM4 wakeup on compare match. */
		true,  /* Disable EM4 wakeup on overflaow. */
#endif
	};

	/* Enable BURTC module clock */
	CMU_ClockEnable(cmuClock_BURTC, true);

	/* Initialize BURTC */
	BURTC_Init(&burtc_config);

	/* Disable module's internal interrupt sources */
	BURTC_IntDisable(BURTC_IF_COMP);
	BURTC_IntDisable(BURTC_IF_OF);
	BURTC_IntClear(BURTC_IF_COMP);
	BURTC_IntClear(BURTC_IF_OF);

	/* Clear the counter */
	BURTC_CounterReset();

	/* Configure & enable module interrupts */
	dev_cfg->irq_config();
	LOG_INF("Device %s initialized", dev->name);
	return 0;
}

static DEVICE_API(counter, counter_gecko_driver_api) = {
	.start = counter_gecko_start,
	.stop = counter_gecko_stop,
	.get_value = counter_gecko_get_value,
	.set_alarm = counter_gecko_set_alarm,
	.cancel_alarm = counter_gecko_cancel_alarm,
	.set_top_value = counter_gecko_set_top_value,
	.get_pending_int = counter_gecko_get_pending_int,
	.get_top_value = counter_gecko_get_top_value,
};

ISR_DIRECT_DECLARE(counter_gecko_isr_0)
{
	const struct device *const dev = DEVICE_DT_INST_GET(0);
	struct counter_gecko_data *const dev_data = dev->data;
	counter_alarm_callback_t alarm_callback;
	uint32_t count = BURTC_CounterGet();
	uint32_t flags = BURTC_IntGetEnabled();

	BURTC_IntClear(flags);

#if defined(CONFIG_COUNTER_GECKO_BURTC_ALARM)
	if (flags & BURTC_IF_COMP && dev_data->callback) {
		alarm_callback = dev_data->callback;
		dev_data->callback = NULL;
		alarm_callback(dev, 0, count, dev_data->user_data);
	}
#else
	if (flags & BURTC_IF_COMP && dev_data->top_callback) {
		dev_data->top_callback(dev, dev_data->top_user_data);
	}
#endif

	ISR_DIRECT_PM();
	return 1;
}

static void counter_gecko_0_irq_config(void)
{
	IRQ_DIRECT_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), counter_gecko_isr_0, 0);
	irq_enable(DT_INST_IRQN(0));
}

static const struct counter_gecko_config counter_gecko_0_config = {
	.info = {
			.max_top_value = BURTC_MAX_VALUE,
			.freq = DT_INST_PROP(0, clock_frequency) / DT_INST_PROP(0, clock_div),
			.flags = COUNTER_CONFIG_INFO_COUNT_UP,
			.channels = BURTC_ALARM_NUM,
		},
	.irq_config = counter_gecko_0_irq_config,
	.prescaler = DT_INST_PROP(0, clock_div),
};

static struct counter_gecko_data counter_gecko_0_data;

DEVICE_DT_INST_DEFINE(0, counter_gecko_init, NULL, &counter_gecko_0_data, &counter_gecko_0_config,
		      PRE_KERNEL_1, CONFIG_COUNTER_INIT_PRIORITY, &counter_gecko_driver_api);
