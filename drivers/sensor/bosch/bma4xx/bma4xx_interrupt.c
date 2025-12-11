/*
 * Copyright (c) 2024 Cienet
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bosch_bma4xx

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "bma4xx.h"
#include "bma4xx_interrupt.h"
#include "bma4xx_defs.h"
#include "bma4xx_rtio.h"

LOG_MODULE_DECLARE(bma4xx, CONFIG_SENSOR_LOG_LEVEL);

#ifdef CONFIG_BMA4XX_STREAM

static void bma4xx_gpio_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct bma4xx_data *data = CONTAINER_OF(cb, struct bma4xx_data, gpio_cb);

	ARG_UNUSED(dev);
	ARG_UNUSED(pins);

	bma4xx_fifo_event(data->dev);
}

int bma4xx_init_interrupt(const struct device *dev)
{
	struct bma4xx_data *data = dev->data;
	const struct bma4xx_config *cfg = dev->config;
	int ret = 0;

	if (!cfg->gpio_interrupt.port) {
		LOG_ERR("Stream enabled but no interrupt gpio supplied");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&cfg->gpio_interrupt)) {
		LOG_ERR("GPIO interrupt not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&cfg->gpio_interrupt, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure gpio pin");
		return ret;
	}

	gpio_init_callback(&data->gpio_cb, bma4xx_gpio_callback, BIT(cfg->gpio_interrupt.pin));

	ret = gpio_add_callback(cfg->gpio_interrupt.port, &data->gpio_cb);
	if (ret < 0) {
		LOG_ERR("Failed to add gpio callback");
		return ret;
	}

	data->dev = dev;

	return 0;
}

int bma4xx_enable_interrupt1(const struct device *dev, struct bma4xx_runtime_config *new_cfg)
{
	struct bma4xx_data *data = dev->data;
	uint8_t value = 0;

	if (new_cfg->interrupt1_fifo_wm) {
		value |= FIELD_PREP(BMA4XX_BIT_INT_MAP_DATA_INT1_FWM, 1);
	}
	if (new_cfg->interrupt1_fifo_full) {
		value |= FIELD_PREP(BMA4XX_BIT_INT_MAP_DATA_INT1_FFUL, 1);
	}

	return data->hw_ops->write_reg(dev, BMA4XX_REG_INT_MAP_DATA, value);
}

#endif /* CONFIG_BMA4XX_STREAM */
