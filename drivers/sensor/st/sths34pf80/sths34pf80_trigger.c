/*
 * Copyright (c) 2026 Mahendra Sondagar <mahendra@dotcom.co.in>
 * Copyright (c) 2026 Parin Baudhanwala <parin.baudhanwala@dnkmail.in>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Data-ready (DRDY) trigger support of the STHS34PF80 driver.
 *
 * The INT/DRDY pin interrupt is handled in the system work queue and the
 * application handler is called from there. The handler is expected to read
 * the sensor, for instance with sensor_read() (Read & Decode API).
 */

#define DT_DRV_COMPAT st_sths34pf80

#include <errno.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "sths34pf80.h"

LOG_MODULE_DECLARE(STHS34PF80, CONFIG_SENSOR_LOG_LEVEL);

static void sths34pf80_work_handler(struct k_work *work)
{
	struct sths34pf80_data *data = CONTAINER_OF(work, struct sths34pf80_data, work);

	if (data->handler != NULL) {
		data->handler(data->dev, &data->trig);
	}
}

static void sths34pf80_gpio_callback(const struct device *port, struct gpio_callback *cb,
				     uint32_t pins)
{
	struct sths34pf80_data *data = CONTAINER_OF(cb, struct sths34pf80_data, gpio_cb);

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	k_work_submit(&data->work);
}

int sths34pf80_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			   sensor_trigger_handler_t handler)
{
	const struct sths34pf80_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	struct sths34pf80_data *data = dev->data;
	int ret;

	if (cfg->int_gpio.port == NULL) {
		return -ENOTSUP;
	}

	if (trig == NULL || trig->type != SENSOR_TRIG_DATA_READY) {
		return -ENOTSUP;
	}

	/* Stop the interrupt while the handler is changed */
	ret = gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_DISABLE);
	if (ret != 0) {
		return ret;
	}

	data->handler = handler;
	data->trig = *trig;

	/* Route DRDY (or nothing) to the INT pin */
	ret = sths34pf80_route_int_set(ctx,
				       handler != NULL ? STHS34PF80_INT_DRDY : STHS34PF80_INT_HIZ);
	if (ret != 0) {
		LOG_ERR("Failed to route DRDY (%d)", ret);
		return -EIO;
	}

	if (handler == NULL) {
		return 0;
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		return ret;
	}

	/* The pin may already be active: no edge would be seen, so kick the handler once */
	if (gpio_pin_get_dt(&cfg->int_gpio) > 0) {
		k_work_submit(&data->work);
	}

	return 0;
}

int sths34pf80_trigger_init(const struct device *dev)
{
	const struct sths34pf80_config *cfg = dev->config;
	struct sths34pf80_data *data = dev->data;
	int ret;

	data->dev = dev;
	k_work_init(&data->work, sths34pf80_work_handler);

	if (cfg->int_gpio.port == NULL) {
		return 0; /* "int-gpios" not in devicetree: polling only */
	}

	if (!gpio_is_ready_dt(&cfg->int_gpio)) {
		LOG_ERR("INT GPIO not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&cfg->int_gpio, GPIO_INPUT);
	if (ret != 0) {
		return ret;
	}

	gpio_init_callback(&data->gpio_cb, sths34pf80_gpio_callback, BIT(cfg->int_gpio.pin));

	return gpio_add_callback(cfg->int_gpio.port, &data->gpio_cb);
}
