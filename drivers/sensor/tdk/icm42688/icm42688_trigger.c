/*
 * Copyright (c) 2022 Intel Corporation
 * Copyright (c) 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "icm42688.h"
#include "icm42688_reg.h"
#include "icm42688_rtio.h"
#include "icm42688_spi.h"
#include "icm42688_trigger.h"

LOG_MODULE_DECLARE(ICM42688, CONFIG_SENSOR_LOG_LEVEL);

static void icm42688_gpio_callback(const struct device *dev, struct gpio_callback *cb,
				   uint32_t pins)
{
	struct icm42688_dev_data *data = CONTAINER_OF(cb, struct icm42688_dev_data, gpio_cb);

	ARG_UNUSED(dev);
	ARG_UNUSED(pins);

#if defined(CONFIG_ICM42688_TRIGGER_OWN_THREAD)
	k_sem_give(&data->gpio_sem);
#elif defined(CONFIG_ICM42688_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
	if (IS_ENABLED(CONFIG_ICM42688_STREAM)) {
		icm42688_fifo_event(data->dev);
	}
}

#if defined(CONFIG_ICM42688_TRIGGER_OWN_THREAD) || defined(CONFIG_ICM42688_TRIGGER_GLOBAL_THREAD)
static void icm42688_thread_cb(const struct device *dev)
{
	struct icm42688_dev_data *data = dev->data;

	icm42688_lock(dev);

	if (data->data_ready_handler != NULL) {
		data->data_ready_handler(dev, data->data_ready_trigger);
	}

	icm42688_unlock(dev);
}
#endif

#ifdef CONFIG_ICM42688_TRIGGER_OWN_THREAD

static void icm42688_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct icm42688_dev_data *data = p1;

	while (1) {
		k_sem_take(&data->gpio_sem, K_FOREVER);
		icm42688_thread_cb(data->dev);
	}
}

#elif defined(CONFIG_ICM42688_TRIGGER_GLOBAL_THREAD)

static void icm42688_work_handler(struct k_work *work)
{
	struct icm42688_dev_data *data = CONTAINER_OF(work, struct icm42688_dev_data, work);

	icm42688_thread_cb(data->dev);
}

#endif

int icm42688_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler)
{
	struct icm42688_dev_data *data = dev->data;
	const struct icm42688_dev_cfg *cfg = dev->config;
	uint8_t status;
	int res = 0;

	if (trig == NULL || handler == NULL) {
		return -EINVAL;
	}

	icm42688_lock(dev);
	gpio_pin_interrupt_configure_dt(&cfg->gpio_int1, GPIO_INT_DISABLE);

	switch (trig->type) {
	case SENSOR_TRIG_DATA_READY:
	case SENSOR_TRIG_FIFO_WATERMARK:
	case SENSOR_TRIG_FIFO_FULL:
		data->data_ready_handler = handler;
		data->data_ready_trigger = trig;

		res = icm42688_spi_read(&cfg->spi, REG_INT_STATUS, &status, 1);
		break;
	default:
		res = -ENOTSUP;
		break;
	}

	icm42688_unlock(dev);
	gpio_pin_interrupt_configure_dt(&cfg->gpio_int1, GPIO_INT_EDGE_TO_ACTIVE);

	return res;
}

int icm42688_trigger_init(const struct device *dev)
{
	struct icm42688_dev_data *data = dev->data;
	const struct icm42688_dev_cfg *cfg = dev->config;
	int res = 0;

	if (!cfg->gpio_int1.port) {
		LOG_ERR("trigger enabled but no interrupt gpio supplied");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&cfg->gpio_int1)) {
		LOG_ERR("gpio_int1 not ready");
		return -ENODEV;
	}

	data->dev = dev;
	gpio_pin_configure_dt(&cfg->gpio_int1, GPIO_INPUT);
	gpio_init_callback(&data->gpio_cb, icm42688_gpio_callback, BIT(cfg->gpio_int1.pin));
	res = gpio_add_callback(cfg->gpio_int1.port, &data->gpio_cb);

	if (res < 0) {
		LOG_ERR("Failed to set gpio callback");
		return res;
	}

	k_mutex_init(&data->mutex);
#if defined(CONFIG_ICM42688_TRIGGER_OWN_THREAD)
	k_sem_init(&data->gpio_sem, 0, K_SEM_MAX_LIMIT);
	k_thread_create(&data->thread, data->thread_stack, CONFIG_ICM42688_THREAD_STACK_SIZE,
			icm42688_thread, data, NULL, NULL,
			K_PRIO_COOP(CONFIG_ICM42688_THREAD_PRIORITY), 0, K_NO_WAIT);
#elif defined(CONFIG_ICM42688_TRIGGER_GLOBAL_THREAD)
	data->work.handler = icm42688_work_handler;
#endif
	return gpio_pin_interrupt_configure_dt(&cfg->gpio_int1, GPIO_INT_EDGE_TO_ACTIVE);
}

int icm42688_trigger_enable_interrupt(const struct device *dev, struct icm42688_cfg *new_cfg)
{
	int res;
	const struct icm42688_dev_cfg *cfg = dev->config;

	/* pulse-mode (auto clearing), push-pull and active-high */
	res = icm42688_spi_single_write(&cfg->spi, REG_INT_CONFIG,
					BIT_INT1_DRIVE_CIRCUIT | BIT_INT1_POLARITY);
	if (res != 0) {
		return res;
	}

	/* Deassert async reset for proper INT pin operation, see datasheet 14.50 */
	res = icm42688_spi_single_write(&cfg->spi, REG_INT_CONFIG1, 0);
	if (res != 0) {
		return res;
	}

	/* enable interrupts on INT1 pin */
	uint8_t value = 0;

	if (new_cfg->interrupt1_drdy) {
		value |= FIELD_PREP(BIT_UI_DRDY_INT1_EN, 1);
	}
	if (new_cfg->interrupt1_fifo_ths) {
		value |= FIELD_PREP(BIT_FIFO_THS_INT1_EN, 1);
	}
	if (new_cfg->interrupt1_fifo_full) {
		value |= FIELD_PREP(BIT_FIFO_FULL_INT1_EN, 1);
	}
	return icm42688_spi_single_write(&cfg->spi, REG_INT_SOURCE0, value);
}

void icm42688_lock(const struct device *dev)
{
	struct icm42688_dev_data *data = dev->data;

	k_mutex_lock(&data->mutex, K_FOREVER);
}

void icm42688_unlock(const struct device *dev)
{
	struct icm42688_dev_data *data = dev->data;

	k_mutex_unlock(&data->mutex);
}
