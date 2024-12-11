/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_timer

#include <esp_clk_tree.h>
#include <driver/timer_types_legacy.h>
#include <hal/timer_hal.h>
#include <hal/timer_ll.h>

#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/kernel.h>
#if defined(CONFIG_RISCV)
#include <zephyr/drivers/interrupt_controller/intc_esp32c3.h>
#else
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#endif
#include <zephyr/device.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(esp32_counter, CONFIG_COUNTER_LOG_LEVEL);

#if defined(CONFIG_RISCV)
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
	const struct device *clock_dev;
	const clock_control_subsys_t clock_subsys;
	timer_group_t group;
	timer_idx_t index;
	int irq_source;
	int irq_priority;
	int irq_flags;
};

struct counter_esp32_data {
	struct counter_alarm_cfg alarm_cfg;
	uint32_t ticks;
	uint32_t clock_src_hz;
	timer_hal_context_t hal_ctx;
	struct timer_isr_func_t timer_isr_fun;
};

static int counter_esp32_init(const struct device *dev)
{
	const struct counter_esp32_config *cfg = dev->config;
	struct counter_esp32_data *data = dev->data;

	if (!device_is_ready(cfg->clock_dev)) {
		return -ENODEV;
	}

	/* Return value is not checked below, as clock might have been already enabled
	 * by another timer of the same group.
	 */
	clock_control_on(cfg->clock_dev, cfg->clock_subsys);

	timer_hal_init(&data->hal_ctx, cfg->group, cfg->index);
	data->alarm_cfg.callback = NULL;
	timer_ll_enable_intr(data->hal_ctx.dev, TIMER_LL_EVENT_ALARM(data->hal_ctx.timer_id),
			     false);
	timer_ll_clear_intr_status(data->hal_ctx.dev, TIMER_LL_EVENT_ALARM(data->hal_ctx.timer_id));
	timer_ll_enable_auto_reload(data->hal_ctx.dev, data->hal_ctx.timer_id,
				    cfg->config.auto_reload);
	timer_ll_set_clock_source(data->hal_ctx.dev, data->hal_ctx.timer_id,
				  GPTIMER_CLK_SRC_DEFAULT);
	timer_ll_set_clock_prescale(data->hal_ctx.dev, data->hal_ctx.timer_id, cfg->config.divider);
	timer_ll_set_count_direction(data->hal_ctx.dev, data->hal_ctx.timer_id,
				     cfg->config.counter_dir);
	timer_ll_enable_alarm(data->hal_ctx.dev, data->hal_ctx.timer_id, cfg->config.alarm_en);
	timer_ll_set_reload_value(data->hal_ctx.dev, data->hal_ctx.timer_id, 0);
	timer_ll_enable_counter(data->hal_ctx.dev, data->hal_ctx.timer_id, cfg->config.counter_en);

	esp_clk_tree_src_get_freq_hz(GPTIMER_CLK_SRC_DEFAULT,
				     ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED, &data->clock_src_hz);

	int ret = esp_intr_alloc(cfg->irq_source,
				 ESP_PRIO_TO_FLAGS(cfg->irq_priority) |
					 ESP_INT_FLAGS_CHECK(cfg->irq_flags),
				 (ISR_HANDLER)counter_esp32_isr, (void *)dev, NULL);

	if (ret != 0) {
		LOG_ERR("could not allocate interrupt (err %d)", ret);
	}

	return ret;
}

static int counter_esp32_start(const struct device *dev)
{
	struct counter_esp32_data *data = dev->data;

	timer_ll_enable_counter(data->hal_ctx.dev, data->hal_ctx.timer_id, TIMER_START);

	return 0;
}

static int counter_esp32_stop(const struct device *dev)
{
	struct counter_esp32_data *data = dev->data;

	timer_ll_enable_counter(data->hal_ctx.dev, data->hal_ctx.timer_id, TIMER_PAUSE);

	return 0;
}

static int counter_esp32_get_value(const struct device *dev, uint32_t *ticks)
{
	struct counter_esp32_data *data = dev->data;

	timer_ll_trigger_soft_capture(data->hal_ctx.dev, data->hal_ctx.timer_id);
	*ticks = (uint32_t)timer_ll_get_counter_value(data->hal_ctx.dev, data->hal_ctx.timer_id);

	return 0;
}

static int counter_esp32_get_value_64(const struct device *dev, uint64_t *ticks)
{
	struct counter_esp32_data *data = dev->data;

	timer_ll_trigger_soft_capture(data->hal_ctx.dev, data->hal_ctx.timer_id);
	*ticks = timer_ll_get_counter_value(data->hal_ctx.dev, data->hal_ctx.timer_id);

	return 0;
}

static int counter_esp32_set_alarm(const struct device *dev, uint8_t chan_id,
				   const struct counter_alarm_cfg *alarm_cfg)
{
	ARG_UNUSED(chan_id);
	struct counter_esp32_data *data = dev->data;
	uint32_t now;

	counter_esp32_get_value(dev, &now);

	if ((alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) == 0) {
		timer_ll_set_alarm_value(data->hal_ctx.dev, data->hal_ctx.timer_id,
					 (now + alarm_cfg->ticks));
	} else {
		timer_ll_set_alarm_value(data->hal_ctx.dev, data->hal_ctx.timer_id,
					 alarm_cfg->ticks);
	}

	timer_ll_enable_intr(data->hal_ctx.dev, TIMER_LL_EVENT_ALARM(data->hal_ctx.timer_id), true);
	timer_ll_enable_alarm(data->hal_ctx.dev, data->hal_ctx.timer_id, TIMER_ALARM_EN);
	data->alarm_cfg.callback = alarm_cfg->callback;
	data->alarm_cfg.user_data = alarm_cfg->user_data;

	return 0;
}

static int counter_esp32_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	ARG_UNUSED(chan_id);
	struct counter_esp32_data *data = dev->data;

	timer_ll_enable_intr(data->hal_ctx.dev, TIMER_LL_EVENT_ALARM(data->hal_ctx.timer_id),
			     false);
	timer_ll_enable_alarm(data->hal_ctx.dev, data->hal_ctx.timer_id, TIMER_ALARM_DIS);

	return 0;
}

static int counter_esp32_set_top_value(const struct device *dev, const struct counter_top_cfg *cfg)
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

	return timer_ll_get_intr_status(data->hal_ctx.dev);
}

static uint32_t counter_esp32_get_top_value(const struct device *dev)
{
	const struct counter_esp32_config *config = dev->config;

	return config->counter_info.max_top_value;
}

uint32_t counter_esp32_get_freq(const struct device *dev)
{
	const struct counter_esp32_config *config = dev->config;
	struct counter_esp32_data *data = dev->data;

	return data->clock_src_hz / config->config.divider;
}

static DEVICE_API(counter, counter_api) = {
	.start = counter_esp32_start,
	.stop = counter_esp32_stop,
	.get_value = counter_esp32_get_value,
	.get_value_64 = counter_esp32_get_value_64,
	.set_alarm = counter_esp32_set_alarm,
	.cancel_alarm = counter_esp32_cancel_alarm,
	.set_top_value = counter_esp32_set_top_value,
	.get_pending_int = counter_esp32_get_pending_int,
	.get_top_value = counter_esp32_get_top_value,
	.get_freq = counter_esp32_get_freq,
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

	timer_ll_clear_intr_status(data->hal_ctx.dev, TIMER_LL_EVENT_ALARM(data->hal_ctx.timer_id));
}

#define ESP32_COUNTER_GET_CLK_DIV(idx)                                                             \
	(((DT_INST_PROP(idx, prescaler) & UINT16_MAX) < 2)                                         \
		 ? 2                                                                               \
		 : (DT_INST_PROP(idx, prescaler) & UINT16_MAX))

#define ESP32_COUNTER_INIT(idx)                                                                    \
                                                                                                   \
	static struct counter_esp32_data counter_data_##idx;                                       \
                                                                                                   \
	static const struct counter_esp32_config counter_config_##idx = {                          \
		.counter_info = {.max_top_value = UINT32_MAX,                                      \
				 .flags = COUNTER_CONFIG_INFO_COUNT_UP,                            \
				 .channels = 1},                                                   \
		.config =                                                                          \
			{                                                                          \
				.alarm_en = TIMER_ALARM_DIS,                                       \
				.counter_en = TIMER_START,                                         \
				.intr_type = TIMER_INTR_LEVEL,                                     \
				.counter_dir = TIMER_COUNT_UP,                                     \
				.auto_reload = TIMER_AUTORELOAD_DIS,                               \
				.divider = ESP32_COUNTER_GET_CLK_DIV(idx),                         \
			},                                                                         \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(idx)),                              \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(idx, offset),          \
		.group = DT_INST_PROP(idx, group),                                                 \
		.index = DT_INST_PROP(idx, index),                                                 \
		.irq_source = DT_INST_IRQ_BY_IDX(idx, 0, irq),                                     \
		.irq_priority = DT_INST_IRQ_BY_IDX(idx, 0, priority),                              \
		.irq_flags = DT_INST_IRQ_BY_IDX(idx, 0, flags)};                                   \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(idx, counter_esp32_init, NULL, &counter_data_##idx,                  \
			      &counter_config_##idx, PRE_KERNEL_1, CONFIG_COUNTER_INIT_PRIORITY,   \
			      &counter_api);

DT_INST_FOREACH_STATUS_OKAY(ESP32_COUNTER_INIT);
