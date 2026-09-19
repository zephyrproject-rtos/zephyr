/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 * Copyright (c) 2022 Esco Medical ApS
 * Copyright (c) 2016 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/util.h>
#include "icm20689.h"
#include "icm20689_reg.h"
#include "icm20689_spi.h"
#include "icm20689_trigger.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(ICM20689, CONFIG_SENSOR_LOG_LEVEL);

static void icm20689_gpio_callback(const struct device *dev, struct gpio_callback *cb,
				   uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pins);

	struct icm20689_data *data = CONTAINER_OF(cb, struct icm20689_data, gpio_cb);
	const struct icm20689_config *cfg = data->dev->config;

	gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_DISABLE);

#if defined(CONFIG_ICM20689_TRIGGER_OWN_THREAD)
	k_sem_give(&data->gpio_sem);
#elif defined(CONFIG_ICM20689_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
}

static void icm20689_thread_cb(const struct device *dev)
{
	struct icm20689_data *data = dev->data;
	const struct icm20689_config *cfg = dev->config;
	sensor_trigger_handler_t handler;
	const struct sensor_trigger *trigger;
	uint8_t status;
	int ret;

	icm20689_lock(dev);
	ret = icm20689_spi_read(&cfg->spi, ICM20689_REG_INT_STATUS, &status, 1);
	handler = data->data_ready_handler;
	trigger = data->data_ready_trigger;
	icm20689_unlock(dev);

	if (ret != 0) {
		LOG_ERR("Failed to read interrupt status: %d", ret);
		goto reenable;
	}

	if (((status & ICM20689_INT_STATUS_BIT_DATA_RDY_INT) != 0U) && (handler != NULL)) {
		handler(dev, trigger);
	}

reenable:
	icm20689_lock(dev);
	handler = data->data_ready_handler;
	icm20689_unlock(dev);

	if (handler != NULL) {
		gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_EDGE_TO_ACTIVE);
	}
}

#if defined(CONFIG_ICM20689_TRIGGER_OWN_THREAD)

static void icm20689_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct icm20689_data *data = p1;

	while (1) {
		k_sem_take(&data->gpio_sem, K_FOREVER);
		icm20689_thread_cb(data->dev);
	}
}

#elif defined(CONFIG_ICM20689_TRIGGER_GLOBAL_THREAD)

static void icm20689_work_handler(struct k_work *work)
{
	struct icm20689_data *data = CONTAINER_OF(work, struct icm20689_data, work);

	icm20689_thread_cb(data->dev);
}

#endif

int icm20689_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler)
{
	struct icm20689_data *data = dev->data;
	const struct icm20689_config *cfg = dev->config;
	sensor_trigger_handler_t previous_handler;
	const struct sensor_trigger *previous_trigger;
	uint8_t status;
	int gpio_ret;
	int ret;

	if ((trig == NULL) || (trig->type != SENSOR_TRIG_DATA_READY)) {
		return -ENOTSUP;
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_DISABLE);
	if (ret != 0) {
		return ret;
	}

	icm20689_lock(dev);
	previous_handler = data->data_ready_handler;
	previous_trigger = data->data_ready_trigger;

	if (handler != NULL) {
		ret = icm20689_spi_read(&cfg->spi, ICM20689_REG_INT_STATUS, &status, 1);
		if (ret == 0) {
			ret = icm20689_spi_update_register(&cfg->spi, ICM20689_REG_INT_ENABLE,
							   ICM20689_INT_ENABLE_MASK_DATA_RDY_INT_EN,
							   ICM20689_INT_ENABLE_BIT_DATA_RDY_INT_EN);
		}
	} else {
		ret = icm20689_spi_update_register(&cfg->spi, ICM20689_REG_INT_ENABLE,
						   ICM20689_INT_ENABLE_MASK_DATA_RDY_INT_EN, 0U);
	}

	if (ret == 0) {
		data->data_ready_handler = handler;
		data->data_ready_trigger = handler != NULL ? trig : NULL;
	}
	icm20689_unlock(dev);

	if (ret != 0) {
		if (previous_handler != NULL) {
			gpio_ret = gpio_pin_interrupt_configure_dt(&cfg->gpio_int,
								   GPIO_INT_EDGE_TO_ACTIVE);
			if (gpio_ret != 0) {
				LOG_ERR("Failed to restore GPIO interrupt: %d", gpio_ret);
			}
		}

		return ret;
	}

	if (handler == NULL) {
		return 0;
	}

	gpio_ret = gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_EDGE_TO_ACTIVE);
	if (gpio_ret == 0) {
		return 0;
	}

	LOG_ERR("Failed to enable GPIO interrupt: %d", gpio_ret);

	icm20689_lock(dev);
	ret = icm20689_spi_update_register(
		&cfg->spi, ICM20689_REG_INT_ENABLE, ICM20689_INT_ENABLE_MASK_DATA_RDY_INT_EN,
		previous_handler != NULL ? ICM20689_INT_ENABLE_BIT_DATA_RDY_INT_EN : 0U);
	data->data_ready_handler = previous_handler;
	data->data_ready_trigger = previous_trigger;
	icm20689_unlock(dev);

	if (ret != 0) {
		LOG_ERR("Failed to restore sensor interrupt state: %d", ret);
	}

	if (previous_handler != NULL) {
		ret = gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_EDGE_TO_ACTIVE);
		if (ret != 0) {
			LOG_ERR("Failed to restore GPIO interrupt: %d", ret);
		}
	}

	return gpio_ret;
}

int icm20689_trigger_init(const struct device *dev)
{
	struct icm20689_data *data = dev->data;
	const struct icm20689_config *cfg = dev->config;
	uint8_t int_pin_cfg = 0U;
	uint8_t status;
	int ret;

	if (!cfg->gpio_int.port) {
		LOG_ERR("trigger enabled but no interrupt gpio supplied");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&cfg->gpio_int)) {
		LOG_ERR("gpio_int gpio not ready");
		return -ENODEV;
	}

	data->dev = dev;
	k_mutex_init(&data->mutex);

	if ((cfg->gpio_int.dt_flags & GPIO_ACTIVE_LOW) != 0U) {
		int_pin_cfg = ICM20689_INT_PIN_CFG_BIT_INT_LEVEL;
	}

	ret = icm20689_spi_single_write(&cfg->spi, ICM20689_REG_INT_PIN_CFG, int_pin_cfg);
	if (ret != 0) {
		return ret;
	}

	ret = icm20689_spi_update_register(&cfg->spi, ICM20689_REG_INT_ENABLE,
					   ICM20689_INT_ENABLE_MASK_DATA_RDY_INT_EN, 0U);
	if (ret != 0) {
		return ret;
	}

	ret = icm20689_spi_read(&cfg->spi, ICM20689_REG_INT_STATUS, &status, 1);
	if (ret != 0) {
		return ret;
	}

	ret = gpio_pin_configure_dt(&cfg->gpio_int, GPIO_INPUT);
	if (ret != 0) {
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_DISABLE);
	if (ret != 0) {
		return ret;
	}

	gpio_init_callback(&data->gpio_cb, icm20689_gpio_callback, BIT(cfg->gpio_int.pin));
	ret = gpio_add_callback(cfg->gpio_int.port, &data->gpio_cb);
	if (ret != 0) {
		LOG_ERR("Failed to set gpio callback");
		return ret;
	}

#if defined(CONFIG_ICM20689_TRIGGER_OWN_THREAD)
	k_sem_init(&data->gpio_sem, 0, K_SEM_MAX_LIMIT);
	k_thread_create(&data->thread, data->thread_stack, CONFIG_ICM20689_THREAD_STACK_SIZE,
			icm20689_thread, data, NULL, NULL,
			K_PRIO_COOP(CONFIG_ICM20689_THREAD_PRIORITY), 0, K_NO_WAIT);
#elif defined(CONFIG_ICM20689_TRIGGER_GLOBAL_THREAD)
	k_work_init(&data->work, icm20689_work_handler);
#endif

	return 0;
}

void icm20689_lock(const struct device *dev)
{
	struct icm20689_data *data = dev->data;

	k_mutex_lock(&data->mutex, K_FOREVER);
}

void icm20689_unlock(const struct device *dev)
{
	struct icm20689_data *data = dev->data;

	k_mutex_unlock(&data->mutex);
}
