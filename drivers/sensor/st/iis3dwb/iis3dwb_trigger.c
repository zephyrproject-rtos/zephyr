/* ST Microelectronics IIS3DWB accelerometer senor
 *
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/iis3dwb.pdf
 */

#define DT_DRV_COMPAT st_iis3dwb

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "iis3dwb.h"

LOG_MODULE_DECLARE(IIS3DWB, CONFIG_SENSOR_LOG_LEVEL);

/**
 * iis3dwb_route_int1 - enable selected int pin1 to generate interrupt
 */
int iis3dwb_route_int1(const struct device *dev, iis3dwb_pin_int1_route_t pin_int)
{
	const struct iis3dwb_config *config = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&config->ctx;
	int ret;

	ret = iis3dwb_pin_int1_route_set(ctx, &pin_int);
	if (ret < 0) {
		LOG_ERR("%s: route on int1 error %d", dev->name, ret);
		return ret;
	}

	return 0;
}

/**
 * iis3dwb_route_int2 - enable selected int pin2 to generate interrupt
 */
int iis3dwb_route_int2(const struct device *dev, iis3dwb_pin_int2_route_t pin_int)
{
	const struct iis3dwb_config *config = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&config->ctx;
	int ret;

	ret = iis3dwb_pin_int2_route_set(ctx, &pin_int);
	if (ret < 0) {
		LOG_ERR("%s: route on int2 error %d", dev->name, ret);
		return ret;
	}

	return 0;
}

static void iis3dwb_gpio_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct iis3dwb_data *iis3dwb = CONTAINER_OF(cb, struct iis3dwb_data, gpio_cb);

	ARG_UNUSED(pins);

	gpio_pin_interrupt_configure_dt(iis3dwb->drdy_gpio, GPIO_INT_DISABLE);

	if (IS_ENABLED(CONFIG_IIS3DWB_STREAM)) {
		iis3dwb_stream_irq_handler(iis3dwb->dev);
	}
}

int iis3dwb_init_interrupt(const struct device *dev)
{
	struct iis3dwb_data *iis3dwb = dev->data;
	const struct iis3dwb_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	int ret;

	iis3dwb->drdy_gpio = (cfg->drdy_pin == 1) ? (const struct gpio_dt_spec *)&cfg->int1_gpio
						  : (const struct gpio_dt_spec *)&cfg->int2_gpio;

	/* setup data ready gpio interrupt (INT1 or INT2) */
	if (!gpio_is_ready_dt(iis3dwb->drdy_gpio)) {
		LOG_ERR("Cannot get pointer to drdy_gpio device");
		return -ENODEV;
	}

	iis3dwb->dev = dev;

	ret = gpio_pin_configure_dt(iis3dwb->drdy_gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Could not configure gpio");
		return ret;
	}

	gpio_init_callback(&iis3dwb->gpio_cb, iis3dwb_gpio_callback, BIT(iis3dwb->drdy_gpio->pin));

	if (gpio_add_callback(iis3dwb->drdy_gpio->port, &iis3dwb->gpio_cb) < 0) {
		LOG_DBG("Could not set gpio callback");
		return -EIO;
	}

	/* enable drdy on int1/int2 in pulse mode */
	iis3dwb_dataready_pulsed_t drdy =
		(cfg->drdy_pulsed) ? IIS3DWB_DRDY_PULSED : IIS3DWB_DRDY_LATCHED;
	if (iis3dwb_data_ready_mode_set(ctx, drdy)) {
		return -EIO;
	}

	return gpio_pin_interrupt_configure_dt(iis3dwb->drdy_gpio, GPIO_INT_EDGE_TO_ACTIVE);
}
