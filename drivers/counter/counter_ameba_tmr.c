/*
 * Copyright (c) 2026 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Realtek Ameba LP basic-timer counter driver.
 *
 * TIM0-3 expose a single auto-reload counter with one update IRQ and no
 * compare-match channel. The counter API's alarm and top therefore share
 * one ARR + one IRQ, with asymmetric preemption:
 *
 *   - set_alarm() (relative only; absolute is -ENOTSUP) reprograms ARR
 *     for a one-shot fire. If a top is configured, it is preempted; the
 *     ISR restores top's ARR after the alarm. -EBUSY if another alarm
 *     is armed.
 *
 *   - set_top_value() returns -EBUSY while an alarm is armed.
 *
 * Each alarm preemption shifts the top callback's phase, because the HAL
 * UG bit resets the counter on every ARR change.
 */

#define DT_DRV_COMPAT realtek_ameba_counter

/* Include <soc.h> before <ameba_soc.h> to avoid redefining unlikely() macro */
#include <soc.h>
#include <ameba_soc.h>

#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ameba_counter, CONFIG_COUNTER_LOG_LEVEL);

static void counter_ameba_isr(const struct device *dev);

struct counter_ameba_config {
	struct counter_config_info counter_info;
	RTIM_TypeDef *basic_timer;
	int irq_source;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	uint32_t clock_frequency;
	void (*irq_config_func)(const struct device *dev);
};

struct counter_ameba_data {
	counter_alarm_callback_t alarm_callback;
	void *alarm_user_data;
	counter_top_callback_t top_callback;
	void *top_user_data;
	uint32_t top_value;
	bool alarm_active;
	struct k_spinlock lock;
};

static int counter_ameba_init(const struct device *dev)
{
	const struct counter_ameba_config *cfg = dev->config;
	struct counter_ameba_data *data = dev->data;
	RTIM_TimeBaseInitTypeDef tim_init;
	int ret;

	data->top_value = cfg->counter_info.max_top_value;

	if (!cfg->clock_dev) {
		return -EINVAL;
	}

	if (!device_is_ready(cfg->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	ret = clock_control_on(cfg->clock_dev, cfg->clock_subsys);
	if (ret < 0) {
		LOG_ERR("Could not enable counter clock (%d)", ret);
		return ret;
	}

	cfg->irq_config_func(dev);

	/*
	 * Use the HAL only to program the timer base; ISR registration is
	 * handled by Zephyr's IRQ_CONNECT to avoid the HAL passing the
	 * stack-local tim_init pointer as user data to the ISR.
	 *
	 * The HAL default Period is 0xFFFF; override to the driver's full
	 * counter range so get_top_value() reflects the real ARR before any
	 * set_top_value() call.
	 */
	RTIM_TimeBaseStructInit(&tim_init);
	tim_init.TIM_Period = cfg->counter_info.max_top_value;
	RTIM_TimeBaseInit(cfg->basic_timer, &tim_init, cfg->irq_source, (IRQ_FUN)NULL,
			  (uint32_t)NULL);

	return 0;
}

static int counter_ameba_start(const struct device *dev)
{
	const struct counter_ameba_config *cfg = dev->config;

	RTIM_Cmd(cfg->basic_timer, ENABLE);
	return 0;
}

static int counter_ameba_stop(const struct device *dev)
{
	const struct counter_ameba_config *cfg = dev->config;

	RTIM_Cmd(cfg->basic_timer, DISABLE);
	return 0;
}

static int counter_ameba_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct counter_ameba_config *cfg = dev->config;

	*ticks = RTIM_GetCount(cfg->basic_timer);
	return 0;
}

static uint32_t counter_ameba_get_top_value(const struct device *dev)
{
	struct counter_ameba_data *data = dev->data;

	return data->top_value;
}

static int counter_ameba_set_alarm(const struct device *dev, uint8_t chan_id,
				   const struct counter_alarm_cfg *alarm_cfg)
{
	ARG_UNUSED(chan_id);
	struct counter_ameba_data *data = dev->data;
	const struct counter_ameba_config *cfg = dev->config;
	k_spinlock_key_t key;

	if (alarm_cfg->ticks == 0 || alarm_cfg->ticks > data->top_value) {
		return -EINVAL;
	}

	/*
	 * Absolute alarms cannot be implemented on this hardware: the LP basic
	 * timer has no compare-match channel, and any change to the auto-reload
	 * value forces the counter back to 0 (HAL UG semantics). Reject rather
	 * than silently degrade to a relative alarm.
	 */
	if (alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) {
		return -ENOTSUP;
	}

	key = k_spin_lock(&data->lock);

	if (data->alarm_active) {
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}

	data->alarm_callback = alarm_cfg->callback;
	data->alarm_user_data = alarm_cfg->user_data;
	data->alarm_active = true;

	RTIM_ChangePeriodImmediate(cfg->basic_timer, alarm_cfg->ticks);
	RTIM_INTConfig(cfg->basic_timer, TIM_IT_Update, ENABLE);

	k_spin_unlock(&data->lock, key);
	return 0;
}

static int counter_ameba_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	ARG_UNUSED(chan_id);
	struct counter_ameba_data *data = dev->data;
	const struct counter_ameba_config *cfg = dev->config;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);

	if (!data->alarm_active) {
		k_spin_unlock(&data->lock, key);
		return 0;
	}

	data->alarm_callback = NULL;
	data->alarm_user_data = NULL;
	data->alarm_active = false;

	if (data->top_callback != NULL) {
		/*
		 * Alarm had overwritten the period via ChangePeriodImmediate
		 * in set_alarm(); restore the top-driven period and keep the
		 * update IRQ enabled for the periodic top callback.
		 */
		RTIM_ChangePeriodImmediate(cfg->basic_timer, data->top_value);
	} else {
		RTIM_INTConfig(cfg->basic_timer, TIM_IT_Update, DISABLE);
	}

	k_spin_unlock(&data->lock, key);
	return 0;
}

static int counter_ameba_set_top_value(const struct device *dev,
				       const struct counter_top_cfg *top_cfg)
{
	const struct counter_ameba_config *cfg = dev->config;
	struct counter_ameba_data *data = dev->data;
	k_spinlock_key_t key;

	if (top_cfg->ticks == 0 || top_cfg->ticks > cfg->counter_info.max_top_value) {
		return -EINVAL;
	}

	/*
	 * RTIM_ChangePeriodImmediate forces UG which always resets the
	 * counter, so DONT_RESET cannot be honored. RESET_WHEN_LATE is
	 * implicitly satisfied (counter is always reset).
	 */
	if (top_cfg->flags & COUNTER_TOP_CFG_DONT_RESET) {
		return -ENOTSUP;
	}

	key = k_spin_lock(&data->lock);

	if (data->alarm_active) {
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}

	data->top_callback = top_cfg->callback;
	data->top_user_data = top_cfg->user_data;
	data->top_value = top_cfg->ticks;

	RTIM_ChangePeriodImmediate(cfg->basic_timer, top_cfg->ticks);
	if (top_cfg->callback != NULL) {
		RTIM_INTConfig(cfg->basic_timer, TIM_IT_Update, ENABLE);
	} else {
		RTIM_INTConfig(cfg->basic_timer, TIM_IT_Update, DISABLE);
	}

	k_spin_unlock(&data->lock, key);
	return 0;
}

static uint32_t counter_ameba_get_pending_int(const struct device *dev)
{
	const struct counter_ameba_config *cfg = dev->config;

	return RTIM_GetINTStatus(cfg->basic_timer, TIM_IT_Update);
}

static uint32_t counter_ameba_get_freq(const struct device *dev)
{
	const struct counter_ameba_config *cfg = dev->config;

	return cfg->clock_frequency;
}

static DEVICE_API(counter, counter_api) = {
	.start = counter_ameba_start,
	.stop = counter_ameba_stop,
	.get_value = counter_ameba_get_value,
	.set_alarm = counter_ameba_set_alarm,
	.cancel_alarm = counter_ameba_cancel_alarm,
	.set_top_value = counter_ameba_set_top_value,
	.get_pending_int = counter_ameba_get_pending_int,
	.get_top_value = counter_ameba_get_top_value,
	.get_freq = counter_ameba_get_freq,
};

static void counter_ameba_isr(const struct device *dev)
{
	struct counter_ameba_data *data = dev->data;
	const struct counter_ameba_config *cfg = dev->config;
	counter_alarm_callback_t alarm_cb = NULL;
	void *alarm_ud = NULL;
	counter_top_callback_t top_cb = NULL;
	void *top_ud = NULL;
	uint32_t now = 0;
	k_spinlock_key_t key;

	/* Guard against spurious or shared-vector calls. */
	if (!RTIM_GetINTStatus(cfg->basic_timer, TIM_IT_Update)) {
		return;
	}
	RTIM_INTClear(cfg->basic_timer);

	key = k_spin_lock(&data->lock);

	if (data->alarm_active) {
		alarm_cb = data->alarm_callback;
		alarm_ud = data->alarm_user_data;
		now = RTIM_GetCount(cfg->basic_timer);

		/* Alarm is single-shot per the counter API contract: clear the
		 * armed state before invoking the callback so the callback may
		 * safely re-arm a new alarm via set_alarm().
		 */
		data->alarm_callback = NULL;
		data->alarm_user_data = NULL;
		data->alarm_active = false;

		if (data->top_callback == NULL) {
			RTIM_INTConfig(cfg->basic_timer, TIM_IT_Update, DISABLE);
		}
	} else if (data->top_callback != NULL) {
		top_cb = data->top_callback;
		top_ud = data->top_user_data;
	}

	k_spin_unlock(&data->lock, key);

	if (alarm_cb != NULL) {
		alarm_cb(dev, 0, now, alarm_ud);

		/*
		 * If a top callback is configured and the alarm callback did
		 * not re-arm a new alarm, restore the top-driven period.
		 */
		key = k_spin_lock(&data->lock);
		if (!data->alarm_active && data->top_callback != NULL) {
			RTIM_ChangePeriodImmediate(cfg->basic_timer, data->top_value);
		}
		k_spin_unlock(&data->lock, key);
	} else if (top_cb != NULL) {
		top_cb(dev, top_ud);
	}
}

#define TIMER_IRQ_CONFIG(n)                                                                        \
	static void irq_config_##n(const struct device *dev)                                       \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), counter_ameba_isr,          \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}

#define AMEBA_COUNTER_INIT(n)                                                                      \
	TIMER_IRQ_CONFIG(n)                                                                        \
	static struct counter_ameba_data counter_data_##n;                                         \
                                                                                                   \
	static const struct counter_ameba_config counter_config_##n = {                            \
		.counter_info = {.max_top_value = UINT32_MAX,                                      \
				 .flags = COUNTER_CONFIG_INFO_COUNT_UP,                            \
				 .channels = 1},                                                   \
		.basic_timer = (RTIM_TypeDef *)DT_INST_REG_ADDR(n),                                \
		.irq_source = DT_INST_IRQN(n),                                                     \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, idx),               \
		.clock_frequency = DT_INST_PROP(n, clock_frequency),                               \
		.irq_config_func = irq_config_##n};                                                \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, counter_ameba_init, NULL, &counter_data_##n, &counter_config_##n, \
			      PRE_KERNEL_1, CONFIG_COUNTER_INIT_PRIORITY, &counter_api);

DT_INST_FOREACH_STATUS_OKAY(AMEBA_COUNTER_INIT);
