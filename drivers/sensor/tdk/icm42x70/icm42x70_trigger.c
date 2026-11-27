/*
 * Copyright (c) 2024 TDK Invensense
 * Copyright (c) 2022 Esco Medical ApS
 * Copyright (c) 2016 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/util.h>
#include "icm42x70.h"
#include "icm42x70_trigger.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(ICM42X70, CONFIG_SENSOR_LOG_LEVEL);

static void icm42x70_gpio_callback(const struct device *dev, struct gpio_callback *cb,
				   uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pins);

	struct icm42x70_data *data = CONTAINER_OF(cb, struct icm42x70_data, gpio_cb);

#if defined(CONFIG_ICM42X70_TRIGGER_OWN_THREAD)
	k_sem_give(&data->gpio_sem);
#elif defined(CONFIG_ICM42X70_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
}

static void icm42x70_thread_cb(const struct device *dev)
{
	struct icm42x70_data *data = dev->data;
	const struct icm42x70_config *cfg = dev->config;
	uint8_t int_status[4];
	int rc = 0;

	icm42x70_lock(dev);
	gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_DISABLE);

	rc |= inv_imu_read_reg(&data->driver, INT_STATUS_DRDY, 4, int_status);
	if (data->data_ready_handler) {
#ifdef CONFIG_ICM42X70_TRIGGER
		data->int_status = int_status[1];
#else
		data->int_status = int_status[0];
#endif
		if (data->int_status != 0) {
			data->data_ready_handler(dev, data->data_ready_trigger);
		}
	}
	if (data->apex_ready_handler) {
		/* Read APEX interrupt status */
		if ((int_status[2] != 0) || (int_status[3] != 0)) {
			data->int_status2 = int_status[2];
			data->int_status3 = int_status[3];
			data->apex_ready_handler(dev, data->apex_ready_trigger);
		}
	}
	gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_EDGE_TO_ACTIVE);
	icm42x70_unlock(dev);
}

#if defined(CONFIG_ICM42X70_TRIGGER_OWN_THREAD)

static void icm42x70_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct icm42x70_data *data = p1;

	while (1) {
		k_sem_take(&data->gpio_sem, K_FOREVER);
		icm42x70_thread_cb(data->dev);
	}
}
#elif defined(CONFIG_ICM42X70_TRIGGER_GLOBAL_THREAD)

static void icm42x70_work_handler(struct k_work *work)
{
	struct icm42x70_data *data = CONTAINER_OF(work, struct icm42x70_data, work);

	icm42x70_thread_cb(data->dev);
}

#endif

int icm42x70_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler)
{
	struct icm42x70_data *data = dev->data;
	const struct icm42x70_config *cfg = dev->config;

	if (!handler) {
		return -EINVAL;
	}

	icm42x70_lock(dev);
	gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_DISABLE);

	if (trig->type == SENSOR_TRIG_DATA_READY) {
		data->data_ready_handler = handler;
		data->data_ready_trigger = trig;
#ifdef CONFIG_TDK_APEX
	} else if (trig->type == SENSOR_TRIG_MOTION) {
		data->apex_ready_handler = handler;
		data->apex_ready_trigger = trig;
#endif
	} else {
		LOG_ERR("ERROR While set trigger");
		return -ENOTSUP;
	}

	icm42x70_unlock(dev);
	gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_EDGE_TO_ACTIVE);

	return 0;
}

int icm42x70_trigger_init(const struct device *dev)
{
	struct icm42x70_data *data = dev->data;
	const struct icm42x70_config *cfg = dev->config;
	int res = 0;

	if (!cfg->gpio_int.port) {
		LOG_ERR("trigger enabled but no interrupt gpio supplied");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&cfg->gpio_int)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->gpio_int.port);
		return -ENODEV;
	}

	data->dev = dev;
	gpio_pin_configure_dt(&cfg->gpio_int, GPIO_INPUT);
	gpio_init_callback(&data->gpio_cb, icm42x70_gpio_callback, BIT(cfg->gpio_int.pin));
	res = gpio_add_callback(cfg->gpio_int.port, &data->gpio_cb);

	if (res < 0) {
		LOG_ERR("Failed to set gpio callback");
		return res;
	}

	k_mutex_init(&data->mutex);

#if defined(CONFIG_ICM42X70_TRIGGER_OWN_THREAD)
	k_sem_init(&data->gpio_sem, 0, K_SEM_MAX_LIMIT);
	k_thread_create(&data->thread, data->thread_stack, CONFIG_ICM42X70_THREAD_STACK_SIZE,
			icm42x70_thread, data, NULL, NULL,
			K_PRIO_COOP(CONFIG_ICM42X70_THREAD_PRIORITY), 0, K_NO_WAIT);
#elif defined(CONFIG_ICM42X70_TRIGGER_GLOBAL_THREAD)
	data->work.handler = icm42x70_work_handler;
#endif

	return gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_EDGE_TO_INACTIVE);
}

int icm42x70_trigger_enable_interrupt(struct icm42x70_data *drv_data)
{
	int err = 0;
	inv_imu_int1_pin_config_t int1_pin_config;
	inv_imu_interrupt_parameter_t config_int;

	err |= inv_imu_get_config_int1(&drv_data->driver, &config_int);

	/* Set interrupt config */
	int1_pin_config.int_polarity = INT_CONFIG_INT1_POLARITY_HIGH;
	int1_pin_config.int_mode = INT_CONFIG_INT1_MODE_PULSED;
	int1_pin_config.int_drive = INT_CONFIG_INT1_DRIVE_CIRCUIT_PP;
	err |= inv_imu_set_pin_config_int1(&drv_data->driver, &int1_pin_config);

	config_int.INV_FIFO_THS = INV_IMU_ENABLE;
	err |= inv_imu_set_config_int1(&drv_data->driver, &config_int);
	err |= inv_imu_configure_fifo(&drv_data->driver, INV_IMU_FIFO_ENABLED);

	return err;
}

int icm42x70_trigger_disable_interrupt(struct icm42x70_data *drv_data)
{
	int err = 0;
	inv_imu_interrupt_parameter_t config_int;

	err |= inv_imu_get_config_int1(&drv_data->driver, &config_int);

	config_int.INV_FIFO_THS = INV_IMU_DISABLE;
	err |= inv_imu_set_config_int1(&drv_data->driver, &config_int);
	err |= inv_imu_configure_fifo(&drv_data->driver, INV_IMU_FIFO_DISABLED);

	return err;
}

void icm42x70_lock(const struct device *dev)
{
	struct icm42x70_data *data = dev->data;

	k_mutex_lock(&data->mutex, K_FOREVER);
}

void icm42x70_unlock(const struct device *dev)
{
	struct icm42x70_data *data = dev->data;

	k_mutex_unlock(&data->mutex);
}
