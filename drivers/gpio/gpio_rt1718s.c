/*
 * Copyright 2022 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file File that collects common data and configs for RS1718S chip. The file
 * doesn't provide any API itself. The feature-specific API should be provided
 * in separated files e.g. GPIO API.
 *
 * This file is placed in drivers/gpio directory, because GPIO is only one
 * supported feature at the moment. It can be move to tcpc dir in the future.
 */

#define DT_DRV_COMPAT richtek_rt1718s

#include "gpio_rt1718s.h"

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util_macro.h>
LOG_MODULE_REGISTER(gpio_rt1718s, CONFIG_GPIO_LOG_LEVEL);

static void rt1718s_alert_callback(const struct device *dev, struct gpio_callback *cb,
				   uint32_t pins)
{
	ARG_UNUSED(pins);
	struct rt1718s_data *data = CONTAINER_OF(cb, struct rt1718s_data, gpio_cb);

	k_work_submit(&data->alert_worker);
}

static void rt1718s_alert_worker(struct k_work *work)
{
	struct rt1718s_data *const data = CONTAINER_OF(work, struct rt1718s_data, alert_worker);
	const struct device *const dev = data->dev;
	const struct rt1718s_config *const config = dev->config;
	uint16_t alert, mask;

	do {
		/* Read alert and mask */
		k_sem_take(&data->lock_tcpci, K_FOREVER);
		if (rt1718s_reg_burst_read(dev, RT1718S_REG_ALERT, (uint8_t *)&alert,
					   sizeof(alert)) ||
		    rt1718s_reg_burst_read(dev, RT1718S_REG_ALERT_MASK, (uint8_t *)&mask,
					   sizeof(mask))) {
			k_sem_give(&data->lock_tcpci);
			LOG_ERR("i2c access failed");
			continue;
		}

		/* Content of the alert and alert mask registers are
		 * defined by the TCPCI specification - "A masked
		 * register will still indicate in the ALERT register,
		 * but shall not set the Alert# pin low"
		 */
		alert &= mask;
		if (alert) {
			/* Clear all alert bits that causes the interrupt */
			if (rt1718s_reg_burst_write(dev, RT1718S_REG_ALERT, (uint8_t *)&alert,
						    sizeof(alert))) {
				k_sem_give(&data->lock_tcpci);
				LOG_ERR("i2c access failed");
				continue;
			}
		}
		k_sem_give(&data->lock_tcpci);

		/* There are a few sources of the vendor
		 * defined alert for the RT1718S, but handle
		 * only GPIO at the moment.
		 */
		if (alert & RT1718S_REG_ALERT_VENDOR_DEFINED_ALERT) {
			rt1718s_gpio_alert_handler(dev);
		}
		/* While the interrupt signal is still active, we have more work to do. */
	} while (gpio_pin_get_dt(&config->irq_gpio));
}

static int rt1718s_init(const struct device *dev)
{
	const struct rt1718s_config *const config = dev->config;
	struct rt1718s_data *data = dev->data;
	int ret;

	/* Check I2C is ready */
	if (!device_is_ready(config->i2c_dev.bus)) {
		LOG_ERR("%s device not ready", config->i2c_dev.bus->name);
		return -ENODEV;
	}

	k_sem_init(&data->lock_tcpci, 1, 1);

	if (IS_ENABLED(CONFIG_GPIO_RT1718S_INTERRUPT)) {
		if (!gpio_is_ready_dt(&config->irq_gpio)) {
			LOG_ERR("%s device not ready", config->irq_gpio.port->name);
			return -ENODEV;
		}

		/* Set the interrupt pin for handling the alert */
		k_work_init(&data->alert_worker, rt1718s_alert_worker);

		gpio_pin_configure_dt(&config->irq_gpio, GPIO_INPUT);

		gpio_init_callback(&data->gpio_cb, rt1718s_alert_callback,
				   BIT(config->irq_gpio.pin));

		ret = gpio_add_callback(config->irq_gpio.port, &data->gpio_cb);
		if (ret < 0) {
			return ret;
		}

		gpio_pin_interrupt_configure_dt(&config->irq_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	}

	return 0;
}

#define CHECK_PORT_DEVICE(node_id)                                                                 \
	COND_CODE_1(DT_HAS_COMPAT_STATUS_OKAY(richtek_rt1718s_gpio_port), DEVICE_DT_GET(node_id),  \
		    ())

#define IRQ_GPIO(inst)                                                                             \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, irq_gpios),                                        \
		    (.irq_gpio = GPIO_DT_SPEC_INST_GET(inst, irq_gpios)), ())

#define GET_PORT_DEVICE(inst) DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, CHECK_PORT_DEVICE)

#define GPIO_RT1718S_DEVICE_INSTANCE(inst)                                                         \
	static const struct rt1718s_config rt1718s_cfg_##inst = {                                  \
		.i2c_dev = I2C_DT_SPEC_INST_GET(inst),                                             \
		.gpio_port_dev = GET_PORT_DEVICE(inst),                                            \
		IRQ_GPIO(inst)                                                                     \
	};                                                                                         \
	static struct rt1718s_data rt1718s_data_##inst = {                                         \
		.dev = DEVICE_DT_INST_GET(inst),                                                   \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, rt1718s_init, NULL, &rt1718s_data_##inst, &rt1718s_cfg_##inst, \
			      POST_KERNEL, CONFIG_RT1718S_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(GPIO_RT1718S_DEVICE_INSTANCE)
