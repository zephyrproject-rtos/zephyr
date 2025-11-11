/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/irq.h>
#include "fake_driver.h"

#define DT_DRV_COMPAT fakedriver

static void fake_driver_isr(const void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct fake_driver_data *data = dev->data;

	/* Store the address of fake_driver_isr in user_data because it will be optimized by the
	 * compiler (even with __noinline__ attribute)
	 */
	data->user_data = (void *)&fake_driver_isr;

	if (data->irq_callback != NULL) {
		data->irq_callback(dev, data->user_data);
	}
}

static int fake_driver_configure(const struct device *dev, int config)
{
	return 0;
}

static int fake_driver_register_irq_callback(const struct device *dev,
					     fake_driver_irq_callback_t cb, void *user_data)
{
	struct fake_driver_data *data = dev->data;

	data->irq_callback = cb;
	data->user_data = user_data;

	return 0;
}

DEVICE_API(fake, fake_driver_func) = {
	.configure = fake_driver_configure,
	.register_irq_callback = fake_driver_register_irq_callback,
};

static int fake_driver_init(const struct device *dev)
{
	const struct fake_driver_config *config = dev->config;
	struct fake_driver_data *data = dev->data;

	data->irq_callback = NULL;
	data->user_data = NULL;

	config->irq_config_func();

	return 0;
}

#define FAKE_INIT(inst)                                                                            \
	static struct fake_driver_data fake_driver_data_##inst;                                    \
	static void fake_driver_irq_config_func_##inst(void)                                       \
	{                                                                                          \
		IRQ_CONNECT(TEST_IRQ_NUM, CONFIG_TEST_IRQ_PRIO, fake_driver_isr,                   \
			    DEVICE_DT_INST_GET(inst), 0);                                          \
		irq_enable(TEST_IRQ_NUM);                                                          \
	}                                                                                          \
	static struct fake_driver_config fake_driver_config_##inst = {                             \
		.irq_config_func = fake_driver_irq_config_func_##inst,                             \
		.irq_num = TEST_IRQ_NUM,                                                           \
		.irq_priority = CONFIG_TEST_IRQ_PRIO,                                              \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, &fake_driver_init, NULL, &fake_driver_data_##inst,             \
			      &fake_driver_config_##inst, PRE_KERNEL_1,                            \
			      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &fake_driver_func);

DT_INST_FOREACH_STATUS_OKAY(FAKE_INIT)
