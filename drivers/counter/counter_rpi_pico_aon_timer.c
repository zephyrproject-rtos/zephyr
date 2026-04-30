/*
 * Copyright (c) 2025-2026 Dmitrii Sharshakov
 *
 * Based on counter_rpi_pico_timer.c, which is:
 * Copyright (c) 2023 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <hardware/powman.h>

#include <zephyr/sys/barrier.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/irq.h>

#define LOG_LEVEL CONFIG_COUNTER_LOG_LEVEL
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(counter_rpi_pico_aon_timer, LOG_LEVEL);

#define DT_DRV_COMPAT raspberrypi_pico_aon_timer

struct counter_rpi_pico_aon_timer_data {
	/*
	 * When 32-bit callback is used, callback is set to a trampoline
	 * which calls alarm_callback_32
	 */
	counter_alarm_callback_64_t callback;
	counter_alarm_callback_t alarm_callback_32;
	void *user_data;
	uint64_t guard_period;
};

struct counter_rpi_pico_aon_timer_config {
	struct counter_config_info info;
	void (*config_irq)();
	void (*enable_irq)();
	void (*disable_irq)();
};

static int counter_rpi_pico_aon_start(const struct device *dev)
{
	powman_timer_start();

	return 0;
}

static int counter_rpi_pico_aon_stop(const struct device *dev)
{
	powman_timer_stop();

	return 0;
}

static uint32_t counter_rpi_pico_aon_get_top_value(const struct device *dev)
{
	const struct counter_rpi_pico_aon_timer_config *config = dev->config;

	return config->info.max_top_value;
}

static uint64_t counter_rpi_pico_aon_get_top_value_64(const struct device *dev)
{
	const struct counter_rpi_pico_aon_timer_config *config = dev->config;

	return config->info.max_top_value_64;
}

static int counter_rpi_pico_aon_get_value(const struct device *dev, uint32_t *ticks)
{
	*ticks = powman_timer_get_ms();
	return 0;
}

static int counter_rpi_pico_aon_get_value_64(const struct device *dev, uint64_t *ticks)
{
	*ticks = powman_timer_get_ms();
	return 0;
}

static int counter_rpi_pico_aon_set_alarm_64(const struct device *dev, uint8_t id,
					    const struct counter_alarm_cfg_64 *alarm_cfg)
{
	struct counter_rpi_pico_aon_timer_data *data = dev->data;
	const struct counter_rpi_pico_aon_timer_config *config = dev->config;
	uint64_t current_time = powman_timer_get_ms();
	uint64_t target = (alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) ?
		alarm_cfg->ticks :
		current_time + alarm_cfg->ticks;
	uint32_t key;
	int ret = 0;

	if (alarm_cfg->ticks > counter_rpi_pico_aon_get_top_value_64(dev)) {
		return -EINVAL;
	}

	if (data->callback) {
		return -EBUSY;
	}

	data->callback = alarm_cfg->callback;
	data->user_data = alarm_cfg->user_data;

	if (target + data->guard_period <= current_time &&
		(alarm_cfg->flags & COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE) == 0) {
		data->callback = NULL;
		data->user_data = NULL;
		ret = -ETIME;
	}

	barrier_dsync_fence_full();

	key = irq_lock();
	powman_enable_alarm_wakeup_at_ms(target);
	config->enable_irq();
	irq_unlock(key);

	return ret;
}

static void counter_aon_callback_32_trampoline(const struct device *dev,
						 uint8_t chan_id,
						 uint64_t ticks,
						 void *user_data)
{
	struct counter_rpi_pico_aon_timer_data *data = dev->data;

	/* Safely call the original 32-bit callback */
	data->alarm_callback_32(dev, chan_id, (uint32_t)ticks, user_data);
}

static int counter_rpi_pico_aon_set_alarm(const struct device *dev, uint8_t id,
						const struct counter_alarm_cfg *alarm_cfg)
{
	struct counter_rpi_pico_aon_timer_data *data = dev->data;
	struct counter_alarm_cfg_64 alarm_cfg_64 = {
		.callback = counter_aon_callback_32_trampoline,
		.ticks = (uint64_t)alarm_cfg->ticks,
		.user_data = alarm_cfg->user_data,
		.flags = alarm_cfg->flags,
	};

	/* use trampoline function to handle the function pointer type difference */
	data->alarm_callback_32 = alarm_cfg->callback;

	return counter_rpi_pico_aon_set_alarm_64(dev, id, &alarm_cfg_64);
}

static int counter_rpi_pico_aon_cancel_alarm(const struct device *dev, uint8_t id)
{
	struct counter_rpi_pico_aon_timer_data *data = dev->data;
	const struct counter_rpi_pico_aon_timer_config *config = dev->config;

	config->disable_irq();
	barrier_dsync_fence_full();
	data->callback = NULL;
	data->user_data = NULL;
	powman_timer_disable_alarm();

	return 0;
}

static int counter_rpi_pico_aon_set_top_value(const struct device *dev,
						const struct counter_top_cfg *cfg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cfg);

	return -ENOTSUP;
}

static int counter_rpi_pico_aon_set_top_value_64(const struct device *dev,
						const struct counter_top_cfg_64 *cfg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cfg);

	return -ENOTSUP;
}

static uint32_t counter_rpi_pico_aon_get_pending_int(const struct device *dev)
{
	return 0;
}

static uint32_t counter_rpi_pico_aon_get_guard_period(const struct device *dev, uint32_t flags)
{
	struct counter_rpi_pico_aon_timer_data *data = dev->data;

	return data->guard_period;
}

static int counter_rpi_pico_aon_set_guard_period(const struct device *dev, uint32_t guard,
						   uint32_t flags)
{
	struct counter_rpi_pico_aon_timer_data *data = dev->data;

	__ASSERT_NO_MSG(guard < counter_rpi_pico_aon_get_top_value(dev));

	data->guard_period = guard;

	return 0;
}

static uint64_t counter_rpi_pico_aon_get_guard_period_64(const struct device *dev, uint32_t flags)
{
	struct counter_rpi_pico_aon_timer_data *data = dev->data;

	return data->guard_period;
}

static int counter_rpi_pico_aon_set_guard_period_64(const struct device *dev, uint64_t guard,
						   uint32_t flags)
{
	struct counter_rpi_pico_aon_timer_data *data = dev->data;

	__ASSERT_NO_MSG(guard < counter_rpi_pico_aon_get_top_value_64(dev));

	data->guard_period = guard;

	return 0;
}

static void counter_rpi_pico_aon_irq_handle(void *arg)
{
	struct device *dev = arg;
	struct counter_rpi_pico_aon_timer_data *data = dev->data;
	const struct counter_rpi_pico_aon_timer_config *config = dev->config;
	counter_alarm_callback_64_t cb = data->callback;
	void *user_data = data->user_data;

	config->disable_irq();
	barrier_dsync_fence_full();

	if (cb) {
		data->callback = NULL;
		data->user_data = NULL;
		cb(dev, 0, powman_timer_get_ms(), user_data);
	}
}

static int counter_rpi_pico_aon_timer_init(const struct device *dev)
{
	const struct counter_rpi_pico_aon_timer_config *config = dev->config;
	config->config_irq();

	/* TODO: allow running off XOSC, for purposes other than wakeup source
	 * TODO: apply LPOSC frequency from OTP, potentially recalibrate when restarting
	 */
	powman_timer_set_1khz_tick_source_lposc();

	return 0;
}

static DEVICE_API(counter, counter_rpi_pico_driver_api) = {
	.start = counter_rpi_pico_aon_start,
	.stop = counter_rpi_pico_aon_stop,
	.get_value = counter_rpi_pico_aon_get_value,
	.set_alarm = counter_rpi_pico_aon_set_alarm,
	.cancel_alarm = counter_rpi_pico_aon_cancel_alarm,
	.set_top_value = counter_rpi_pico_aon_set_top_value,
	.get_pending_int = counter_rpi_pico_aon_get_pending_int,
	.get_top_value = counter_rpi_pico_aon_get_top_value,
	.get_guard_period = counter_rpi_pico_aon_get_guard_period,
	.set_guard_period = counter_rpi_pico_aon_set_guard_period,
#ifdef CONFIG_COUNTER_64BITS_TICKS
	.get_value_64 = counter_rpi_pico_aon_get_value_64,
	.set_alarm_64 = counter_rpi_pico_aon_set_alarm_64,
	.set_top_value_64 = counter_rpi_pico_aon_set_top_value_64,
	.get_top_value_64 = counter_rpi_pico_aon_get_top_value_64,
	.get_guard_period_64 = counter_rpi_pico_aon_get_guard_period_64,
	.set_guard_period_64 = counter_rpi_pico_aon_set_guard_period_64,
#endif /* CONFIG_COUNTER_64BITS_TICKS */
};

#define RPI_PICO_AON_TIMER_IRQ_CONNECT(node_id, name, idx)                                         \
	do {                                                                                       \
		IRQ_CONNECT((DT_IRQ_BY_IDX(node_id, idx, irq)),                                    \
			    (DT_IRQ_BY_IDX(node_id, idx, priority)),                               \
			    counter_rpi_pico_aon_irq_handle,                                       \
			    (DEVICE_DT_GET(node_id)), 0);                                          \
	} while (false);

#define RPI_PICO_AON_TIMER_IRQ_ENABLE(node_id, name, idx)                                          \
	do {                                                                                       \
		irq_enable((DT_IRQ_BY_IDX(node_id, idx, irq)));                                    \
	} while (false);

#define RPI_PICO_AON_TIMER_IRQ_DISABLE(node_id, name, idx)                                         \
	do {                                                                                       \
		irq_disable((DT_IRQ_BY_IDX(node_id, idx, irq)));                                   \
	} while (false);

#define COUNTER_RPI_PICO_AON_TIMER(inst)                                                           \
	static void counter_irq_config##inst(void)                                                 \
	{                                                                                          \
		DT_INST_FOREACH_PROP_ELEM(inst, interrupt_names, RPI_PICO_AON_TIMER_IRQ_CONNECT);  \
	}                                                                                          \
	static void counter_irq_enable##inst(void)                                                 \
	{                                                                                          \
		DT_INST_FOREACH_PROP_ELEM(inst, interrupt_names, RPI_PICO_AON_TIMER_IRQ_ENABLE);   \
	}                                                                                          \
	static void counter_irq_disable##inst(void)                                                \
	{                                                                                          \
		DT_INST_FOREACH_PROP_ELEM(inst, interrupt_names, RPI_PICO_AON_TIMER_IRQ_DISABLE);  \
	}                                                                                          \
	static struct counter_rpi_pico_aon_timer_data counter_##inst##_data = {                    \
		.callback = NULL,                                                                  \
		.user_data = NULL,                                                                 \
	};                                                                                         \
	static const struct counter_rpi_pico_aon_timer_config counter_##inst##_config = {          \
		.config_irq = counter_irq_config##inst,                                            \
		.enable_irq = counter_irq_enable##inst,                                            \
		.disable_irq = counter_irq_disable##inst,                                          \
		.info = {																		\
			COND_CODE_1(CONFIG_COUNTER_64BITS_TICKS,                                  \
			(.max_top_value_64 = UINT64_MAX,),())                                          \
			.max_top_value = UINT32_MAX, \
			.freq = 1000,                                                              \
			.flags = COUNTER_CONFIG_INFO_COUNT_UP,                                     \
			.channels = 1},                                                            \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, counter_rpi_pico_aon_timer_init, NULL, &counter_##inst##_data, \
			      &counter_##inst##_config, PRE_KERNEL_1,                              \
			      CONFIG_COUNTER_INIT_PRIORITY, &counter_rpi_pico_driver_api);

DT_INST_FOREACH_STATUS_OKAY(COUNTER_RPI_PICO_AON_TIMER)
