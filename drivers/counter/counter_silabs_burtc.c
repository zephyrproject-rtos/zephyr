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
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_silabs.h>
#include <zephyr/logging/log.h>

#include <sl_hal_burtc.h>

LOG_MODULE_REGISTER(counter_silabs_burtc, CONFIG_COUNTER_LOG_LEVEL);

#define BURTC_MAX_VALUE (_BURTC_CNT_MASK)
#define BURTC_ALARM_NUM 1

struct counter_silabs_config {
	struct counter_config_info info;
	const struct device *clock_dev;
	const struct silabs_clock_control_cmu_config clock_cfg;
	void (*irq_config)(void);
	uint32_t prescaler;
};

struct counter_silabs_data {
#if defined(CONFIG_COUNTER_SILABS_BURTC_ALARM)
	counter_alarm_callback_t callback;
	void *user_data;
#else
	counter_top_callback_t top_callback;
	void *top_user_data;
#endif
};

static int counter_silabs_start(const struct device *dev)
{
	ARG_UNUSED(dev);

	sl_hal_burtc_start();
	return 0;
}

static int counter_silabs_stop(const struct device *dev)
{
	ARG_UNUSED(dev);

	sl_hal_burtc_stop();
	return 0;
}

static int counter_silabs_get_value(const struct device *dev, uint32_t *ticks)
{
	ARG_UNUSED(dev);

	*ticks = sl_hal_burtc_get_counter();
	return 0;
}

static uint32_t counter_silabs_get_top_value(const struct device *dev)
{
	ARG_UNUSED(dev);
	return sl_hal_burtc_get_compare();
}

static uint32_t counter_silabs_get_freq(const struct device *dev)
{
	const struct counter_silabs_config *const config = dev->config;
	const struct device *clock_dev = config->clock_dev;
	uint32_t freq_hz = 0;

	if (clock_control_get_rate(clock_dev, (clock_control_subsys_t)&config->clock_cfg,
				   &freq_hz)) {
		return 0;
	}
	return freq_hz / config->prescaler;
}

#if defined(CONFIG_COUNTER_SILABS_BURTC_ALARM)
static int counter_silabs_set_alarm(const struct device *dev, uint8_t chan_id,
				    const struct counter_alarm_cfg *alarm_cfg)
{
	struct counter_silabs_data *const dev_data = dev->data;
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
		alarm_ticks += sl_hal_burtc_get_counter();
	}

	sl_hal_burtc_clear_interrupts(BURTC_IF_COMP);

	dev_data->callback = alarm_cfg->callback;
	dev_data->user_data = alarm_cfg->user_data;

	sl_hal_burtc_set_compare(alarm_ticks);
	LOG_DBG("set alarm: %u", alarm_ticks);

	/* Enable the compare interrupt */
	sl_hal_burtc_enable_interrupts(BURTC_IF_COMP);
	return 0;
}

static int counter_silabs_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	struct counter_silabs_data *const dev_data = dev->data;

	/* BURTC has only one channel */
	if (chan_id != 0) {
		return -EINVAL;
	}

	/* Disable the compare interrupt */
	sl_hal_burtc_disable_interrupts(BURTC_IF_COMP);
	sl_hal_burtc_clear_interrupts(BURTC_IF_COMP);

	dev_data->callback = NULL;
	dev_data->user_data = NULL;

	sl_hal_burtc_set_compare(0);
	LOG_DBG("cancel alarm");
	return 0;
}

static int counter_silabs_set_top_value(const struct device *dev, const struct counter_top_cfg *cfg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cfg);
	return -ENOTSUP;
}

#else
static int counter_silabs_set_top_value(const struct device *dev, const struct counter_top_cfg *cfg)
{
	struct counter_silabs_data *const dev_data = dev->data;
	uint32_t ticks;
	uint32_t flags;
	int err = 0;

	sl_hal_burtc_clear_interrupts(BURTC_IF_COMP);

	dev_data->top_callback = cfg->callback;
	dev_data->top_user_data = cfg->user_data;
	ticks = cfg->ticks;
	flags = cfg->flags;

	if (!(flags & COUNTER_TOP_CFG_DONT_RESET)) {
		sl_hal_burtc_reset_counter();
	} else if (sl_hal_burtc_get_counter() > ticks) {
		if (flags & COUNTER_TOP_CFG_RESET_WHEN_LATE) {
			sl_hal_burtc_reset_counter();
		} else {
			err = -ETIME;
		}
	}

	sl_hal_burtc_set_compare(ticks);
	LOG_DBG("set top value: %u", ticks);

	/* Enable the compare interrupt */
	sl_hal_burtc_enable_interrupts(BURTC_IF_COMP);
	return err;
}

static int counter_silabs_set_alarm(const struct device *dev, uint8_t chan_id,
				    const struct counter_alarm_cfg *alarm_cfg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(chan_id);
	ARG_UNUSED(alarm_cfg);
	return -ENOTSUP;
}

static int counter_silabs_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(chan_id);
	return -ENOTSUP;
}
#endif

static uint32_t counter_silabs_get_pending_int(const struct device *dev)
{
	ARG_UNUSED(dev);
	return sl_hal_burtc_get_enabled_pending_interrupts() != 0;
}

static int counter_silabs_init(const struct device *dev)
{
	int err = 0;
	const struct counter_silabs_config *const burtc_cfg = dev->config;

	sl_hal_burtc_init_t burtc_config = {
		burtc_cfg->prescaler, /* Set clkdiv. */
		false,                /* Disable RTC during debug halt. */
#if defined(CONFIG_COUNTER_SILABS_BURTC_ALARM)
		false, /* Don't set compare value as the top value. */
		true,  /* Enable EM4 wakeup on compare match. */
		false, /* Disable EM4 wakeup on overflow. */
#else
		true,  /* Set compare value as the top value. */
		false, /* Disable EM4 wakeup on compare match. */
		true,  /* Enable EM4 wakeup on overflow. */
#endif
	};

	/* Enable BURTC clock */
	err = clock_control_on(burtc_cfg->clock_dev, (clock_control_subsys_t)&burtc_cfg->clock_cfg);
	if (err < 0 && err != -EALREADY) {
		return err;
	}

	/* Initialize BURTC */
	sl_hal_burtc_init(&burtc_config);
	sl_hal_burtc_enable();

	/* Disable module's internal interrupt sources */
	sl_hal_burtc_disable_interrupts(BURTC_IF_COMP | BURTC_IF_OF);
	sl_hal_burtc_clear_interrupts(BURTC_IF_COMP | BURTC_IF_OF);

	/* Clear the counter */
	sl_hal_burtc_reset_counter();

	/* Configure & enable module interrupts */
	burtc_cfg->irq_config();
	LOG_INF("Device %s initialized", dev->name);
	return 0;
}

static DEVICE_API(counter, counter_silabs_driver_api) = {
	.start = counter_silabs_start,
	.stop = counter_silabs_stop,
	.get_value = counter_silabs_get_value,
	.set_alarm = counter_silabs_set_alarm,
	.cancel_alarm = counter_silabs_cancel_alarm,
	.set_top_value = counter_silabs_set_top_value,
	.get_pending_int = counter_silabs_get_pending_int,
	.get_top_value = counter_silabs_get_top_value,
	.get_freq = counter_silabs_get_freq,
};

void counter_silabs_isr_handler(const struct device *dev)
{
	struct counter_silabs_data *const burtc_data = dev->data;
	uint32_t flags = sl_hal_burtc_get_enabled_pending_interrupts();
#if defined(CONFIG_COUNTER_SILABS_BURTC_ALARM)
	counter_alarm_callback_t alarm_callback;
#endif

	sl_hal_burtc_clear_interrupts(flags);

#if defined(CONFIG_COUNTER_SILABS_BURTC_ALARM)
	if (flags & BURTC_IF_COMP && burtc_data->callback) {
		alarm_callback = burtc_data->callback;
		burtc_data->callback = NULL;
		alarm_callback(dev, 0, sl_hal_burtc_get_counter(), burtc_data->user_data);
	}
#else
	if (flags & BURTC_IF_COMP && burtc_data->top_callback) {
		burtc_data->top_callback(dev, burtc_data->top_user_data);
	}
#endif
}

#define BURTC_INIT(n)                                                                              \
	ISR_DIRECT_DECLARE(counter_silabs_isr_##n)                                                 \
	{                                                                                          \
		const struct device *const dev = DEVICE_DT_INST_GET(n);                            \
		counter_silabs_isr_handler(dev);                                                   \
		ISR_DIRECT_PM();                                                                   \
		return 1;                                                                          \
	}                                                                                          \
                                                                                                   \
	static void counter_silabs_irq_config_##n(void)                                            \
	{                                                                                          \
		IRQ_DIRECT_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),                      \
				   counter_silabs_isr_##n, n);                                     \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
                                                                                                   \
	static struct counter_silabs_data counter_silabs_data_##n;                                 \
                                                                                                   \
	static const struct counter_silabs_config counter_silabs_config_##n = {                    \
		.info =                                                                            \
			{                                                                          \
				.max_top_value = BURTC_MAX_VALUE,                                  \
				.flags = COUNTER_CONFIG_INFO_COUNT_UP,                             \
				.channels = BURTC_ALARM_NUM,                                       \
			},                                                                         \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_cfg = SILABS_DT_INST_CLOCK_CFG(n),                                          \
		.irq_config = counter_silabs_irq_config_##n,                                       \
		.prescaler = DT_INST_PROP(n, clock_div),                                           \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, counter_silabs_init, NULL, &counter_silabs_data_##n,              \
			      &counter_silabs_config_##n, PRE_KERNEL_1,                            \
			      CONFIG_COUNTER_INIT_PRIORITY, &counter_silabs_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BURTC_INIT)
