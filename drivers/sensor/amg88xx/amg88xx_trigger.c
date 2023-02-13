/*
 * Copyright (c) 2017-2019 Phytec Messtechnik GmbH
 * Copyright (c) 2017 Benedict Ohl (Benedict-Ohl@web.de)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include "amg88xx.h"

extern struct amg88xx_data amg88xx_driver;

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(AMG88XX, CONFIG_SENSOR_LOG_LEVEL);

static inline void amg88xx_setup_int(const struct amg88xx_config *cfg,
				     bool enable)
{
	unsigned int flags = enable
		? GPIO_INT_EDGE_TO_ACTIVE
		: GPIO_INT_DISABLE;

	gpio_pin_interrupt_configure_dt(&cfg->int_gpio, flags);
}

int amg88xx_attr_set(const struct device *dev,
		     enum sensor_channel chan,
		     enum sensor_attribute attr,
		     const struct sensor_value *val)
{
	const struct amg88xx_config *config = dev->config;
	int16_t int_level = (val->val1 * 1000000 + val->val2) /
			  AMG88XX_TREG_LSB_SCALING;
	uint8_t intl_reg;
	uint8_t inth_reg;

	if (!config->int_gpio.port) {
		return -ENOTSUP;
	}

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	LOG_DBG("set threshold to %d", int_level);

	if (attr == SENSOR_ATTR_UPPER_THRESH) {
		intl_reg = AMG88XX_INTHL;
		inth_reg = AMG88XX_INTHH;
	} else if (attr == SENSOR_ATTR_LOWER_THRESH) {
		intl_reg = AMG88XX_INTLL;
		inth_reg = AMG88XX_INTLH;
	} else {
		return -ENOTSUP;
	}

	if (i2c_reg_write_byte_dt(&config->i2c,
			       intl_reg, (uint8_t)int_level)) {
		LOG_DBG("Failed to set INTxL attribute!");
		return -EIO;
	}

	if (i2c_reg_write_byte_dt(&config->i2c,
			       inth_reg, (uint8_t)(int_level >> 8))) {
		LOG_DBG("Failed to set INTxH attribute!");
		return -EIO;
	}

	return 0;
}

static void amg88xx_gpio_callback(const struct device *dev,
				  struct gpio_callback *cb, uint32_t pins)
{
	struct amg88xx_data *drv_data =
		CONTAINER_OF(cb, struct amg88xx_data, gpio_cb);
	const struct amg88xx_config *config = drv_data->dev->config;

	amg88xx_setup_int(config, false);

#if defined(CONFIG_AMG88XX_TRIGGER_OWN_THREAD)
	k_sem_give(&drv_data->gpio_sem);
#elif defined(CONFIG_AMG88XX_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&drv_data->work);
#endif
}

static void amg88xx_thread_cb(const struct device *dev)
{
	struct amg88xx_data *drv_data = dev->data;
	const struct amg88xx_config *config = dev->config;
	uint8_t status;

	if (i2c_reg_read_byte_dt(&config->i2c,
			      AMG88XX_STAT, &status) < 0) {
		return;
	}

	if (drv_data->drdy_handler != NULL) {
		drv_data->drdy_handler(dev, &drv_data->drdy_trigger);
	}

	if (drv_data->th_handler != NULL) {
		drv_data->th_handler(dev, &drv_data->th_trigger);
	}

	amg88xx_setup_int(config, true);
}

#ifdef CONFIG_AMG88XX_TRIGGER_OWN_THREAD
static void amg88xx_thread(struct amg88xx_data *drv_data)
{
	while (42) {
		k_sem_take(&drv_data->gpio_sem, K_FOREVER);
		amg88xx_thread_cb(drv_data->dev);
	}
}
#endif

#ifdef CONFIG_AMG88XX_TRIGGER_GLOBAL_THREAD
static void amg88xx_work_cb(struct k_work *work)
{
	struct amg88xx_data *drv_data =
		CONTAINER_OF(work, struct amg88xx_data, work);
	amg88xx_thread_cb(drv_data->dev);
}
#endif

int amg88xx_trigger_set(const struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
	struct amg88xx_data *drv_data = dev->data;
	const struct amg88xx_config *config = dev->config;

	if (!config->int_gpio.port) {
		return -ENOTSUP;
	}

	if (i2c_reg_write_byte_dt(&config->i2c,
			       AMG88XX_INTC, AMG88XX_INTC_DISABLED)) {
		return -EIO;
	}

	amg88xx_setup_int(config, false);

	if (trig->type == SENSOR_TRIG_THRESHOLD) {
		drv_data->th_handler = handler;
		drv_data->th_trigger = *trig;
	} else {
		LOG_ERR("Unsupported sensor trigger");
		return -ENOTSUP;
	}

	amg88xx_setup_int(config, true);

	if (i2c_reg_write_byte_dt(&config->i2c,
			       AMG88XX_INTC, AMG88XX_INTC_ABS_MODE)) {
		return -EIO;
	}

	return 0;
}

int amg88xx_init_interrupt(const struct device *dev)
{
	struct amg88xx_data *drv_data = dev->data;
	const struct amg88xx_config *config = dev->config;

	if (!device_is_ready(config->int_gpio.port)) {
		LOG_ERR("%s: device %s is not ready", dev->name,
				config->int_gpio.port->name);
		return -ENODEV;
	}

	gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT | config->int_gpio.dt_flags);

	gpio_init_callback(&drv_data->gpio_cb,
			   amg88xx_gpio_callback,
			   BIT(config->int_gpio.pin));

	if (gpio_add_callback(config->int_gpio.port, &drv_data->gpio_cb) < 0) {
		LOG_DBG("Failed to set gpio callback!");
		return -EIO;
	}

	drv_data->dev = dev;

#if defined(CONFIG_AMG88XX_TRIGGER_OWN_THREAD)
	k_sem_init(&drv_data->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&drv_data->thread, drv_data->thread_stack,
			CONFIG_AMG88XX_THREAD_STACK_SIZE,
			(k_thread_entry_t)amg88xx_thread, drv_data,
			NULL, NULL, K_PRIO_COOP(CONFIG_AMG88XX_THREAD_PRIORITY),
			0, K_NO_WAIT);
#elif defined(CONFIG_AMG88XX_TRIGGER_GLOBAL_THREAD)
	drv_data->work.handler = amg88xx_work_cb;
#endif
	amg88xx_setup_int(config, true);

	return 0;
}
