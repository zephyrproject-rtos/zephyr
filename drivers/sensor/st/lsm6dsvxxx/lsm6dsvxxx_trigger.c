/* ST Microelectronics LSM6DSVXXX family IMU sensor
 *
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lsm6dsv320x.pdf
 */

#define DT_DRV_COMPAT st_lsm6dsvxxx

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "lsm6dsvxxx.h"
#include "lsm6dsvxxx_rtio.h"

LOG_MODULE_DECLARE(LSM6DSVXXX, CONFIG_SENSOR_LOG_LEVEL);

static void lsm6dsvxxx_gpio_callback(const struct device *dev, struct gpio_callback *cb,
				     uint32_t pins)
{
	struct lsm6dsvxxx_data *lsm6dsvxxx = CONTAINER_OF(cb, struct lsm6dsvxxx_data, gpio_cb);

	ARG_UNUSED(pins);

	gpio_pin_interrupt_configure_dt(lsm6dsvxxx->drdy_gpio, GPIO_INT_DISABLE);

	if (IS_ENABLED(CONFIG_LSM6DSVXXX_STREAM)) {
		lsm6dsvxxx_stream_irq_handler(lsm6dsvxxx->dev);
	}
}

int lsm6dsvxxx_init_interrupt(const struct device *dev)
{
	struct lsm6dsvxxx_data *lsm6dsvxxx = dev->data;
	const struct lsm6dsvxxx_config *cfg = dev->config;
	int ret;

	lsm6dsvxxx->drdy_gpio = (cfg->drdy_pin == 1) ? (const struct gpio_dt_spec *)&cfg->int1_gpio
						     : (const struct gpio_dt_spec *)&cfg->int2_gpio;

	/* setup data ready gpio interrupt (INT1 or INT2) */
	if (!gpio_is_ready_dt(lsm6dsvxxx->drdy_gpio)) {
		LOG_ERR("Cannot get pointer to drdy_gpio device");
		return -ENODEV;
	}

	lsm6dsvxxx->dev = dev;

	ret = gpio_pin_configure_dt(lsm6dsvxxx->drdy_gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Could not configure gpio");
		return ret;
	}

	gpio_init_callback(&lsm6dsvxxx->gpio_cb, lsm6dsvxxx_gpio_callback,
			   BIT(lsm6dsvxxx->drdy_gpio->pin));

	if (gpio_add_callback(lsm6dsvxxx->drdy_gpio->port, &lsm6dsvxxx->gpio_cb) < 0) {
		LOG_DBG("Could not set gpio callback");
		return -EIO;
	}

	/* set drdy mode on int1/int2 */
	cfg->chip_api->drdy_mode_set(dev);

	return gpio_pin_interrupt_configure_dt(lsm6dsvxxx->drdy_gpio, GPIO_INT_EDGE_TO_ACTIVE);
}
