/*
 * Copyright (c) 2024 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/util.h>
#include "icm42370.h"
#include "icm42370_reg.h"
#ifdef CONFIG_I2C
	#include "icm42370_i2c.h"
#elif CONFIG_SPI
	#include "icm42370_spi.h"
#endif
#include "icm42370_trigger.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(ICM42370, CONFIG_SENSOR_LOG_LEVEL);

static void icm42370_gpio_callback(const struct device *dev, struct gpio_callback *cb,
				   uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pins);

	struct icm42370_data *data = CONTAINER_OF(cb, struct icm42370_data, gpio_cb);

#if defined(CONFIG_ICM42370_TRIGGER_OWN_THREAD)
	k_sem_give(&data->gpio_sem);
#elif defined(CONFIG_ICM42370_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
}

static void icm42370_thread_cb(const struct device *dev)
{
	struct icm42370_data *data = dev->data;
	const struct icm42370_config *cfg = dev->config;

	icm42370_lock(dev);
	gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_DISABLE);

	if (data->data_ready_handler) {
		data->data_ready_handler(dev, data->data_ready_trigger);
	}

	if (data->motion_handler != NULL) {
		icm42370_motion_fetch(dev);
	}

	gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_EDGE_TO_ACTIVE);
	icm42370_unlock(dev);
}

#if defined(CONFIG_ICM42370_TRIGGER_OWN_THREAD)

static void icm42370_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct icm42370_data *data = p1;

	while (1) {
		k_sem_take(&data->gpio_sem, K_FOREVER);
		icm42370_thread_cb(data->dev);
	}
}

#elif defined(CONFIG_ICM42370_TRIGGER_GLOBAL_THREAD)

static void icm42370_work_handler(struct k_work *work)
{
	struct icm42370_data *data = CONTAINER_OF(work, struct icm42370_data, work);

	icm42370_thread_cb(data->dev);
}

#endif

int icm42370_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler)
{
	int res = 0;
	struct icm42370_data *data = dev->data;
	const struct icm42370_config *cfg = dev->config;

	if (!handler) {
		printk("%s handler is null\n", __func__);
		return -EINVAL;
	}

	if (trig->type != SENSOR_TRIG_DATA_READY
	    && trig->type != SENSOR_TRIG_MOTION) {
		return -ENOTSUP;
	}

	icm42370_lock(dev);
	gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_DISABLE);

	switch (trig->type) {
	case SENSOR_TRIG_DATA_READY:
		data->data_ready_handler = handler;
		data->data_ready_trigger = trig;
		break;
	case SENSOR_TRIG_MOTION:
		data->motion_handler = handler;
		data->motion_trigger = trig;
		data->motion_en = true;
		break;
	default:
		printk("%s incorrect type\n", __func__);
		res = -ENOTSUP;
		break;
	}

	icm42370_unlock(dev);
	gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_EDGE_TO_ACTIVE);

	icm42370_turn_on_sensor(dev);

	return res;
}

int icm42370_trigger_init(const struct device *dev)
{
	struct icm42370_data *data = dev->data;
	const struct icm42370_config *cfg = dev->config;
	int res = 0;

	if (!cfg->gpio_int.port) {
		printk("trigger enabled but no interrupt gpio supplied");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&cfg->gpio_int)) {
		printk("gpio_int gpio not ready");
		return -ENODEV;
	}

	data->dev = dev;
	gpio_pin_configure_dt(&cfg->gpio_int, GPIO_INPUT);
	gpio_init_callback(&data->gpio_cb, icm42370_gpio_callback, BIT(cfg->gpio_int.pin));
	res = gpio_add_callback(cfg->gpio_int.port, &data->gpio_cb);

	if (res < 0) {
		printk("Failed to set gpio callback");
		return res;
	}

	k_mutex_init(&data->mutex);

#if defined(CONFIG_ICM42370_TRIGGER_OWN_THREAD)
	k_sem_init(&data->gpio_sem, 0, K_SEM_MAX_LIMIT);
	k_thread_create(&data->thread, data->thread_stack, CONFIG_ICM42370_THREAD_STACK_SIZE,
			icm42370_thread, data, NULL, NULL,
			K_PRIO_COOP(CONFIG_ICM42370_THREAD_PRIORITY), 0, K_NO_WAIT);
#elif defined(CONFIG_ICM42370_TRIGGER_GLOBAL_THREAD)
	data->work.handler = icm42370_work_handler;
#endif

	return gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_EDGE_TO_ACTIVE);
}

int icm42370_trigger_enable_interrupt(const struct device *dev)
{
	int res;
	const struct icm42370_config *cfg = dev->config;

	/* pulse-mode (auto clearing), push-pull and active-high */
	res = icm42370_single_write(&cfg->bus, REG_INT_CONFIG,
					BIT_INT1_DRIVE_CIRCUIT | BIT_INT1_POLARITY);

	if (res) {
		return res;
	}

	/* enable data ready interrupt on INT1 pin */
	return icm42370_single_write(&cfg->bus, REG_INT_SOURCE0, BIT_INT_DRDY_INT1_EN);
}

void icm42370_lock(const struct device *dev)
{
	struct icm42370_data *data = dev->data;

	k_mutex_lock(&data->mutex, K_FOREVER);
}

void icm42370_unlock(const struct device *dev)
{
	struct icm42370_data *data = dev->data;

	k_mutex_unlock(&data->mutex);
}
