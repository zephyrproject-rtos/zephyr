/*
 * Copyright Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <nrf.h>
#include <errno.h>
#include <stdint.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(vregusb, CONFIG_REGULATOR_LOG_LEVEL);

struct vregusb_config {
	struct regulator_common_config common;
	NRF_VREGUSB_Type *base;
	void (*irq_enable_func)(const struct device *dev);
	void (*irq_disable_func)(const struct device *dev);
};

struct vregusb_data {
	struct regulator_common_data data;
	regulator_callback_t cb;
	const void *user_data;
};

static void vregusb_isr(void *const arg)
{
	const struct device *const dev = arg;
	const struct vregusb_config *const config = dev->config;
	struct vregusb_data *const data = dev->data;
	NRF_VREGUSB_Type *const base = config->base;
	struct regulator_event event;

	if (base->EVENTS_VBUSDETECTED) {
		LOG_DBG("VBUS detected");
		base->EVENTS_VBUSDETECTED = 0;
		event.type = REGULATOR_VOLTAGE_DETECTED;
	}

	if (base->EVENTS_VBUSREMOVED) {
		LOG_DBG("VBUS removed");
		base->EVENTS_VBUSREMOVED = 0;
		event.type = REGULATOR_VOLTAGE_REMOVED;
	}

	if (data->cb != NULL) {
		data->cb(dev, &event, data->user_data);
	}
}

static int vregusb_enable(const struct device *const dev)
{
	const struct vregusb_config *const config = dev->config;
	NRF_VREGUSB_Type *const base = config->base;

	base->INTEN = VREGUSB_INTEN_VBUSDETECTED_Msk | VREGUSB_INTEN_VBUSREMOVED_Msk;
	config->irq_enable_func(dev);

	base->TASKS_START = 1;

	return 0;
}

static int vregusb_disable(const struct device *const dev)
{
	const struct vregusb_config *const config = dev->config;
	NRF_VREGUSB_Type *const base = config->base;

	config->irq_disable_func(dev);
	base->TASKS_STOP = 1;

	return 0;
}

static int vregusb_set_callback(const struct device *const dev,
				regulator_callback_t cb, const void *const user_data)
{
	struct vregusb_data *const data = dev->data;

	data->cb = cb;
	data->user_data = user_data;

	return 0;
}

static int regulator_vregusb_init(const struct device *const dev)
{
	regulator_common_data_init(dev);

	return 0;
}

static DEVICE_API(regulator, api) = {
	.enable = vregusb_enable,
	.disable = vregusb_disable,
	.set_callback = vregusb_set_callback,
};

#define DT_DRV_COMPAT nordic_vregusb_regulator

#define REGULATOR_VREGUSB_DEFINE(n)						\
	static void irq_enable_func_##n(const struct device *const dev)		\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(n),					\
			    DT_INST_IRQ(n, priority),				\
			    vregusb_isr,					\
			    DEVICE_DT_INST_GET(n),				\
			    0);							\
										\
		irq_enable(DT_INST_IRQN(n));					\
	}									\
										\
	static void irq_disable_func_##n(const struct device *const dev)	\
	{									\
		irq_disable(DT_INST_IRQN(n));					\
	}									\
										\
	static struct vregusb_data data_##n;					\
										\
	static const struct vregusb_config config_##n = {			\
		.base = (void *)(DT_INST_REG_ADDR(n)),				\
		.common = REGULATOR_DT_INST_COMMON_CONFIG_INIT(n),		\
		.irq_enable_func = irq_enable_func_##n,				\
		.irq_disable_func = irq_disable_func_##n,			\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n, regulator_vregusb_init, NULL,			\
			      &data_##n, &config_##n,				\
			      POST_KERNEL,					\
			      CONFIG_REGULATOR_NRF_VREGUSB_INIT_PRIORITY, &api);\

DT_INST_FOREACH_STATUS_OKAY(REGULATOR_VREGUSB_DEFINE)
