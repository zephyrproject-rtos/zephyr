/*
 * Copyright (c) 2022 Intel Corporation
 * Copyright (c) 2023 Google LLC
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>

#include "icm45686.h"
#include "icm45686_bus.h"
#include "icm45686_reg.h"
#include "icm45686_trigger.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ICM45686_TRIGGER, CONFIG_SENSOR_LOG_LEVEL);

static void icm45686_gpio_callback(const struct device *dev, struct gpio_callback *cb,
				   uint32_t pins)
{
	struct icm45686_data *data = CONTAINER_OF(cb, struct icm45686_data, triggers.cb);

	ARG_UNUSED(dev);
	ARG_UNUSED(pins);

#if defined(CONFIG_ICM45686_TRIGGER_OWN_THREAD)
	k_sem_give(&data->triggers.sem);
#elif defined(CONFIG_ICM45686_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->triggers.work);
#endif
}

static void icm45686_thread_cb(const struct device *dev)
{
	struct icm45686_data *data = dev->data;

	(void)k_mutex_lock(&data->triggers.lock, K_FOREVER);

	if (data->triggers.entry.handler) {
		data->triggers.entry.handler(dev, data->triggers.entry.trigger);
	}

	(void)k_mutex_unlock(&data->triggers.lock);
}

#if defined(CONFIG_ICM45686_TRIGGER_OWN_THREAD)

static void icm45686_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct icm45686_data *data = p1;

	while (true) {
		k_sem_take(&data->triggers.sem, K_FOREVER);

		icm45686_thread_cb(data->triggers.dev);
	}
}

#elif defined(CONFIG_ICM45686_TRIGGER_GLOBAL_THREAD)

static void icm45686_work_handler(struct k_work *work)
{
	struct icm45686_data *data = CONTAINER_OF(work, struct icm45686_data, triggers.work);

	icm45686_thread_cb(data->triggers.dev);
}

#endif

int icm45686_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler)
{
	int err = 0;
	struct icm45686_data *data = dev->data;
	inv_imu_int_state_t int_config;

	(void)k_mutex_lock(&data->triggers.lock, K_FOREVER);
	memset(&int_config, INV_IMU_DISABLE, sizeof(int_config));

	switch (trig->type) {
	case SENSOR_TRIG_DATA_READY:
		data->triggers.entry.trigger = trig;
		data->triggers.entry.handler = handler;

		if (handler) {
			/* Enable data ready interrupt */
			int_config.INV_FIFO_THS = INV_IMU_ENABLE;
			int_config.INV_UI_DRDY = INV_IMU_ENABLE;
		}
		err = icm456xx_set_config_int(&data->driver, INV_IMU_INT1, &int_config);
		break;
	case SENSOR_TRIG_MOTION:
		data->triggers.entry.trigger = trig;
		data->triggers.entry.handler = handler;
		break;
	default:
		err = -ENOTSUP;
		break;
	}

	(void)k_mutex_unlock(&data->triggers.lock);

	return err;
}

int icm45686_trigger_init(const struct device *dev)
{
	const struct icm45686_config *cfg = dev->config;
	struct icm45686_data *data = dev->data;
	inv_imu_int_pin_config_t int_pin_config;
	int err;

	err = k_mutex_init(&data->triggers.lock);
	__ASSERT_NO_MSG(!err);

	/** Needed to get back the device handle from the callback context */
	data->triggers.dev = dev;

#if defined(CONFIG_ICM45686_TRIGGER_OWN_THREAD)

	err = k_sem_init(&data->triggers.sem, 0, 1);
	__ASSERT_NO_MSG(!err);

	(void)k_thread_create(&data->triggers.thread, data->triggers.thread_stack,
			      K_KERNEL_STACK_SIZEOF(data->triggers.thread_stack), icm45686_thread,
			      data, NULL, NULL, K_PRIO_COOP(CONFIG_ICM45686_THREAD_PRIORITY), 0,
			      K_NO_WAIT);

#elif defined(CONFIG_ICM45686_TRIGGER_GLOBAL_THREAD)
	k_work_init(&data->triggers.work, icm45686_work_handler);
#endif

	if (!cfg->int_gpio.port) {
		LOG_ERR("Interrupt GPIO not supplied");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&cfg->int_gpio)) {
		LOG_ERR("Interrupt GPIO not ready");
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&cfg->int_gpio, GPIO_INPUT);
	if (err) {
		LOG_ERR("Failed to configure interrupt GPIO");
		return -EIO;
	}

	gpio_init_callback(&data->triggers.cb, icm45686_gpio_callback, BIT(cfg->int_gpio.pin));

	err = gpio_add_callback(cfg->int_gpio.port, &data->triggers.cb);
	if (err) {
		LOG_ERR("Failed to add interrupt callback");
		return -EIO;
	}

	err = gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (err) {
		LOG_ERR("Failed to configure interrupt");
	}

	/*
	 * Configure interrupts pins
	 * - Polarity High
	 * - Pulse mode
	 * - Push-Pull drive
	 */
	int_pin_config.int_polarity = INTX_CONFIG2_INTX_POLARITY_HIGH;
	int_pin_config.int_mode = INTX_CONFIG2_INTX_MODE_PULSE;
	int_pin_config.int_drive = INTX_CONFIG2_INTX_DRIVE_PP;
	err = icm456xx_set_pin_config_int(&data->driver, INV_IMU_INT1, &int_pin_config);

	return err;
}
