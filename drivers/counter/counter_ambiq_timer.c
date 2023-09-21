/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ambiq_counter

#include <zephyr/drivers/counter.h>
#include <zephyr/spinlock.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
/* ambiq-sdk includes */
#include <am_mcu_apollo.h>

LOG_MODULE_REGISTER(ambiq_counter, CONFIG_COUNTER_LOG_LEVEL);

static void counter_ambiq_isr(void *arg);

#define TIMER_IRQ (DT_INST_IRQN(0))

struct counter_ambiq_config {
	struct counter_config_info counter_info;
};

struct counter_ambiq_data {
	counter_alarm_callback_t callback;
	void *user_data;
};

static struct k_spinlock lock;

static int counter_ambiq_init(const struct device *dev)
{
	am_hal_timer_config_t tc;

	k_spinlock_key_t key = k_spin_lock(&lock);

	am_hal_timer_default_config_set(&tc);
	tc.eInputClock = AM_HAL_TIMER_CLOCK_HFRC_DIV16;
	tc.eFunction = AM_HAL_TIMER_FN_UPCOUNT;
	tc.ui32PatternLimit = 0;

	am_hal_timer_config(0, &tc);
	am_hal_timer_interrupt_enable(AM_HAL_TIMER_MASK(0, AM_HAL_TIMER_COMPARE1));

	k_spin_unlock(&lock, key);

	NVIC_ClearPendingIRQ(TIMER_IRQ);
	IRQ_CONNECT(TIMER_IRQ, 0, counter_ambiq_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(TIMER_IRQ);

	return 0;
}

static int counter_ambiq_start(const struct device *dev)
{
	k_spinlock_key_t key = k_spin_lock(&lock);

	am_hal_timer_start(0);

	k_spin_unlock(&lock, key);

	return 0;
}

static int counter_ambiq_stop(const struct device *dev)
{
	k_spinlock_key_t key = k_spin_lock(&lock);

	am_hal_timer_stop(0);

	k_spin_unlock(&lock, key);

	return 0;
}

static int counter_ambiq_get_value(const struct device *dev, uint32_t *ticks)
{
	k_spinlock_key_t key = k_spin_lock(&lock);

	*ticks = am_hal_timer_read(0);

	k_spin_unlock(&lock, key);

	return 0;
}

static int counter_ambiq_set_alarm(const struct device *dev, uint8_t chan_id,
				   const struct counter_alarm_cfg *alarm_cfg)
{
	ARG_UNUSED(chan_id);
	struct counter_ambiq_data *data = dev->data;
	uint32_t now;

	counter_ambiq_get_value(dev, &now);

	k_spinlock_key_t key = k_spin_lock(&lock);

	if ((alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) == 0) {
		am_hal_timer_compare1_set(0, now + alarm_cfg->ticks);
	} else {
		am_hal_timer_compare1_set(0, alarm_cfg->ticks);
	}

	data->user_data = alarm_cfg->user_data;
	data->callback = alarm_cfg->callback;

	k_spin_unlock(&lock, key);

	return 0;
}

static int counter_ambiq_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	ARG_UNUSED(chan_id);

	k_spinlock_key_t key = k_spin_lock(&lock);

	am_hal_timer_interrupt_disable(AM_HAL_TIMER_MASK(0, AM_HAL_TIMER_COMPARE1));
	k_spin_unlock(&lock, key);

	return 0;
}

static int counter_ambiq_set_top_value(const struct device *dev, const struct counter_top_cfg *cfg)
{
	const struct counter_ambiq_config *config = dev->config;

	if (cfg->ticks != config->counter_info.max_top_value) {
		return -ENOTSUP;
	} else {
		return 0;
	}
}

static uint32_t counter_ambiq_get_pending_int(const struct device *dev)
{
	return 0;
}

static uint32_t counter_ambiq_get_top_value(const struct device *dev)
{
	const struct counter_ambiq_config *config = dev->config;

	return config->counter_info.max_top_value;
}

static const struct counter_driver_api counter_api = {
	.start = counter_ambiq_start,
	.stop = counter_ambiq_stop,
	.get_value = counter_ambiq_get_value,
	.set_alarm = counter_ambiq_set_alarm,
	.cancel_alarm = counter_ambiq_cancel_alarm,
	.set_top_value = counter_ambiq_set_top_value,
	.get_pending_int = counter_ambiq_get_pending_int,
	.get_top_value = counter_ambiq_get_top_value,
};

static void counter_ambiq_isr(void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct counter_ambiq_data *data = dev->data;
	uint32_t now = 0;

	am_hal_timer_interrupt_clear(AM_HAL_TIMER_MASK(0, AM_HAL_TIMER_COMPARE1));
	counter_ambiq_get_value(dev, &now);

	if (data->callback) {
		data->callback(dev, 0, now, data->user_data);
	}
}

#define AMBIQ_COUNTER_INIT(idx)                                                                    \
                                                                                                   \
	static struct counter_ambiq_data counter_data_##idx;                                       \
                                                                                                   \
	static const struct counter_ambiq_config counter_config_##idx = {                          \
		.counter_info = {.max_top_value = UINT32_MAX,                                      \
				 .freq = 6000000,                                                  \
				 .flags = COUNTER_CONFIG_INFO_COUNT_UP,                            \
				 .channels = 1},                                                   \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(idx, counter_ambiq_init, NULL, &counter_data_##idx,                  \
			      &counter_config_##idx, PRE_KERNEL_1, CONFIG_COUNTER_INIT_PRIORITY,   \
			      &counter_api);

DT_INST_FOREACH_STATUS_OKAY(AMBIQ_COUNTER_INIT);
