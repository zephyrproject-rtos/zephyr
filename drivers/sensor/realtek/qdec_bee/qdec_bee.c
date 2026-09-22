/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/bee_clock_control.h>
#include <zephyr/drivers/sensor/qdec_bee.h>
#include <soc.h>
#include <zephyr/irq.h>

#include "bee_qdec_common.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(qdec_bee, CONFIG_SENSOR_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(realtek_bee_basic_qdec)
#define DT_DRV_COMPAT   realtek_bee_basic_qdec
#define QDEC_AXIS_COUNT BEE_QDEC_AXIS_MAX
#define USE_BASIC_QDEC  1
#elif DT_HAS_COMPAT_STATUS_OKAY(realtek_bee_aon_qdec)
#define DT_DRV_COMPAT   realtek_bee_aon_qdec
#define QDEC_AXIS_COUNT 1
#else
#error "No enabled QDEC node found in Device Tree"
#endif

struct qdec_bee_axis_data {
	int32_t acc;
	int32_t round;
	sensor_trigger_handler_t data_ready_handler;
	const struct sensor_trigger *data_ready_trigger;
};

struct qdec_bee_data {
	struct qdec_bee_axis_data axes[QDEC_AXIS_COUNT];
	const struct bee_qdec_ops *ops;
};

struct qdec_bee_config {
	uint32_t reg;
	const struct pinctrl_dev_config *pcfg;
	void (*irq_connect)(void);
	struct bee_qdec_axis_config axis_cfgs[BEE_QDEC_AXIS_MAX];
#ifdef USE_BASIC_QDEC
	uint16_t clkid;
#endif
};

static int qdec_bee_get_axis_idx(const struct qdec_bee_config *config, enum sensor_channel chan)
{
	if (chan == SENSOR_CHAN_ROTATION) {
		for (int i = 0; i < QDEC_AXIS_COUNT; i++) {
			if (config->axis_cfgs[i].enable) {
				return i;
			}
		}
		return -ENODEV;
	}

	if (chan >= (enum sensor_channel)SENSOR_CHAN_QDEC_X_COUNT &&
	    chan <= (enum sensor_channel)SENSOR_CHAN_QDEC_Z_COUNT) {
		int axis_idx = chan - (enum sensor_channel)SENSOR_CHAN_QDEC_X_COUNT;

		if (axis_idx >= 0 && axis_idx < QDEC_AXIS_COUNT &&
		    config->axis_cfgs[axis_idx].enable) {
			return axis_idx;
		}
	}

	return -ENOTSUP;
}

static int qdec_bee_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct qdec_bee_config *config = dev->config;
	struct qdec_bee_data *data = dev->data;
	const struct bee_qdec_ops *ops = data->ops;
	unsigned int key;
	int i;

	key = irq_lock();

	if (chan == SENSOR_CHAN_ALL) {
		for (i = 0; i < QDEC_AXIS_COUNT; i++) {
			if (config->axis_cfgs[i].enable) {
				data->axes[i].acc = (data->axes[i].round << 16) +
						    ops->get_count(config->reg, i);
			}
		}
	} else {
		i = qdec_bee_get_axis_idx(config, chan);
		if (i < 0) {
			irq_unlock(key);
			return i;
		}
		data->axes[i].acc = (data->axes[i].round << 16) + ops->get_count(config->reg, i);
	}

	irq_unlock(key);

	return 0;
}

static int qdec_bee_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	const struct qdec_bee_config *config = dev->config;
	struct qdec_bee_data *data = dev->data;
	int axis_idx = -1;

	axis_idx = qdec_bee_get_axis_idx(config, chan);

	if (axis_idx < 0) {
		return -ENOTSUP;
	}

	val->val1 = data->axes[axis_idx].acc;
	val->val2 = 0;

	return 0;
}

static int qdec_bee_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
				sensor_trigger_handler_t handler)
{
	const struct qdec_bee_config *config = dev->config;
	struct qdec_bee_data *data = dev->data;
	const struct bee_qdec_ops *ops = data->ops;
	unsigned int key;
	int axis_idx;

	if (trig->type != SENSOR_TRIG_DATA_READY) {
		return -ENOTSUP;
	}

	axis_idx = qdec_bee_get_axis_idx(config, trig->chan);

	if (axis_idx < 0) {
		return -ENOTSUP;
	}

	key = irq_lock();

	data->axes[axis_idx].data_ready_handler = handler;
	data->axes[axis_idx].data_ready_trigger = trig;
	if (handler) {
		ops->int_enable(config->reg, axis_idx, BEE_QDEC_EVENT_NEW_DATA);
		ops->int_enable(config->reg, axis_idx, BEE_QDEC_EVENT_ILLEGAL);
	} else {
		ops->int_disable(config->reg, axis_idx, BEE_QDEC_EVENT_NEW_DATA);
		ops->int_disable(config->reg, axis_idx, BEE_QDEC_EVENT_ILLEGAL);
	}

	irq_unlock(key);

	return 0;
}

static void qdec_bee_isr_handle_axis(const struct device *dev, int axis_idx)
{
	struct qdec_bee_data *data = dev->data;
	const struct qdec_bee_config *config = dev->config;
	const struct bee_qdec_ops *ops = data->ops;
	uint32_t reg = config->reg;
	sensor_trigger_handler_t handler;

	if (!config->axis_cfgs[axis_idx].enable) {
		return;
	}

	if (ops->get_status(reg, axis_idx, BEE_QDEC_EVENT_ILLEGAL)) {
		ops->int_clear(reg, axis_idx, BEE_QDEC_EVENT_ILLEGAL);
		const char *axis_name = (axis_idx == 0) ? "X" : (axis_idx == 1) ? "Y" : "Z";

		LOG_ERR("%s axis qdec illegal status", axis_name);
	}

	if (ops->get_status(reg, axis_idx, BEE_QDEC_EVENT_NEW_DATA)) {
		if (ops->get_status(reg, axis_idx, BEE_QDEC_EVENT_OVERFLOW)) {
			data->axes[axis_idx].round += 1;
			ops->int_clear(reg, axis_idx, BEE_QDEC_EVENT_OVERFLOW);
		}
		if (ops->get_status(reg, axis_idx, BEE_QDEC_EVENT_UNDERFLOW)) {
			data->axes[axis_idx].round -= 1;
			ops->int_clear(reg, axis_idx, BEE_QDEC_EVENT_UNDERFLOW);
		}

		ops->int_clear(reg, axis_idx, BEE_QDEC_EVENT_NEW_DATA);

		handler = data->axes[axis_idx].data_ready_handler;

		if (handler) {
			handler(dev, data->axes[axis_idx].data_ready_trigger);
		}
	}
}

static void qdec_bee_isr(void *arg)
{
	const struct device *dev = (const struct device *)arg;

	for (int i = 0; i < QDEC_AXIS_COUNT; i++) {
		qdec_bee_isr_handle_axis(dev, i);
	}
}

static int qdec_bee_init(const struct device *dev)
{
	struct qdec_bee_data *data = dev->data;
	const struct qdec_bee_config *config = dev->config;
	int ret;

	data->ops = bee_qdec_get_ops();
	if (!data->ops) {
		LOG_ERR("Bee QDEC HAL not available");
		return -ENOTSUP;
	}

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

#ifdef USE_BASIC_QDEC
	(void)clock_control_on(BEE_CLOCK_CONTROLLER, (clock_control_subsys_t)&config->clkid);
#endif

	ret = data->ops->init(config->reg, config->axis_cfgs);
	if (ret < 0) {
		return ret;
	}

	for (int i = 0; i < QDEC_AXIS_COUNT; i++) {
		if (config->axis_cfgs[i].enable) {
			data->ops->int_enable(config->reg, i, BEE_QDEC_EVENT_NEW_DATA);
			data->ops->int_enable(config->reg, i, BEE_QDEC_EVENT_ILLEGAL);
			data->ops->enable(config->reg, i);
		}
	}

	config->irq_connect();

	return 0;
}

static DEVICE_API(sensor, qdec_bee_driver_api) = {
	.sample_fetch = qdec_bee_sample_fetch,
	.channel_get = qdec_bee_channel_get,
	.trigger_set = qdec_bee_trigger_set,
};

#define QDEC_BEE_AXIS_CFG(inst, axis_name)                                                         \
	{.enable = DT_INST_PROP_OR(inst, axis_name##_enable, 0),                                   \
	 .debounce_time_ms = DT_INST_PROP_OR(inst, axis_name##_debounce_time_ms, 0),               \
	 .counts_per_revolution = DT_INST_PROP_OR(inst, axis_name##_counts_per_revolution, 4)}

#ifdef USE_BASIC_QDEC
#define QDEC_BEE_CLK_INIT(inst) .clkid = DT_INST_CLOCKS_CELL(inst, id),
#define QDEC_BEE_IRQ_CONNECT_FUNC(inst)                                                            \
	static void qdec_bee_isr_wrapper_##inst(void)                                              \
	{                                                                                          \
		const struct device *dev = DEVICE_DT_INST_GET(inst);                               \
		qdec_bee_isr((void *)dev);                                                         \
	}                                                                                          \
	static void qdec_bee_irq_connect_##inst(void)                                              \
	{                                                                                          \
		RamVectorTableUpdate(Qdecode_VECTORn, qdec_bee_isr_wrapper_##inst);                \
		NVIC_InitTypeDef NVIC_InitStruct;                                                  \
		NVIC_InitStruct.NVIC_IRQChannel = Qdecode_IRQn;                                    \
		NVIC_InitStruct.NVIC_IRQChannelPriority = 2;                                       \
		NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;                                       \
		NVIC_Init(&NVIC_InitStruct);                                                       \
	}
#else
#define QDEC_BEE_CLK_INIT(inst)
#define QDEC_BEE_IRQ_CONNECT_FUNC(inst)                                                            \
	static void qdec_bee_irq_connect_##inst(void)                                              \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority), qdec_bee_isr,         \
			    DEVICE_DT_INST_GET(inst), 0);                                          \
		irq_enable(DT_INST_IRQN(inst));                                                    \
	}
#endif

#if (QDEC_AXIS_COUNT == 1)
#define AON_QDEC_BEE_ASSERT(inst)                                                                  \
	BUILD_ASSERT(DT_INST_PROP_OR(inst, x_enable, 0) == 1, "AON QDEC requires x_enable=1");     \
	BUILD_ASSERT(DT_INST_PROP_OR(inst, y_enable, 0) == 0,                                      \
		     "AON QDEC does not support y_enable");                                        \
	BUILD_ASSERT(DT_INST_PROP_OR(inst, z_enable, 0) == 0, "AON QDEC does not support "         \
							      "z_enable");
#else
#define AON_QDEC_BEE_ASSERT(inst)
#endif

#define QDEC_BEE_DEFINE(inst)                                                                      \
	BUILD_ASSERT((DT_INST_PROP_OR(inst, x_enable, 0) | DT_INST_PROP_OR(inst, y_enable, 0) |    \
		      DT_INST_PROP_OR(inst, z_enable, 0)) != 0,                                    \
		     "Enable at least one qdec axis in device tree");                              \
                                                                                                   \
	AON_QDEC_BEE_ASSERT(inst)                                                                  \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
                                                                                                   \
	QDEC_BEE_IRQ_CONNECT_FUNC(inst)                                                            \
                                                                                                   \
	static const struct qdec_bee_config qdec_bee_config_##inst = {                             \
		.reg = DT_INST_REG_ADDR(inst),                                                     \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                      \
		.irq_connect = qdec_bee_irq_connect_##inst,                                        \
		.axis_cfgs =                                                                       \
			{                                                                          \
				QDEC_BEE_AXIS_CFG(inst, x),                                        \
				QDEC_BEE_AXIS_CFG(inst, y),                                        \
				QDEC_BEE_AXIS_CFG(inst, z),                                        \
			},                                                                         \
		QDEC_BEE_CLK_INIT(inst)};                                                          \
                                                                                                   \
	static struct qdec_bee_data qdec_bee_data_##inst;                                          \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, qdec_bee_init, NULL, &qdec_bee_data_##inst,             \
				     &qdec_bee_config_##inst, POST_KERNEL,                         \
				     CONFIG_SENSOR_INIT_PRIORITY, &qdec_bee_driver_api);

DT_INST_FOREACH_STATUS_OKAY(QDEC_BEE_DEFINE)
