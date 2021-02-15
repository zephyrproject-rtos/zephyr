/*
 * Copyright (c) 2020 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_timer

/* Include esp-idf headers first to avoid redefining BIT() macro */
#include <soc/rtc_cntl_reg.h>
#include <soc/timer_group_reg.h>
#include <driver/periph_ctrl.h>
#include <soc/periph_defs.h>
#include <hal/timer_types.h>
#include <driver/timer.h>
#include <hal/timer_hal.h>
#include <soc.h>
#include <string.h>
#include <drivers/counter.h>
#include <device.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(esp32_counter, CONFIG_COUNTER_LOG_LEVEL);

#define INITIAL_COUNT (0x00000000ULL)

#define INTR_SRC_0 ETS_TG0_T0_LEVEL_INTR_SOURCE
#define INTR_SRC_1 ETS_TG0_T1_LEVEL_INTR_SOURCE
#define INTR_SRC_2 ETS_TG1_T0_LEVEL_INTR_SOURCE
#define INTR_SRC_3 ETS_TG1_T1_LEVEL_INTR_SOURCE

#define INST_0_INDEX TIMER_0
#define INST_1_INDEX TIMER_1
#define INST_2_INDEX TIMER_0
#define INST_3_INDEX TIMER_1

#define INST_0_GROUP TIMER_GROUP_0
#define INST_1_GROUP TIMER_GROUP_0
#define INST_2_GROUP TIMER_GROUP_1
#define INST_3_GROUP TIMER_GROUP_1

#define TIMX p_timer_obj[TIMG(dev)][TIDX(dev)]
#define DEV_CFG(dev) ((const struct counter_esp32_config *const)(dev)->config)
#define DEV_DATA(dev) ((struct counter_esp32_data *)(dev)->data)
#define TIMG(dev) (DEV_CFG(dev)->group)
#define TIDX(dev) (DEV_CFG(dev)->idx)

typedef void (*counter_irq_config_func_t)(const struct device *dev);

struct timer_isr_func_t {
	timer_isr_t fn;
	void *args;
	timer_isr_handle_t timer_isr_handle;
	timer_group_t isr_timer_group;
};

struct counter_obj_t {
	timer_hal_context_t hal;
	struct timer_isr_func_t timer_isr_fun;
};

struct counter_esp32_config {
	struct counter_config_info counter_info;
	timer_config_t config;
	timer_group_t group;
	timer_idx_t idx;

	const struct {
		int source;
		int line;
	} irq;

	counter_irq_config_func_t irq_config_fn;
};

struct counter_esp32_data {
	struct counter_alarm_cfg alarm_cfg;
	uint32_t ticks;
};

static struct counter_obj_t *p_timer_obj[TIMER_GROUP_MAX][TIMER_MAX] = { 0 };
static struct k_spinlock lock;

static int counter_esp32_init(const struct device *dev)
{
	const struct counter_esp32_config *cfg = DEV_CFG(dev);

	if (TIMG(dev) == TIMER_GROUP_0) {
		periph_module_enable(PERIPH_TIMG0_MODULE);
	} else if (TIMG(dev) == TIMER_GROUP_1) {
		periph_module_enable(PERIPH_TIMG1_MODULE);
	} else {
		return -ENOTSUP;
	}

	if (TIMX == NULL) {
		TIMX = (struct counter_obj_t *)k_calloc(1, sizeof(struct counter_obj_t));
		if (TIMX == NULL) {
			LOG_ERR("TIMER driver malloc error");
			return -ENOMEM;
		}
	}

	k_spinlock_key_t key = k_spin_lock(&lock);

	timer_hal_init(&TIMX->hal, TIMG(dev), TIDX(dev));
	DEV_DATA(dev)->alarm_cfg.callback = NULL;
	timer_hal_intr_disable(&TIMX->hal);
	timer_hal_clear_intr_status(&TIMX->hal);
	timer_hal_set_auto_reload(&TIMX->hal, cfg->config.auto_reload);
	timer_hal_set_divider(&TIMX->hal, cfg->config.divider);
	timer_hal_set_counter_increase(&TIMX->hal, cfg->config.counter_dir);
	timer_hal_set_alarm_enable(&TIMX->hal, cfg->config.alarm_en);
	if (cfg->config.intr_type == TIMER_INTR_LEVEL) {
		timer_hal_set_level_int_enable(&TIMX->hal, true);
	}
	timer_hal_set_counter_value(&TIMX->hal, INITIAL_COUNT);
	timer_hal_set_counter_enable(&TIMX->hal, cfg->config.counter_en);
	DEV_CFG(dev)->irq_config_fn(dev);
	k_spin_unlock(&lock, key);

	return 0;
}

static int counter_esp32_start(const struct device *dev)
{
	k_spinlock_key_t key = k_spin_lock(&lock);

	timer_hal_set_counter_enable(&TIMX->hal, TIMER_START);
	k_spin_unlock(&lock, key);

	return 0;
}

static int counter_esp32_stop(const struct device *dev)
{
	k_spinlock_key_t key = k_spin_lock(&lock);

	timer_hal_set_counter_enable(&TIMX->hal, TIMER_PAUSE);
	k_spin_unlock(&lock, key);

	return 0;
}

static int counter_esp32_get_value(const struct device *dev, uint32_t *ticks)
{
	k_spinlock_key_t key = k_spin_lock(&lock);

	timer_hal_get_counter_value(&TIMX->hal, (uint64_t *)ticks);
	k_spin_unlock(&lock, key);

	return 0;
}

static int counter_esp32_set_alarm(const struct device *dev, uint8_t chan_id,
				   const struct counter_alarm_cfg *alarm_cfg)
{
	ARG_UNUSED(chan_id);
	uint32_t now;

	counter_esp32_get_value(dev, &now);
	k_spinlock_key_t key = k_spin_lock(&lock);

	timer_hal_set_alarm_value(&TIMX->hal, (now + alarm_cfg->ticks));
	timer_hal_intr_enable(&TIMX->hal);
	timer_hal_set_alarm_enable(&TIMX->hal, TIMER_ALARM_EN);
	DEV_DATA(dev)->alarm_cfg.callback = alarm_cfg->callback;
	DEV_DATA(dev)->alarm_cfg.user_data = alarm_cfg->user_data;
	k_spin_unlock(&lock, key);

	return 0;
}

static int counter_esp32_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	ARG_UNUSED(chan_id);

	k_spinlock_key_t key = k_spin_lock(&lock);

	timer_hal_intr_disable(&TIMX->hal);
	timer_hal_set_alarm_enable(&TIMX->hal, TIMER_ALARM_DIS);
	k_spin_unlock(&lock, key);

	return 0;
}

static int counter_esp32_set_top_value(const struct device *dev,
				       const struct counter_top_cfg *cfg)
{
	if (cfg->ticks != (DEV_CFG(dev))->counter_info.max_top_value) {
		return -ENOTSUP;
	} else {
		return 0;
	}
}

static uint32_t counter_esp32_get_pending_int(const struct device *dev)
{
	timer_hal_get_intr_status_reg(&TIMX->hal);

	return 0;
}

static uint32_t counter_esp32_get_top_value(const struct device *dev)
{
	return DEV_CFG(dev)->counter_info.max_top_value;
}

static uint32_t counter_esp32_get_max_relative_alarm(const struct device *dev)
{
	return counter_esp32_get_top_value(dev);
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
	.get_max_relative_alarm = counter_esp32_get_max_relative_alarm
};

static void counter_esp32_isr(struct device *dev)
{
	counter_esp32_cancel_alarm(dev, 0);
	uint32_t now;

	counter_esp32_get_value(dev, &now);

	struct counter_alarm_cfg *alarm_cfg = &DEV_DATA(dev)->alarm_cfg;

	if (alarm_cfg->callback) {
		alarm_cfg->callback(dev, 0, now, alarm_cfg->user_data);
	}

	timer_hal_clear_intr_status(&TIMX->hal);
}

#define ESP32_COUNTER_INIT(n)							 \
										 \
	static struct counter_esp32_data counter_data_##n;			 \
	static void counter_esp32_irq_config_##n(const struct device *dev);	 \
										 \
	static const struct counter_esp32_config counter_config_##n = {		 \
		.counter_info = {						 \
			.max_top_value = UINT32_MAX,				 \
			.freq = (APB_CLK_FREQ / CONFIG_COUNTER_ESP32_PRESCALER), \
			.flags = COUNTER_CONFIG_INFO_COUNT_UP,			 \
			.channels = 1						 \
		},								 \
		.config = {							 \
			.alarm_en = TIMER_ALARM_DIS,				 \
			.counter_en = TIMER_START,				 \
			.intr_type = TIMER_INTR_LEVEL,				 \
			.counter_dir = TIMER_COUNT_UP,				 \
			.auto_reload = TIMER_AUTORELOAD_DIS,			 \
			.divider = CONFIG_COUNTER_ESP32_PRESCALER,		 \
		},								 \
		.group = INST_##n##_GROUP,					 \
		.idx = INST_##n##_INDEX,					 \
		.irq = {							 \
			.source =  INTR_SRC_##n,				 \
			.line =  CONFIG_COUNTER_ESP32_IRQ_##n,			 \
		},								 \
		.irq_config_fn = counter_esp32_irq_config_##n			 \
	};									 \
										 \
	static void counter_esp32_irq_config_##n(const struct device *dev)	 \
	{									 \
		intr_matrix_set(0, INTR_SRC_##n,				 \
				CONFIG_COUNTER_ESP32_IRQ_##n);			 \
		IRQ_CONNECT(CONFIG_COUNTER_ESP32_IRQ_##n,			 \
			    DT_INST_IRQ(n, priority), counter_esp32_isr,	 \
			    DEVICE_DT_INST_GET(n), 0);				 \
		irq_enable(CONFIG_COUNTER_ESP32_IRQ_##n);			 \
	}									 \
										 \
	DEVICE_DT_INST_DEFINE(n,						 \
			      counter_esp32_init,				 \
			      device_pm_control_nop, &counter_data_##n,		 \
			      &counter_config_##n, PRE_KERNEL_1,		 \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &counter_api);

DT_INST_FOREACH_STATUS_OKAY(ESP32_COUNTER_INIT)
