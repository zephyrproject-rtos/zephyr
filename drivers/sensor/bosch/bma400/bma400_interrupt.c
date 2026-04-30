/*
 * Bosch BMA400 3-axis accelerometer driver
 * SPDX-FileCopyrightText: Copyright 2026 Luca Gessi lucagessi90@gmail.com
 * SPDX-FileCopyrightText: Copyright 2026 The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bosch_bma400

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "bma400.h"
#include "bma400_interrupt.h"
#include "bma400_defs.h"
#include "bma400_rtio.h"

LOG_MODULE_DECLARE(bma400, CONFIG_SENSOR_LOG_LEVEL);

#ifdef CONFIG_BMA400_STREAM

static void bma400_gpio_callback1(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct bma400_data *data = CONTAINER_OF(cb, struct bma400_data, gpio_cb1);
	const struct bma400_config *drv_cfg = data->dev->config;

	if (data->pending_gpio_interrupt) {
		/* Another interrupt is pending, ignore this one */
		return;
	}
	data->pending_gpio_interrupt = &drv_cfg->gpio_interrupt1;
	gpio_pin_interrupt_configure_dt(&drv_cfg->gpio_interrupt1, GPIO_INT_DISABLE);

	ARG_UNUSED(dev);
	ARG_UNUSED(pins);

	bma400_stream_event(data->dev);
}

static void bma400_gpio_callback2(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct bma400_data *data = CONTAINER_OF(cb, struct bma400_data, gpio_cb2);
	const struct bma400_config *drv_cfg = data->dev->config;

	if (data->pending_gpio_interrupt) {
		/* Another interrupt is pending, ignore this one */
		return;
	}
	data->pending_gpio_interrupt = &drv_cfg->gpio_interrupt2;
	gpio_pin_interrupt_configure_dt(&drv_cfg->gpio_interrupt2, GPIO_INT_DISABLE);
	ARG_UNUSED(dev);
	ARG_UNUSED(pins);

	bma400_stream_event(data->dev);
}

int bma400_init_interrupt(const struct device *dev)
{
	struct bma400_data *data = dev->data;
	const struct bma400_config *cfg = dev->config;
	int ret = 0;

	/* Init GPIO interrupt 1 */
	if (!cfg->gpio_interrupt1.port) {
		LOG_ERR("Stream enabled but no interrupt 1 gpio supplied");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&cfg->gpio_interrupt1)) {
		LOG_ERR("GPIO interrupt 1 not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&cfg->gpio_interrupt1, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure gpio interrupt 1 pin");
		return ret;
	}

	gpio_init_callback(&data->gpio_cb1, bma400_gpio_callback1, BIT(cfg->gpio_interrupt1.pin));

	ret = gpio_add_callback(cfg->gpio_interrupt1.port, &data->gpio_cb1);
	if (ret < 0) {
		LOG_ERR("Failed to add gpio callback 1");
		return ret;
	}
	/* Init GPIO interrupt 2 */
	if (cfg->gpio_interrupt2.port) {
		if (!gpio_is_ready_dt(&cfg->gpio_interrupt2)) {
			LOG_ERR("GPIO interrupt 2 not ready");
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&cfg->gpio_interrupt2, GPIO_INPUT);
		if (ret < 0) {
			LOG_ERR("Failed to configure gpio interrupt 2 pin");
			return ret;
		}

		gpio_init_callback(&data->gpio_cb2, bma400_gpio_callback2,
				   BIT(cfg->gpio_interrupt2.pin));

		ret = gpio_add_callback(cfg->gpio_interrupt2.port, &data->gpio_cb2);
		if (ret < 0) {
			LOG_ERR("Failed to add gpio callback 2");
			return ret;
		}
	}
	data->dev = dev;

	return 0;
}

#endif /* CONFIG_BMA400_STREAM */
