/*
 * Copyright (c) 2022, Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_bq274xx

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#ifdef CONFIG_BQ274XX_PM
#include <zephyr/pm/device.h>
#endif

#include "bq274xx.h"

LOG_MODULE_DECLARE(bq274xx, CONFIG_SENSOR_LOG_LEVEL);

static void bq274xx_handle_interrupts(const struct device *dev)
{
	struct bq274xx_data *data = dev->data;

	if (data->ready_handler) {
		data->ready_handler(dev, data->ready_trig);
	}
}

#ifdef CONFIG_BQ274XX_TRIGGER_OWN_THREAD
static K_KERNEL_STACK_DEFINE(bq274xx_thread_stack, CONFIG_BQ274XX_THREAD_STACK_SIZE);
static struct k_thread bq274xx_thread;

static void bq274xx_thread_main(struct bq274xx_data *data)
{
	while (1) {
		k_sem_take(&data->sem, K_FOREVER);
		bq274xx_handle_interrupts(data->dev);
	}
}
#endif

#ifdef CONFIG_BQ274XX_TRIGGER_GLOBAL_THREAD
static void bq274xx_work_handler(struct k_work *work)
{
	struct bq274xx_data *data = CONTAINER_OF(work, struct bq274xx_data, work);

	bq274xx_handle_interrupts(data->dev);
}
#endif

static void bq274xx_ready_callback_handler(const struct device *port,
					   struct gpio_callback *cb,
					   gpio_port_pins_t pins)
{
	struct bq274xx_data *data = CONTAINER_OF(cb, struct bq274xx_data, ready_callback);

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

#if defined(CONFIG_BQ274XX_TRIGGER_OWN_THREAD)
	k_sem_give(&data->sem);
#elif defined(CONFIG_BQ274XX_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
}

int bq274xx_trigger_mode_init(const struct device *dev)
{
	const struct bq274xx_config *const config = dev->config;
	struct bq274xx_data *data = dev->data;
	int status = 0;

	data->dev = dev;

#if defined(CONFIG_BQ274XX_TRIGGER_OWN_THREAD)
	k_sem_init(&data->sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&bq274xx_thread, bq274xx_thread_stack,
			CONFIG_BQ274XX_THREAD_STACK_SIZE,
			(k_thread_entry_t)bq274xx_thread_main,
			data, NULL, NULL,
			K_PRIO_COOP(CONFIG_BQ274XX_THREAD_PRIORITY),
			0, K_NO_WAIT);
#elif defined(CONFIG_BQ274XX_TRIGGER_GLOBAL_THREAD)
	k_work_init(&data->work, bq274xx_work_handler);
#endif

	status = gpio_pin_configure_dt(&config->int_gpios, GPIO_INPUT);
	if (status < 0) {
		LOG_ERR("Unable to configure interrupt pin to input");
		return status;
	}
	gpio_init_callback(&data->ready_callback,
			   bq274xx_ready_callback_handler,
			   BIT(config->int_gpios.pin));

	return 0;
}

int bq274xx_trigger_set(const struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
	const struct bq274xx_config *config = dev->config;
	struct bq274xx_data *data = dev->data;
	int status;

#ifdef CONFIG_BQ274XX_PM
	enum pm_device_state state;

	(void)pm_device_state_get(dev, &state);
	if (state != PM_DEVICE_STATE_ACTIVE) {
		return -EBUSY;
	}
#endif

	if (trig->type != SENSOR_TRIG_DATA_READY) {
		return -ENOTSUP;
	}

	if (!device_is_ready(config->int_gpios.port)) {
		LOG_ERR("GPIO device pointer is not ready to be used");
		return -ENODEV;
	}

	data->ready_handler = handler;
	data->ready_trig = trig;

	if (handler) {
		status = gpio_pin_configure_dt(&config->int_gpios, GPIO_INPUT);
		if (status < 0) {
			LOG_ERR("Unable to configure interrupt pin to input (%d)", status);
			return status;
		}

		status = gpio_add_callback(config->int_gpios.port, &data->ready_callback);
		if (status < 0) {
			LOG_ERR("Unable to add interrupt callback (%d)", status);
			return status;
		}

		status = gpio_pin_interrupt_configure_dt(&config->int_gpios,
							 GPIO_INT_EDGE_TO_ACTIVE);
		if (status < 0) {
			LOG_ERR("Unable to configure interrupt (%d)", status);
			return status;
		}
	} else {
		status = gpio_remove_callback(config->int_gpios.port, &data->ready_callback);
		if (status < 0) {
			LOG_ERR("Unable to remove interrupt callback (%d)", status);
			return status;
		}

		status = gpio_pin_interrupt_configure_dt(&config->int_gpios, GPIO_INT_DISABLE);
		if (status < 0) {
			LOG_ERR("Unable to configure interrupt (%d)", status);
			return status;
		}
	}

	return 0;
}
