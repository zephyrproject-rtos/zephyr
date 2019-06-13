/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <gpio.h>
#include <i2c.h>
#include <misc/util.h>
#include <kernel.h>
#include <sensor.h>
#include "bh1749.h"

extern struct bh1749_data bh1749_driver;

#include <logging/log.h>
LOG_MODULE_REGISTER(BH1749_TRIGGER, CONFIG_SENSOR_LOG_LEVEL);

/* Callback for active sense pin from BH1749 */
static void bh1749_gpio_callback(struct device *dev, struct gpio_callback *cb,
				 u32_t pins)
{
	struct bh1749_data *drv_data =
		CONTAINER_OF(cb, struct bh1749_data, gpio_cb);

	gpio_pin_disable_callback(dev, DT_ROHM_BH1749_0_INT_GPIOS_PIN);

	k_work_submit(&drv_data->work);
}

void bh1749_work_cb(struct k_work *work)
{
	struct bh1749_data *data = CONTAINER_OF(work,
						struct bh1749_data,
						work);
	struct device *dev = data->dev;

	if (data->trg_handler != NULL) {
		data->trg_handler(dev, &data->trigger);
	}
}

/* Set sensor trigger attributes */
int bh1749_attr_set(struct device *dev,
		    enum sensor_channel chan,
		    enum sensor_attribute attr,
		    const struct sensor_value *val)
{
	int err;
	struct bh1749_data *data = dev->driver_data;

	if (chan != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	if (attr == SENSOR_ATTR_UPPER_THRESH) {
		err = i2c_reg_write_byte(data->i2c,
					 DT_ROHM_BH1749_0_BASE_ADDRESS,
					 BH1749_TH_HIGH_LSB,
					 (u8_t)val->val1);
		if (err) {
			LOG_ERR("Could not set threshold, error: %d", err);
			return err;
		}

		err = i2c_reg_write_byte(data->i2c,
					 DT_ROHM_BH1749_0_BASE_ADDRESS,
					 BH1749_TH_HIGH_MSB,
					 (u8_t)(val->val1 >> 8));
		if (err) {
			LOG_ERR("Could not set threshold, error: %d", err);
			return err;
		}
		return 0;
	}

	if (attr == SENSOR_ATTR_LOWER_THRESH) {
		err = i2c_reg_write_byte(data->i2c,
					 DT_ROHM_BH1749_0_BASE_ADDRESS,
					 BH1749_TH_LOW_LSB,
					 (u8_t)val->val1);
		if (err) {
			LOG_ERR("Could not set threshold, error: %d", err);
			return err;
		}

		err = i2c_reg_write_byte(data->i2c,
					 DT_ROHM_BH1749_0_BASE_ADDRESS,
					 BH1749_TH_LOW_MSB,
					 (u8_t)(val->val1 >> 8));
		if (err) {
			LOG_ERR("Could not set threshold, error: %d", err);
			return err;
		}
		return 0;
	}
	return 0;
}

int bh1749_trigger_set(struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler)
{
	struct bh1749_data *data = dev->driver_data;

	gpio_pin_disable_callback(data->gpio, DT_ROHM_BH1749_0_INT_GPIOS_PIN);

	switch (trig->type) {
	case SENSOR_TRIG_THRESHOLD:
		if (trig->chan == SENSOR_CHAN_RED) {
			if (i2c_reg_update_byte(data->i2c,
						DT_ROHM_BH1749_0_BASE_ADDRESS,
						BH1749_INTERRUPT,
						BH1749_INTERRUPT_INT_SOURCE_Msk,
						BH1749_INTERRUPT_INT_SOURCE_RED)) {
				return -EIO;
			}
		} else if (trig->chan == SENSOR_CHAN_GREEN) {
			if (i2c_reg_update_byte(data->i2c,
						DT_ROHM_BH1749_0_BASE_ADDRESS,
						BH1749_INTERRUPT,
						BH1749_INTERRUPT_INT_SOURCE_Msk,
						BH1749_INTERRUPT_INT_SOURCE_GREEN)) {
				return -EIO;
			}
		} else if (trig->chan == SENSOR_CHAN_BLUE) {
			if (i2c_reg_update_byte(data->i2c,
						DT_ROHM_BH1749_0_BASE_ADDRESS,
						BH1749_INTERRUPT,
						BH1749_INTERRUPT_INT_SOURCE_Msk,
						BH1749_INTERRUPT_INT_SOURCE_BLUE)) {
				return -EIO;
			}
		} else {
			return -ENOTSUP;
		}

		if (i2c_reg_update_byte(
			    data->i2c, DT_ROHM_BH1749_0_BASE_ADDRESS,
			    BH1749_PERSISTENCE,
			    BH1749_PERSISTENCE_PERSISTENCE_Msk,
			    BH1749_PERSISTENCE_PERSISTENCE_8_SAMPLES)) {
			return -EIO;
		}
		break;
	case SENSOR_TRIG_DATA_READY:
		if (i2c_reg_update_byte(data->i2c,
					DT_ROHM_BH1749_0_BASE_ADDRESS,
					BH1749_PERSISTENCE,
					BH1749_PERSISTENCE_PERSISTENCE_Msk,
					BH1749_PERSISTENCE_PERSISTENCE_ACTIVE_END)) {
			return -EIO;
		}

		break;
	default:
		LOG_ERR("Unsupported sensor trigger");
		return -ENOTSUP;
	}
	data->trg_handler = handler;
	data->trigger.type = trig->type;
	gpio_pin_enable_callback(data->gpio, DT_ROHM_BH1749_0_INT_GPIOS_PIN);

	return 0;
}

/* Enabling GPIO sense on BH1749 INT pin. */
int bh1749_gpio_interrupt_init(struct device *dev)
{
	int err;
	struct bh1749_data *drv_data = dev->driver_data;

	/* Setup gpio interrupt */
	drv_data->gpio =
		device_get_binding(DT_ROHM_BH1749_0_INT_GPIOS_CONTROLLER);
	if (drv_data->gpio == NULL) {
		LOG_ERR("Failed to get binding to %s device!",
			DT_ROHM_BH1749_0_INT_GPIOS_CONTROLLER);
		return -EINVAL;
	}

	err = gpio_pin_configure(drv_data->gpio, DT_ROHM_BH1749_0_INT_GPIOS_PIN,
				 (GPIO_DIR_IN | GPIO_INT | GPIO_INT_LEVEL |
				  GPIO_INT_ACTIVE_LOW | GPIO_PUD_PULL_UP));
	if (err) {
		LOG_DBG("Failed to configure interrupt GPIO");
		return -EIO;
	}

	gpio_init_callback(&drv_data->gpio_cb, bh1749_gpio_callback,
			   BIT(DT_ROHM_BH1749_0_INT_GPIOS_PIN));

	err = gpio_add_callback(drv_data->gpio, &drv_data->gpio_cb);
	if (err) {
		LOG_DBG("Failed to set GPIO callback");
		return err;
	}

	drv_data->work.handler = bh1749_work_cb;
	drv_data->dev = dev;

	err = i2c_reg_update_byte(
		drv_data->i2c, DT_ROHM_BH1749_0_BASE_ADDRESS, BH1749_INTERRUPT,
		BH1749_INTERRUPT_ENABLE_Msk | BH1749_INTERRUPT_INT_SOURCE_Msk,
		BH1749_INTERRUPT_ENABLE_ENABLE |
		BH1749_INTERRUPT_INT_SOURCE_RED);
	if (err) {
		LOG_ERR("Interrupts could not be enabled.");
		return -EIO;
	}

	k_sem_init(&drv_data->data_sem, 0, UINT_MAX);

	return 0;
}
