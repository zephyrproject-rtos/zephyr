/*
 * Copyright (c) 2020 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_timer

#include <string.h>

#include <zephyr/drivers/counter.h>
#include <zephyr/spinlock.h>
#include <zephyr/kernel.h>
#ifndef CONFIG_SOC_ESP32C3
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#else
#include <zephyr/drivers/interrupt_controller/intc_esp32c3.h>
#endif
#include <zephyr/device.h>
#include <zephyr/logging/log.h>

#include <driver/periph_ctrl.h>
#include <hal/timer_types.h>
#include <hal/timer_hal.h>
#include <soc/periph_defs.h>
#include <soc/rtc_cntl_reg.h>
#include <soc/timer_group_reg.h>

LOG_MODULE_REGISTER(esp32_counter, CONFIG_COUNTER_LOG_LEVEL);

#ifdef CONFIG_SOC_ESP32C3
#define ISR_HANDLER isr_handler_t
#else
#define ISR_HANDLER intr_handler_t
#endif

static void counter_esp32_isr(void *arg);

typedef bool (*timer_isr_t)(void *);

struct timer_isr_func_t {
	timer_isr_t fn;
	void *args;
	struct intr_handle_data_t *timer_isr_handle;
	timer_group_t isr_timer_group;
};

struct counter_esp32_config {
	struct counter_config_info counter_info;
	timer_config_t config;
	timer_group_t group;
	timer_idx_t index;
	int irq_source;
};

struct counter_esp32_data {
	struct counter_alarm_cfg alarm_cfg;
	uint32_t ticks;
	timer_hal_context_t hal_ctx;
	struct timer_isr_func_t timer_isr_fun;
};

static struct k_spinlock lock;

static int counter_esp32_init(const struct device *dev)
{
	const struct counter_esp32_config *cfg = dev->config;
	struct counter_esp32_data *data = dev->data;

	switch (cfg->group) {
	case TIMER_GROUP_0:
		periph_module_enable(PERIPH_TIMG0_MODULE);
		break;
	case TIMER_GROUP_1:
		periph_module_enable(PERIPH_TIMG1_MODULE);
		break;
	default:
		return -ENOTSUP;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);

	timer_hal_init(&data->hal_ctx, cfg->group, cfg->index);
	data->alarm_cfg.callback = NULL;
	timer_hal_intr_disable(&data->hal_ctx);
	timer_hal_clear_intr_status(&data->hal_ctx);
	timer_hal_set_auto_reload(&data->hal_ctx, cfg->config.auto_reload);
	timer_hal_set_divider(&data->hal_ctx, cfg->config.divider);
	timer_hal_set_counter_increase(&data->hal_ctx, cfg->config.counter_dir);
	timer_hal_set_alarm_enable(&data->hal_ctx, cfg->config.alarm_en);
	if (cfg->config.intr_type == TIMER_INTR_LEVEL) {
		timer_hal_set_level_int_enable(&data->hal_ctx, true);
	}
	timer_hal_set_counter_value(&data->hal_ctx, 0);
	timer_hal_set_counter_enable(&data->hal_ctx, cfg->config.counter_en);
	esp_intr_alloc(cfg->irq_source,
			0,
			(ISR_HANDLER)counter_esp32_isr,
			(void *)dev,
			NULL);
	k_spin_unlock(&lock, key);

	return 0;
}

static int counter_esp32_start(const struct device *dev)
{
	struct counter_esp32_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&lock);

	timer_hal_set_counter_enable(&data->hal_ctx, TIMER_START);
	k_spin_unlock(&lock, key);

	return 0;
}

static int counter_esp32_stop(const struct device *dev)
{
	struct counter_esp32_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&lock);

	timer_hal_set_counter_enable(&data->hal_ctx, TIMER_PAUSE);
	k_spin_unlock(&lock, key);

	return 0;
}

static int counter_esp32_get_value(const struct device *dev, uint32_t *ticks)
{
	struct counter_esp32_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&lock);

	timer_hal_get_counter_value(&data->hal_ctx, (uint64_t *)ticks);
	k_spin_unlock(&lock, key);

	return 0;
}

static int counter_esp32_set_alarm(const struct device *dev, uint8_t chan_id,
				   const struct counter_alarm_cfg *alarm_cfg)
{
	ARG_UNUSED(chan_id);
	struct counter_esp32_data *data = dev->data;
	uint32_t now;

	counter_esp32_get_value(dev, &now);

	k_spinlock_key_t key = k_spin_lock(&lock);

	if ((alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) == 0) {
		timer_hal_set_alarm_value(&data->hal_ctx, (now + alarm_cfg->ticks));
	} else {
		timer_hal_set_alarm_value(&data->hal_ctx, alarm_cfg->ticks);
	}

	timer_hal_intr_enable(&data->hal_ctx);
	timer_hal_set_alarm_enable(&data->hal_ctx, TIMER_ALARM_EN);
	data->alarm_cfg.callback = alarm_cfg->callback;
	data->alarm_cfg.user_data = alarm_cfg->user_data;
	k_spin_unlock(&lock, key);

	return 0;
}

static int counter_esp32_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	ARG_UNUSED(chan_id);
	struct counter_esp32_data *data = dev->data;

	k_spinlock_key_t key = k_spin_lock(&lock);

	timer_hal_intr_disable(&data->hal_ctx);
	timer_hal_set_alarm_enable(&data->hal_ctx, TIMER_ALARM_DIS);
	k_spin_unlock(&lock, key);

	return 0;
}

static int counter_esp32_set_top_value(const struct device *dev,
				       const struct counter_top_cfg *cfg)
{
	const struct counter_esp32_config *config = dev->config;

	if (cfg->ticks != config->counter_info.max_top_value) {
		return -ENOTSUP;
	} else {
		return 0;
	}
}

static uint32_t counter_esp32_get_pending_int(const struct device *dev)
{
	struct counter_esp32_data *data = dev->data;

	timer_hal_get_intr_status_reg(&data->hal_ctx);

	return 0;
}

static uint32_t counter_esp32_get_top_value(const struct device *dev)
{
	const struct counter_esp32_config *config = dev->config;

	return config->counter_info.max_top_value;
}

static const struct counter_driver_api counter_api = {
	.start = counter_esp32_start,
	.stop = counter_esp32_stop,
	.get_value = counter_esp32_get_value,
	.set_alarm = counter_esp32_set_alarm,
	.cancel_alarm = counter_esp32_cancel_alarm,
	.set_top_value = counter_esp32_set_top_value,
	.get_pending_int = counter_esp32_get_pending_int,
	.get_top_value = counter_esp32_get_top_value,
};

static void counter_esp32_isr(void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct counter_esp32_data *data = dev->data;
	uint32_t now;

	counter_esp32_cancel_alarm(dev, 0);
	counter_esp32_get_value(dev, &now);

	if (data->alarm_cfg.callback) {
		data->alarm_cfg.callback(dev, 0, now, data->alarm_cfg.user_data);
	}

	timer_hal_clear_intr_status(&data->hal_ctx);
}

#define ESP32_COUNTER_GET_CLK_DIV(idx)						 \
	(((DT_INST_PROP(idx, prescaler) & UINT16_MAX) < 2) ?			 \
	2 : (DT_INST_PROP(idx, prescaler) & UINT16_MAX))

#define ESP32_COUNTER_INIT(idx)							 \
										 \
	static struct counter_esp32_data counter_data_##idx;			 \
										 \
	static const struct counter_esp32_config counter_config_##idx = {	 \
		.counter_info = {						 \
			.max_top_value = UINT32_MAX,				 \
			.freq = (APB_CLK_FREQ / ESP32_COUNTER_GET_CLK_DIV(idx)), \
			.flags = COUNTER_CONFIG_INFO_COUNT_UP,			 \
			.channels = 1						 \
		},								 \
		.config = {							 \
			.alarm_en = TIMER_ALARM_DIS,				 \
			.counter_en = TIMER_START,				 \
			.intr_type = TIMER_INTR_LEVEL,				 \
			.counter_dir = TIMER_COUNT_UP,				 \
			.auto_reload = TIMER_AUTORELOAD_DIS,			 \
			.divider = ESP32_COUNTER_GET_CLK_DIV(idx),		 \
		},								 \
		.group = DT_INST_PROP(idx, group),				 \
		.index = DT_INST_PROP(idx, index),				 \
		.irq_source = DT_INST_IRQN(idx),				 \
	};									 \
										 \
										 \
	DEVICE_DT_INST_DEFINE(idx,						 \
			      counter_esp32_init,				 \
			      NULL,						 \
			      &counter_data_##idx,				 \
			      &counter_config_##idx,				 \
			      PRE_KERNEL_1,					 \
			      CONFIG_COUNTER_INIT_PRIORITY,			 \
			      &counter_api);

DT_INST_FOREACH_STATUS_OKAY(ESP32_COUNTER_INIT);
