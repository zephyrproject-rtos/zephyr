/*
 * Copyright (c) 2021, Silvano Cortesi
 * Copyright (c) 2021, ETH Zürich
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT maxim_max30101

#include <kernel.h>
#include <drivers/sensor.h>
#include <drivers/gpio.h>
#include <logging/log.h>
#include <sys/__assert.h>
#include "max30101.h"

LOG_MODULE_DECLARE(MAX30101, CONFIG_SENSOR_LOG_LEVEL);

static int setup_int(const struct device *dev, bool enable)
{
	const struct max30101_config *config = dev->config;

	gpio_flags_t flags = enable ? GPIO_INT_EDGE_TO_ACTIVE : GPIO_INT_DISABLE;

	return gpio_pin_interrupt_configure_dt(&config->gpio_int, flags);
}

static void handle_int(const struct device *dev)
{
	int ret = setup_int(dev, false);

	if (ret < 0) {
		LOG_ERR("Failed to configure interrupt!");
		return;
	}

	struct max30101_data *data = dev->data;

#if defined(CONFIG_MAX30101_TRIGGER_OWN_THREAD)
	k_sem_give(&data->gpio_sem);
#elif defined(CONFIG_MAX30101_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
}

/* function which actually processes interrupt asynchronous */
static void process_int(const struct device *dev)
{
	struct max30101_data *data = dev->data;
	const struct max30101_config *config = dev->config;

	/* Read the status (and implicitly clear it) */
	int ret = i2c_reg_read_byte_dt(&config->bus, MAX30101_REG_INT_STS1, &data->int_sts1);

	if (ret < 0) {
		LOG_ERR("Failed to read status!");
		return;
	}
	ret = i2c_reg_read_byte_dt(&config->bus, MAX30101_REG_INT_STS2, &data->int_sts2);
	if (ret < 0) {
		LOG_ERR("Failed to read status!");
		return;
	}

	/* Check for interrupt and call correct handler(s)*/
	if (data->int_sts1 & (MAX30101_INT_ALC_OVF_MASK)) {
		if (data->th_handler[0] != NULL) {
			data->th_handler[0](dev, &data->th_trigger[0]);
		}
	}
	if (data->int_sts1 & MAX30101_INT_PPG_RDY_MASK) {
		if (data->th_handler[1] != NULL) {
			data->th_handler[1](dev, &data->th_trigger[1]);
		}
	}
	if (data->int_sts1 & MAX30101_INT_A_FULL_MASK) {
		if (data->th_handler[2] != NULL) {
			data->th_handler[2](dev, &data->th_trigger[2]);
		}
	}

	ret = setup_int(dev, true);
	if (ret < 0) {
		LOG_ERR("Failed to configure interrupt!");
		return;
	}

	/* Check for pin that asserted while we were offline */
	int pv = gpio_pin_get_dt(&config->gpio_int);

	if (pv > 0) {
		handle_int(dev);
	}
}

static void max30101_gpio_callback(const struct device *dev, struct gpio_callback *cb,
				uint32_t pins)
{
	struct max30101_data *data = CONTAINER_OF(cb, struct max30101_data, gpio_cb);

	handle_int(data->dev);
}

#if defined(CONFIG_MAX30101_TRIGGER_OWN_THREAD)
static void max30101_thread(struct max30101_data *data)
{
	while (true) {
		k_sem_take(&data->gpio_sem, K_FOREVER);
		process_int(data->dev);
	}
}

#elif defined(CONFIG_MAX30101_TRIGGER_GLOBAL_THREAD)
static void max30101_work_cb(struct k_work *work)
{
	struct max30101_data *data = CONTAINER_OF(work, struct max30101_data, work);

	process_int(data->dev);
}
#endif

int max30101_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
				sensor_trigger_handler_t handler)
{
	struct max30101_data *data = dev->data;
	const struct max30101_config *config = dev->config;

	int ret = setup_int(dev, false);

	if (ret < 0) {
		LOG_ERR("Failed to configure interrupt!");
		return ret;
	}

	uint8_t trigger_number = 0;
	uint8_t bitmask[2] = {0, 0};

	if (handler != NULL) {
		switch (trig->type) {
		case SENSOR_TRIG_DELTA:
			trigger_number = 0;
			bitmask[0] = MAX30101_INT_ALC_OVF_MASK;
			break;
		case SENSOR_TRIG_DATA_READY:
			trigger_number = 1;
			bitmask[0] = MAX30101_INT_PPG_RDY_MASK;
			break;
		case SENSOR_TRIG_FIFO:
			trigger_number = 2;
			bitmask[0] = MAX30101_INT_A_FULL_MASK;
			break;
		default:
			LOG_ERR("Unsupported sensor trigger");
			return -ENOTSUP;
		}

		data->th_handler[trigger_number] = handler;
		data->th_trigger[trigger_number] = *trig;

		/* Enable interrupt */
		ret = i2c_reg_update_byte_dt(&config->bus, MAX30101_REG_INT_EN1, bitmask[0], bitmask[0]);
		if (ret < 0) {
			return ret;
		}
		ret = i2c_reg_update_byte_dt(&config->bus, MAX30101_REG_INT_EN2, bitmask[1], bitmask[1]);
		if (ret < 0) {
			return ret;
		}
	}

	ret = setup_int(dev, true);
	if (ret < 0) {
		LOG_ERR("Failed to configure interrupt!");
		return ret;
	}

	/* Check whether already asserted */
	int pv = gpio_pin_get_dt(&config->gpio_int);

	if (pv > 0) {
		handle_int(dev);
	}

	return 0;
}

int max30101_init_interrupt(const struct device *dev)
{
	struct max30101_data *data = dev->data;
	const struct max30101_config *config = dev->config;

	data->int_sts1 = 0;
	data->int_sts2 = 0;

	if (!device_is_ready(config->gpio_int.port)) {
		LOG_ERR("GPIO device pointer is not ready!");
		return -ENODEV;
	}

	int ret = gpio_pin_configure_dt(&config->gpio_int, GPIO_INPUT);

	if (ret < 0) {
		LOG_ERR("Failed to set gpio as input!");
		return ret;
	}

	gpio_init_callback(&data->gpio_cb, max30101_gpio_callback, BIT(config->gpio_int.pin));

	ret = gpio_add_callback(config->gpio_int.port, &data->gpio_cb);
	if (ret < 0) {
		LOG_ERR("Failed to add gpio callback!");
		return ret;
	}

	data->dev = dev;

#if defined(CONFIG_MAX30101_TRIGGER_OWN_THREAD)
	k_sem_init(&data->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&data->thread, data->thread_stack, CONFIG_MAX30101_THREAD_STACK_SIZE,
				(k_thread_entry_t)max30101_thread, data, NULL, NULL,
				K_PRIO_COOP(CONFIG_MAX30101_THREAD_PRIORITY), 0, K_NO_WAIT);
#elif defined(CONFIG_MAX30101_TRIGGER_GLOBAL_THREAD)
	data->work.handler = max30101_work_cb;
#endif

	return 0;
}
