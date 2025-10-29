/*
 * Copyright (c) 2024 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc23x0_rtc

#include <zephyr/device.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/spinlock.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>

#include <inc/hw_rtc.h>
#include <inc/hw_types.h>
#include <inc/hw_evtsvt.h>
#include <inc/hw_memmap.h>

LOG_MODULE_REGISTER(cc23x0_counter_rtc, CONFIG_COUNTER_LOG_LEVEL);

static void counter_cc23x0_isr(const struct device *dev);

struct counter_cc23x0_config {
	struct counter_config_info counter_info;
	uint32_t base;
};

struct counter_cc23x0_data {
	struct counter_alarm_cfg alarm_cfg0;
};

static int counter_cc23x0_get_value(const struct device *dev, uint32_t *ticks)
{
	/* Resolution is 8us and max timeout ~9.5h */
	const struct counter_cc23x0_config *config = dev->config;

	*ticks = HWREG(config->base + RTC_O_TIME8U);

	return 0;
}

static int counter_cc23x0_get_value_64(const struct device *dev, uint64_t *ticks)
{
	const struct counter_cc23x0_config *config = dev->config;

	/*
	 * RTC counter register is 67 bits, only part of the bits are accessible.
	 * They are split in two partially overlapping registers:
	 * TIME524M [50:19]
	 * TIME8U   [34:3]
	 */

	uint64_t rtc_time_now =
		((HWREG(config->base + RTC_O_TIME524M) << 16) & 0xFFFFFFF800000000) |
		HWREG(config->base + RTC_O_TIME8U);

	*ticks = rtc_time_now;

	return 0;
}

static void counter_cc23x0_isr(const struct device *dev)
{
	const struct counter_cc23x0_config *config = dev->config;
	struct counter_cc23x0_data *data = dev->data;

	/* Clear RTC interrupt regs */
	HWREG(config->base + RTC_O_ICLR) = 0x3;
	HWREG(config->base + RTC_O_IMCLR) = 0x3;

	uint32_t now = HWREG(config->base + RTC_O_TIME8U);

	if (data->alarm_cfg0.callback) {
		data->alarm_cfg0.callback(dev, 0, now, data->alarm_cfg0.user_data);
	}
}

static int counter_cc23x0_set_alarm(const struct device *dev, uint8_t chan_id,
				    const struct counter_alarm_cfg *alarm_cfg)
{
	const struct counter_cc23x0_config *config = dev->config;
	struct counter_cc23x0_data *data = dev->data;

	/* RTC have resolutiuon of 8us */
	if (counter_ticks_to_us(dev, alarm_cfg->ticks) <= 8) {
		return -ENOTSUP;
	}

	uint32_t now = HWREG(config->base + RTC_O_TIME8U);

	/* Calculate next alarm relative to current time in us */
	uint32_t next_alarm = now + (counter_ticks_to_us(dev, alarm_cfg->ticks) / 8);

	HWREG(config->base + RTC_O_CH0CC8U) = next_alarm;
	HWREG(config->base + RTC_O_IMASK) = 0x1;
	HWREG(config->base + RTC_O_ARMSET) = 0x1;

	HWREG(EVTSVT_BASE + EVTSVT_O_CPUIRQ3SEL) = EVTSVT_CPUIRQ16SEL_PUBID_AON_RTC_COMB;

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), counter_cc23x0_isr,
		    DEVICE_DT_INST_GET(0), 0);

	irq_enable(DT_INST_IRQN(0));

	data->alarm_cfg0.flags = 0;
	data->alarm_cfg0.ticks = alarm_cfg->ticks;
	data->alarm_cfg0.callback = alarm_cfg->callback;
	data->alarm_cfg0.user_data = alarm_cfg->user_data;

	return 0;
}

static int counter_cc23x0_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	const struct counter_cc23x0_config *config = dev->config;

	/* Unset interrupt source */
	HWREG(EVTSVT_BASE + EVTSVT_O_CPUIRQ3SEL) = 0x0;

	/* Unarm both channels */
	HWREG(config->base + RTC_O_ARMCLR) = 0x3;

	return 0;
}

static int counter_cc23x0_set_top_value(const struct device *dev, const struct counter_top_cfg *cfg)
{
	return -ENOTSUP;
}

static uint32_t counter_cc23x0_get_pending_int(const struct device *dev)
{
	const struct counter_cc23x0_config *config = dev->config;
	struct counter_cc23x0_data *data = dev->data;

	/* Check interrupt and mask */
	if (HWREG(config->base + RTC_O_RIS) & HWREG(config->base + RTC_O_MIS)) {
		/* Clear RTC interrupt regs */
		HWREG(config->base + RTC_O_ICLR) = 0x3;
		HWREG(config->base + RTC_O_IMCLR) = 0x3;

		uint32_t now = HWREG(config->base + RTC_O_TIME8U);

		if (data->alarm_cfg0.callback) {
			data->alarm_cfg0.callback(dev, 0, now, data->alarm_cfg0.user_data);
		}

		return 0;
	}

	return -ESRCH;
}

#ifdef CONFIG_PM_DEVICE

static int rtc_cc23x0_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		return 0;
	case PM_DEVICE_ACTION_RESUME:
		counter_cc23x0_get_pending_int(dev);
		return 0;
	default:
		return -ENOTSUP;
	}
}

#endif /* CONFIG_PM_DEVICE */

static uint32_t counter_cc23x0_get_top_value(const struct device *dev)
{
	ARG_UNUSED(dev);

	return -ENOTSUP;
}

static uint32_t counter_cc23x0_get_freq(const struct device *dev)
{
	ARG_UNUSED(dev);

	/*
	 * From TRM clock for RTC is 24Mhz handled internally
	 * which is 1/2 from main 48Mhz clock = 24Mhz
	 * Accessible for user resolution is 8us per bit
	 * TIME8U [34:3] ~ 9.5h
	 */

	return (DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency) / 2);
}

static int counter_cc23x0_start(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* RTC timer runs after power-on reset */

	return -ENOTSUP;
}

static int counter_cc23x0_stop(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* Any reset/sleep mode, except for POR, will not stop or reset the RTC timer */

	return -ENOTSUP;
}

static int counter_cc23x0_init(const struct device *dev)
{
	const struct counter_cc23x0_config *config = dev->config;

	/* Clear interrupt Mask */
	HWREG(config->base + RTC_O_IMCLR) = 0x3;

	/* Clear Interrupt */
	HWREG(config->base + RTC_O_ICLR) = 0x3;

	/* Clear Armed */
	HWREG(config->base + RTC_O_ARMCLR) = 0x3;

	return 0;
}

static DEVICE_API(counter, rtc_cc23x0_api) = {
	.start = counter_cc23x0_start,
	.stop = counter_cc23x0_stop,
	.get_value = counter_cc23x0_get_value,
	.get_value_64 = counter_cc23x0_get_value_64,
	.set_alarm = counter_cc23x0_set_alarm,
	.cancel_alarm = counter_cc23x0_cancel_alarm,
	.get_top_value = counter_cc23x0_get_top_value,
	.set_top_value = counter_cc23x0_set_top_value,
	.get_pending_int = counter_cc23x0_get_pending_int,
	.get_freq = counter_cc23x0_get_freq,
};

#define CC23X0_INIT(inst)									\
	PM_DEVICE_DT_INST_DEFINE(inst, rtc_cc23x0_pm_action);					\
												\
	static const struct counter_cc23x0_config cc23x0_config_##inst = {			\
	.counter_info = {									\
		.max_top_value = UINT32_MAX,							\
		.flags = COUNTER_CONFIG_INFO_COUNT_UP,						\
		.channels = 1,									\
	},											\
		.base = DT_INST_REG_ADDR(inst),							\
	};											\
												\
	static struct counter_cc23x0_data cc23x0_data_##inst;					\
												\
	DEVICE_DT_INST_DEFINE(0, &counter_cc23x0_init, PM_DEVICE_DT_INST_GET(inst),		\
			      &cc23x0_data_##inst, &cc23x0_config_##inst, POST_KERNEL,		\
			      CONFIG_COUNTER_INIT_PRIORITY, &rtc_cc23x0_api);

DT_INST_FOREACH_STATUS_OKAY(CC23X0_INIT)
