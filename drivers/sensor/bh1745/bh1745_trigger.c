/*
 * Copyright (c) 2019, 2022 Nordic Semiconductor ASA
 * Copyright (c) 2022 Grzegorz Blach
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <drivers/gpio.h>
#include <drivers/i2c.h>
#include <sys/util.h>
#include <kernel.h>
#include <drivers/sensor.h>
#include "bh1745.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(BH1745_TRIGGER, CONFIG_SENSOR_LOG_LEVEL);

#define DT_DRV_COMPAT rohm_bh1745

/* Callback for active sense pin from BH1745 */
static void bh1745_gpio_callback(const struct device *gpio_dev,
				 struct gpio_callback *cb,
				 uint32_t pins)
{
	struct bh1745_data *data = CONTAINER_OF(cb, struct bh1745_data,
						gpio_cb);
	const struct device *bh1745_dev = data->dev;
	const struct bh1745_config *config = bh1745_dev->config;

	gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_DISABLE);
	k_work_submit(&data->work);
}

static void bh1745_work_cb(struct k_work *work)
{
	struct bh1745_data *data = CONTAINER_OF(work, struct bh1745_data,
						work);
	const struct device *dev = data->dev;

	if (data->trg_handler != NULL) {
		data->trg_handler(dev, &data->trigger);
	}
}

/* Set sensor trigger attributes */
int bh1745_attr_set(const struct device *dev,
		    enum sensor_channel chan,
		    enum sensor_attribute attr,
		    const struct sensor_value *val)
{
	const struct bh1745_config *config = dev->config;
	const struct i2c_dt_spec *i2c = &config->i2c;
	int err;

	if (chan != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	if (attr == SENSOR_ATTR_UPPER_THRESH) {
		err = i2c_reg_write_byte_dt(i2c, BH1745_TH_HIGH_LSB,
					    (uint8_t)val->val1);
		if (err < 0) {
			LOG_ERR("Could not set upper threshold: %d", err);
			return err;
		}

		err = i2c_reg_write_byte_dt(i2c, BH1745_TH_HIGH_MSB,
					    (uint8_t)(val->val1 >> 8));
		if (err < 0) {
			LOG_ERR("Could not set upper threshold: %d", err);
			return err;
		}
		return 0;
	}

	if (attr == SENSOR_ATTR_LOWER_THRESH) {
		err = i2c_reg_write_byte_dt(i2c, BH1745_TH_LOW_LSB,
					    (uint8_t)val->val1);
		if (err < 0) {
			LOG_ERR("Could not set lower threshold: %d", err);
			return err;
		}

		err = i2c_reg_write_byte_dt(i2c, BH1745_TH_LOW_MSB,
					    (uint8_t)(val->val1 >> 8));
		if (err < 0) {
			LOG_ERR("Could not set lower threshold: %d", err);
			return err;
		}
		return 0;
	}
	return -ENOTSUP;
}

int bh1745_trigger_set(const struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler)
{
	struct bh1745_data *data = dev->data;
	const struct bh1745_config *config = dev->config;
	const struct i2c_dt_spec *i2c = &config->i2c;
	uint8_t interrupt_source = 0;
	int err;

	err = gpio_pin_interrupt_configure_dt(&config->int_gpio,
					      GPIO_INT_DISABLE);
	if (err < 0) {
		return err;
	}

	switch (trig->type) {
	case SENSOR_TRIG_THRESHOLD:
		err = i2c_reg_update_byte_dt(i2c, BH1745_PERSISTENCE,
				     BH1745_PERSISTENCE_PERSISTENCE_Msk,
				     BH1745_PERSISTENCE_PERSISTENCE_8_SAMPLES);
		if (err < 0) {
			LOG_ERR("Unable to set threshold persistence: %d",
				err);
			return err;
		}
		if (trig->chan == SENSOR_CHAN_RED) {
			interrupt_source = BH1745_INTERRUPT_INT_SOURCE_RED;
		} else if (trig->chan == SENSOR_CHAN_GREEN) {
			interrupt_source = BH1745_INTERRUPT_INT_SOURCE_GREEN;
		} else if (trig->chan == SENSOR_CHAN_BLUE) {
			interrupt_source = BH1745_INTERRUPT_INT_SOURCE_BLUE;
		} else {
			LOG_ERR("Unsupported interrupt source");
			return -ENOTSUP;
		}
		break;
	case SENSOR_TRIG_DATA_READY:
		err = i2c_reg_update_byte_dt(i2c, BH1745_PERSISTENCE,
				BH1745_PERSISTENCE_PERSISTENCE_Msk,
				BH1745_PERSISTENCE_PERSISTENCE_ACTIVE_END);
		if (err < 0) {
			LOG_ERR("Unable to set data ready trigger: %d", err);
			return err;
		}
		break;
	default:
		LOG_ERR("Unsupported sensor trigger");
		return -ENOTSUP;
	}

	err = i2c_reg_update_byte_dt(i2c, BH1745_INTERRUPT,
				     BH1745_INTERRUPT_ENABLE_Msk |
				     BH1745_INTERRUPT_INT_SOURCE_Msk,
				     BH1745_INTERRUPT_ENABLE_ENABLE |
				     interrupt_source);
	if (err < 0) {
		LOG_ERR("Interrupts could not be enabled.");
		return err;
	}

	data->trg_handler = handler;
	data->trigger = *trig;

	return gpio_pin_interrupt_configure_dt(&config->int_gpio,
					       GPIO_INT_LEVEL_LOW);
}

/* Enabling GPIO sense on BH1745 INT pin. */
int bh1745_gpio_interrupt_init(const struct device *dev)
{
	struct bh1745_data *data = dev->data;
	const struct bh1745_config *config = dev->config;
	const struct gpio_dt_spec *int_gpio = &config->int_gpio;
	int err;

	/* Setup gpio interrupt */
	if (!device_is_ready(int_gpio->port)) {
		LOG_ERR("GPIO device %s is not ready", int_gpio->port->name);
		return -ENODEV;
	}

	k_work_init(&data->work, bh1745_work_cb);

	err = gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);
	if (err < 0) {
		LOG_ERR("Failed to configure interrupt GPIO: %d", err);
		return err;
	}

	gpio_init_callback(&data->gpio_cb, bh1745_gpio_callback,
			   BIT(int_gpio->pin));

	err = gpio_add_callback(int_gpio->port, &data->gpio_cb);
	if (err < 0) {
		LOG_ERR("Failed to set GPIO callback: %d", err);
		return err;
	}

	err = gpio_pin_interrupt_configure_dt(&config->int_gpio,
					      GPIO_INT_LEVEL_ACTIVE);
	if (err < 0) {
		LOG_ERR("Failed to configure interrupt: %d", err);
	}

	return err;
}
