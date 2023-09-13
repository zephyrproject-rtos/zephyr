/*
 * Copyright (c) 2023 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <hardware/timer.h>

#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/irq.h>
#include <cmsis_core.h>

#define LOG_LEVEL CONFIG_COUNTER_LOG_LEVEL
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(counter_rpi_pico_timer, LOG_LEVEL);

#define DT_DRV_COMPAT raspberrypi_pico_timer

struct counter_rpi_pico_timer_ch_data {
	counter_alarm_callback_t callback;
	void *user_data;
};

struct counter_rpi_pico_timer_data {
	struct counter_rpi_pico_timer_ch_data *ch_data;
	uint32_t guard_period;
};

struct counter_rpi_pico_timer_config {
	struct counter_config_info info;
	timer_hw_t *timer;
	void (*irq_config)();
	const struct device *clk_dev;
	clock_control_subsys_t clk_id;
	const struct reset_dt_spec reset;
};

static int counter_rpi_pico_timer_start(const struct device *dev)
{
	const struct counter_rpi_pico_timer_config *config = dev->config;

	config->timer->pause = 0;

	return 0;
}

static int counter_rpi_pico_timer_stop(const struct device *dev)
{
	const struct counter_rpi_pico_timer_config *config = dev->config;

	config->timer->pause = 1u;
	config->timer->timelw = 0;
	config->timer->timehw = 0;

	return 0;
}

static uint32_t counter_rpi_pico_timer_get_top_value(const struct device *dev)
{
	const struct counter_rpi_pico_timer_config *config = dev->config;

	return config->info.max_top_value;
}

static int counter_rpi_pico_timer_get_value(const struct device *dev, uint32_t *ticks)
{
	*ticks = time_us_32();
	return 0;
}

static int counter_rpi_pico_timer_set_alarm(const struct device *dev, uint8_t id,
					    const struct counter_alarm_cfg *alarm_cfg)
{
	const struct counter_rpi_pico_timer_config *config = dev->config;
	struct counter_rpi_pico_timer_data *data = dev->data;
	struct counter_rpi_pico_timer_ch_data *chdata = &data->ch_data[id];
	uint64_t target = (alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) ? 0 : alarm_cfg->ticks;
	absolute_time_t alarm_at;
	bool missed;

	update_us_since_boot(&alarm_at, config->timer->timerawl + target);

	if (alarm_cfg->ticks > counter_rpi_pico_timer_get_top_value(dev)) {
		return -EINVAL;
	}

	if (chdata->callback) {
		return -EBUSY;
	}

	chdata->callback = alarm_cfg->callback;
	chdata->user_data = alarm_cfg->user_data;

	missed = hardware_alarm_set_target(id, alarm_at);

	if (missed) {
		if (alarm_cfg->flags & COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE) {
			hardware_alarm_force_irq(id);
		}
		chdata->callback = NULL;
		chdata->user_data = NULL;
		return -ETIME;
	}

	return 0;
}

static int counter_rpi_pico_timer_cancel_alarm(const struct device *dev, uint8_t id)
{
	struct counter_rpi_pico_timer_data *data = dev->data;
	struct counter_rpi_pico_timer_ch_data *chdata = &data->ch_data[id];

	chdata->callback = NULL;
	chdata->user_data = NULL;
	hardware_alarm_cancel(id);

	return 0;
}

static int counter_rpi_pico_timer_set_top_value(const struct device *dev,
						const struct counter_top_cfg *cfg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cfg);

	return -ENOTSUP;
}

static uint32_t counter_rpi_pico_timer_get_pending_int(const struct device *dev)
{
	return 0;
}

static uint32_t counter_rpi_pico_timer_get_guard_period(const struct device *dev, uint32_t flags)
{
	struct counter_rpi_pico_timer_data *data = dev->data;

	return data->guard_period;
}

static int counter_rpi_pico_timer_set_guard_period(const struct device *dev, uint32_t guard,
						   uint32_t flags)
{
	struct counter_rpi_pico_timer_data *data = dev->data;

	__ASSERT_NO_MSG(guard < counter_rpi_pico_timer_get_top_value(dev));

	data->guard_period = guard;

	return 0;
}

static void counter_rpi_pico_irq_handle(uint32_t ch, void *arg)
{
	struct device *dev = arg;
	struct counter_rpi_pico_timer_data *data = dev->data;
	counter_alarm_callback_t cb = data->ch_data[ch].callback;
	void *user_data = data->ch_data[ch].user_data;

	if (cb) {
		data->ch_data[ch].callback = NULL;
		data->ch_data[ch].user_data = NULL;
		cb(dev, ch, time_us_32(), user_data);
	}
}

static int counter_rpi_pico_timer_init(const struct device *dev)
{
	const struct counter_rpi_pico_timer_config *config = dev->config;
	int ret;

	ret = clock_control_on(config->clk_dev, config->clk_id);
	if (ret < 0) {
		return ret;
	}

	ret = reset_line_toggle_dt(&config->reset);
	if (ret < 0) {
		return ret;
	}

	config->irq_config();

	return 0;
}

static const struct counter_driver_api counter_rpi_pico_driver_api = {
	.start = counter_rpi_pico_timer_start,
	.stop = counter_rpi_pico_timer_stop,
	.get_value = counter_rpi_pico_timer_get_value,
	.set_alarm = counter_rpi_pico_timer_set_alarm,
	.cancel_alarm = counter_rpi_pico_timer_cancel_alarm,
	.set_top_value = counter_rpi_pico_timer_set_top_value,
	.get_pending_int = counter_rpi_pico_timer_get_pending_int,
	.get_top_value = counter_rpi_pico_timer_get_top_value,
	.get_guard_period = counter_rpi_pico_timer_get_guard_period,
	.set_guard_period = counter_rpi_pico_timer_set_guard_period,
};

#define RPI_PICO_TIMER_IRQ_ENABLE(node_id, name, idx)                                              \
	do {                                                                                       \
		hardware_alarm_set_callback(idx, counter_rpi_pico_irq_handle);                     \
		IRQ_CONNECT((DT_IRQ_BY_IDX(node_id, idx, irq)),                                    \
			    (DT_IRQ_BY_IDX(node_id, idx, priority)), hardware_alarm_irq_handler,   \
			    (DEVICE_DT_GET(node_id)), 0);                                          \
		irq_enable((DT_IRQ_BY_IDX(node_id, idx, irq)));                                    \
	} while (false);

#define COUNTER_RPI_PICO_TIMER(inst)                                                               \
	static void counter_irq_config##inst(void)                                                 \
	{                                                                                          \
		DT_INST_FOREACH_PROP_ELEM(inst, interrupt_names, RPI_PICO_TIMER_IRQ_ENABLE);       \
	}                                                                                          \
	static struct counter_rpi_pico_timer_ch_data                                               \
		ch_data##inst[DT_NUM_IRQS(DT_DRV_INST(inst))];                                     \
	static struct counter_rpi_pico_timer_data counter_##inst##_data = {                        \
		.ch_data = ch_data##inst,                                                          \
	};                                                                                         \
	static const struct counter_rpi_pico_timer_config counter_##inst##_config = {              \
		.timer = (timer_hw_t *)DT_INST_REG_ADDR(inst),                                     \
		.irq_config = counter_irq_config##inst,                                            \
		.info =                                                                            \
			{                                                                          \
				.max_top_value = UINT32_MAX,                                       \
				.freq = 1000000,                                                   \
				.flags = COUNTER_CONFIG_INFO_COUNT_UP,                             \
				.channels = ARRAY_SIZE(ch_data##inst),                             \
			},                                                                         \
		.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),                               \
		.clk_id = (clock_control_subsys_t)DT_INST_PHA_BY_IDX(inst, clocks, 0, clk_id),     \
		.reset = RESET_DT_SPEC_INST_GET(inst),                                             \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, counter_rpi_pico_timer_init, NULL, &counter_##inst##_data,     \
			      &counter_##inst##_config, PRE_KERNEL_1,                              \
			      CONFIG_COUNTER_INIT_PRIORITY, &counter_rpi_pico_driver_api);

DT_INST_FOREACH_STATUS_OKAY(COUNTER_RPI_PICO_TIMER)
