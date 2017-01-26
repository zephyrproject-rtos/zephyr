/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <i2c.h>
#include <misc/util.h>
#include <kernel.h>
#include <sensor.h>

#include "bma280.h"

int bma280_attr_set(struct device *dev,
		    enum sensor_channel chan,
		    enum sensor_attribute attr,
		    const struct sensor_value *val)
{
	struct bma280_data *drv_data = dev->driver_data;
	uint64_t slope_th;

	if (chan != SENSOR_CHAN_ACCEL_XYZ) {
		return -ENOTSUP;
	}

	if (attr == SENSOR_ATTR_SLOPE_TH) {
		/* slope_th = (val * 10^6 * 2^10) / BMA280_PMU_FULL_RAGE */
		slope_th = (uint64_t)val->val1 * 1000000 + (uint64_t)val->val2;
		slope_th = (slope_th * (1 << 10)) / BMA280_PMU_FULL_RANGE;
		if (i2c_reg_write_byte(drv_data->i2c, BMA280_I2C_ADDRESS,
				       BMA280_REG_SLOPE_TH, (uint8_t)slope_th)
				       < 0) {
			SYS_LOG_DBG("Could not set slope threshold");
			return -EIO;
		}
	} else if (attr == SENSOR_ATTR_SLOPE_DUR) {
		if (i2c_reg_update_byte(drv_data->i2c, BMA280_I2C_ADDRESS,
					BMA280_REG_INT_5,
					BMA280_SLOPE_DUR_MASK,
					val->val1 << BMA280_SLOPE_DUR_SHIFT)
					< 0) {
			SYS_LOG_DBG("Could not set slope duration");
			return -EIO;
		}
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static void bma280_gpio_callback(struct device *dev,
				 struct gpio_callback *cb, uint32_t pins)
{
	struct bma280_data *drv_data =
		CONTAINER_OF(cb, struct bma280_data, gpio_cb);

	ARG_UNUSED(pins);

	gpio_pin_disable_callback(dev, CONFIG_BMA280_GPIO_PIN_NUM);

#if defined(CONFIG_BMA280_TRIGGER_OWN_THREAD)
	k_sem_give(&drv_data->gpio_sem);
#elif defined(CONFIG_BMA280_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&drv_data->work);
#endif
}

static void bma280_thread_cb(void *arg)
{
	struct device *dev = arg;
	struct bma280_data *drv_data = dev->driver_data;
	uint8_t status = 0;
	int err = 0;

	/* check for data ready */
	err = i2c_reg_read_byte(drv_data->i2c, BMA280_I2C_ADDRESS,
				BMA280_REG_INT_STATUS_1, &status);
	if (status & BMA280_BIT_DATA_INT_STATUS &&
	    drv_data->data_ready_handler != NULL &&
	    err == 0) {
		drv_data->data_ready_handler(dev,
					     &drv_data->data_ready_trigger);
	}

	/* check for any motion */
	err = i2c_reg_read_byte(drv_data->i2c, BMA280_I2C_ADDRESS,
				BMA280_REG_INT_STATUS_0, &status);
	if (status & BMA280_BIT_SLOPE_INT_STATUS &&
	    drv_data->any_motion_handler != NULL &&
	    err == 0) {
		drv_data->any_motion_handler(dev,
					     &drv_data->data_ready_trigger);

		/* clear latched interrupt */
		err = i2c_reg_update_byte(drv_data->i2c, BMA280_I2C_ADDRESS,
					  BMA280_REG_INT_RST_LATCH,
					  BMA280_BIT_INT_LATCH_RESET,
					  BMA280_BIT_INT_LATCH_RESET);

		if (err < 0) {
			SYS_LOG_DBG("Could not update clear the interrupt");
			return;
		}
	}

	gpio_pin_enable_callback(drv_data->gpio, CONFIG_BMA280_GPIO_PIN_NUM);
}

#ifdef CONFIG_BMA280_TRIGGER_OWN_THREAD
static void bma280_thread(int dev_ptr, int unused)
{
	struct device *dev = INT_TO_POINTER(dev_ptr);
	struct bma280_data *drv_data = dev->driver_data;

	ARG_UNUSED(unused);

	while (1) {
		k_sem_take(&drv_data->gpio_sem, K_FOREVER);
		bma280_thread_cb(dev);
	}
}
#endif

#ifdef CONFIG_BMA280_TRIGGER_GLOBAL_THREAD
static void bma280_work_cb(struct k_work *work)
{
	struct bma280_data *drv_data =
		CONTAINER_OF(work, struct bma280_data, work);

	bma280_thread_cb(drv_data->dev);
}
#endif

int bma280_trigger_set(struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler)
{
	struct bma280_data *drv_data = dev->driver_data;

	if (trig->type == SENSOR_TRIG_DATA_READY) {
		/* disable data ready interrupt while changing trigger params */
		if (i2c_reg_update_byte(drv_data->i2c, BMA280_I2C_ADDRESS,
					BMA280_REG_INT_EN_1,
					BMA280_BIT_DATA_EN, 0) < 0) {
			SYS_LOG_DBG("Could not disable data ready interrupt");
			return -EIO;
		}

		drv_data->data_ready_handler = handler;
		if (handler == NULL) {
			return 0;
		}
		drv_data->data_ready_trigger = *trig;

		/* enable data ready interrupt */
		if (i2c_reg_update_byte(drv_data->i2c, BMA280_I2C_ADDRESS,
					BMA280_REG_INT_EN_1,
					BMA280_BIT_DATA_EN,
					BMA280_BIT_DATA_EN) < 0) {
			SYS_LOG_DBG("Could not enable data ready interrupt");
			return -EIO;
		}
	} else if (trig->type == SENSOR_TRIG_DELTA) {
		/* disable any-motion interrupt while changing trigger params */
		if (i2c_reg_update_byte(drv_data->i2c, BMA280_I2C_ADDRESS,
					BMA280_REG_INT_EN_0,
					BMA280_SLOPE_EN_XYZ, 0) < 0) {
			SYS_LOG_DBG("Could not disable data ready interrupt");
			return -EIO;
		}

		drv_data->any_motion_handler = handler;
		if (handler == NULL) {
			return 0;
		}
		drv_data->any_motion_trigger = *trig;

		/* enable any-motion interrupt */
		if (i2c_reg_update_byte(drv_data->i2c, BMA280_I2C_ADDRESS,
					BMA280_REG_INT_EN_0,
					BMA280_SLOPE_EN_XYZ,
					BMA280_SLOPE_EN_XYZ) < 0) {
			SYS_LOG_DBG("Could not enable data ready interrupt");
			return -EIO;
		}
	} else {
		return -ENOTSUP;
	}

	return 0;
}

int bma280_init_interrupt(struct device *dev)
{
	struct bma280_data *drv_data = dev->driver_data;

	/* set latched interrupts */
	if (i2c_reg_write_byte(drv_data->i2c, BMA280_I2C_ADDRESS,
			       BMA280_REG_INT_RST_LATCH,
			       BMA280_BIT_INT_LATCH_RESET |
			       BMA280_INT_MODE_LATCH) < 0) {
		SYS_LOG_DBG("Could not set latched interrupts");
		return -EIO;
	}

	/* setup data ready gpio interrupt */
	drv_data->gpio = device_get_binding(CONFIG_BMA280_GPIO_DEV_NAME);
	if (drv_data->gpio == NULL) {
		SYS_LOG_DBG("Cannot get pointer to %s device",
		    CONFIG_BMA280_GPIO_DEV_NAME);
		return -EINVAL;
	}

	gpio_pin_configure(drv_data->gpio, CONFIG_BMA280_GPIO_PIN_NUM,
			   GPIO_DIR_IN | GPIO_INT | GPIO_INT_LEVEL |
			   GPIO_INT_ACTIVE_HIGH | GPIO_INT_DEBOUNCE);

	gpio_init_callback(&drv_data->gpio_cb,
			   bma280_gpio_callback,
			   BIT(CONFIG_BMA280_GPIO_PIN_NUM));

	if (gpio_add_callback(drv_data->gpio, &drv_data->gpio_cb) < 0) {
		SYS_LOG_DBG("Could not set gpio callback");
		return -EIO;
	}

	/* map data ready interrupt to INT1 */
	if (i2c_reg_update_byte(drv_data->i2c, BMA280_I2C_ADDRESS,
				BMA280_REG_INT_MAP_1,
				BMA280_INT_MAP_1_BIT_DATA,
				BMA280_INT_MAP_1_BIT_DATA) < 0) {
		SYS_LOG_DBG("Could not map data ready interrupt pin");
		return -EIO;
	}

	/* map any-motion interrupt to INT1 */
	if (i2c_reg_update_byte(drv_data->i2c, BMA280_I2C_ADDRESS,
				BMA280_REG_INT_MAP_0,
				BMA280_INT_MAP_0_BIT_SLOPE,
				BMA280_INT_MAP_0_BIT_SLOPE) < 0) {
		SYS_LOG_DBG("Could not map any-motion interrupt pin");
		return -EIO;
	}

	if (i2c_reg_update_byte(drv_data->i2c, BMA280_I2C_ADDRESS,
				BMA280_REG_INT_EN_1,
				BMA280_BIT_DATA_EN, 0) < 0) {
		SYS_LOG_DBG("Could not disable data ready interrupt");
		return -EIO;
	}

	/* disable any-motion interrupt */
	if (i2c_reg_update_byte(drv_data->i2c, BMA280_I2C_ADDRESS,
				BMA280_REG_INT_EN_0,
				BMA280_SLOPE_EN_XYZ, 0) < 0) {
		SYS_LOG_DBG("Could not disable data ready interrupt");
		return -EIO;
	}

#if defined(CONFIG_BMA280_TRIGGER_OWN_THREAD)
	k_sem_init(&drv_data->gpio_sem, 0, UINT_MAX);

	k_thread_spawn(drv_data->thread_stack, CONFIG_BMA280_THREAD_STACK_SIZE,
		    (k_thread_entry_t)bma280_thread, POINTER_TO_INT(dev), 0, NULL,
		    K_PRIO_COOP(CONFIG_BMA280_THREAD_PRIORITY), 0, 0);
#elif defined(CONFIG_BMA280_TRIGGER_GLOBAL_THREAD)
	drv_data->work.handler = bma280_work_cb;
	drv_data->dev = dev;
#endif

	gpio_pin_enable_callback(drv_data->gpio, CONFIG_BMA280_GPIO_PIN_NUM);

	return 0;
}
