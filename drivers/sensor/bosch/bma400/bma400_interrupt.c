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

	ARG_UNUSED(dev);
	ARG_UNUSED(pins);

	gpio_pin_interrupt_configure_dt(&drv_cfg->gpio_interrupt, GPIO_INT_DISABLE);
	bma400_stream_event(data->dev);
}

int bma400_init_interrupt(const struct device *dev)
{
	struct bma400_data *data = dev->data;
	const struct bma400_config *cfg = dev->config;
	int ret = 0;

	if (!cfg->gpio_interrupt.port) {
		LOG_ERR("Stream enabled but no interrupt 1 gpio supplied");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&cfg->gpio_interrupt)) {
		LOG_ERR("GPIO interrupt 1 not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&cfg->gpio_interrupt, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure gpio interrupt 1 pin");
		return ret;
	}

	gpio_init_callback(&data->gpio_cb1, bma400_gpio_callback1, BIT(cfg->gpio_interrupt.pin));

	ret = gpio_add_callback(cfg->gpio_interrupt.port, &data->gpio_cb1);
	if (ret < 0) {
		LOG_ERR("Failed to add gpio callback 1");
		return ret;
	}

	data->dev = dev;

	return 0;
}

#endif /* CONFIG_BMA400_STREAM */
