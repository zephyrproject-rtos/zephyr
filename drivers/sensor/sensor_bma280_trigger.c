/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <device.h>
#include <gpio.h>
#include <misc/util.h>
#include <nanokernel.h>
#include <sensor.h>

#include "sensor_bma280.h"

extern struct bma280_data bma280_driver;

int bma280_attr_set(struct device *dev,
		    enum sensor_channel chan,
		    enum sensor_attribute attr,
		    const struct sensor_value *val)
{
	struct bma280_data *drv_data = dev->driver_data;
	uint64_t slope_th;
	int rc;

	if (chan != SENSOR_CHAN_ACCEL_ANY) {
		return DEV_INVALID_OP;
	}

	if (attr == SENSOR_ATTR_SLOPE_TH) {
		slope_th = (uint64_t)val->val1 * 1000 / BMA280_SLOPE_TH_SCALE;
		rc = bma280_reg_write(drv_data, BMA280_REG_SLOPE_TH,
				      (uint8_t)slope_th);
		if (rc != DEV_OK) {
			DBG("Could not set slope threshold\n");
			return DEV_FAIL;
		}
	} else if (attr == SENSOR_ATTR_SLOPE_DUR) {
		rc = bma280_reg_update(drv_data, BMA280_REG_INT_5,
				       BMA280_SLOPE_DUR_MASK,
				       val->val1 << BMA280_SLOPE_DUR_SHIFT);
		if (rc != DEV_OK) {
			DBG("Could not set slope duration\n");
			return DEV_FAIL;
		}
	} else {
		return DEV_INVALID_OP;
	}

	return DEV_OK;
}

static void bma280_gpio_callback(struct device *dev, uint32_t pin)
{
	gpio_pin_disable_callback(dev, pin);

#if defined(CONFIG_BMA280_TRIGGER_OWN_FIBER)
	nano_sem_give(&bma280_driver.gpio_sem);
#elif defined(CONFIG_BMA280_TRIGGER_GLOBAL_FIBER)
	nano_isr_fifo_put(sensor_get_work_fifo(), &bma280_driver.work);
#endif
}

static void bma280_fiber_cb(void *arg)
{
	struct device *dev = arg;
	struct bma280_data *drv_data = dev->driver_data;
	uint8_t status = 0;

	/* check for data ready */
	bma280_reg_read(drv_data, BMA280_REG_INT_STATUS_1, &status);
	if (status & BMA280_BIT_DATA_INT_STATUS &&
	    drv_data->data_ready_handler != NULL) {
		drv_data->data_ready_handler(dev, &drv_data->data_ready_trigger);
	}

	/* check for any motion */
	bma280_reg_read(drv_data, BMA280_REG_INT_STATUS_0, &status);
	if (status & BMA280_BIT_SLOPE_INT_STATUS &&
	    drv_data->any_motion_handler != NULL) {
		drv_data->any_motion_handler(dev, &drv_data->data_ready_trigger);

		/* clear latched interrupt */
		bma280_reg_update(drv_data, BMA280_REG_INT_RST_LATCH,
				  BMA280_BIT_INT_LATCH_RESET,
				  BMA280_BIT_INT_LATCH_RESET);
	}

	gpio_pin_enable_callback(drv_data->gpio, CONFIG_BMA280_GPIO_PIN_NUM);
}

#ifdef CONFIG_BMA280_TRIGGER_OWN_FIBER
static void bma280_fiber(int dev_ptr, int unused)
{
	struct device *dev = INT_TO_POINTER(dev_ptr);
	struct bma280_data *drv_data = dev->driver_data;

	ARG_UNUSED(unused);

	while (1) {
		nano_fiber_sem_take(&drv_data->gpio_sem, TICKS_UNLIMITED);
		bma280_fiber_cb(dev);
	}
}
#endif

int bma280_trigger_set(struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler)
{
	struct bma280_data *drv_data = dev->driver_data;
	int rc;

	if (trig->type == SENSOR_TRIG_DATA_READY) {
		/* disable data ready interrupt while changing trigger params */
		rc = bma280_reg_update(drv_data, BMA280_REG_INT_EN_1,
				       BMA280_BIT_DATA_EN, 0);
		if (rc != DEV_OK) {
			DBG("Could not disable data ready interrupt\n");
			return DEV_FAIL;
		}

		drv_data->data_ready_handler = handler;
		if (handler == NULL) {
			return DEV_OK;
		}
		drv_data->data_ready_trigger = *trig;

		/* enable data ready interrupt */
		rc = bma280_reg_update(drv_data, BMA280_REG_INT_EN_1,
				       BMA280_BIT_DATA_EN, BMA280_BIT_DATA_EN);
		if (rc != DEV_OK) {
			DBG("Could not enable data ready interrupt\n");
			return DEV_FAIL;
		}
	} else if (trig->type == SENSOR_TRIG_DELTA) {
		/* disable any-motion interrupt while changing trigger params */
		rc = bma280_reg_update(drv_data, BMA280_REG_INT_EN_0,
				       BMA280_SLOPE_EN_XYZ, 0);
		if (rc != DEV_OK) {
			DBG("Could not disable data ready interrupt\n");
			return DEV_FAIL;
		}

		drv_data->any_motion_handler = handler;
		if (handler == NULL) {
			return DEV_OK;
		}
		drv_data->any_motion_trigger = *trig;

		/* enable any-motion interrupt */
		rc = bma280_reg_update(drv_data, BMA280_REG_INT_EN_0,
				       BMA280_SLOPE_EN_XYZ, BMA280_SLOPE_EN_XYZ);
		if (rc != DEV_OK) {
			DBG("Could not enable data ready interrupt\n");
			return DEV_FAIL;
		}
	} else {
		return DEV_INVALID_OP;
	}

	return DEV_OK;
}

int bma280_init_interrupt(struct device *dev)
{
	struct bma280_data *drv_data = dev->driver_data;
	int rc;

	/* set latched interrupts */
	rc = bma280_reg_write(drv_data, BMA280_REG_INT_RST_LATCH,
			      BMA280_BIT_INT_LATCH_RESET |
			      BMA280_INT_MODE_LATCH);
	if (rc != 0) {
		DBG("Could not set latched interrupts\n");
		return DEV_FAIL;
	}

	/* setup data ready gpio interrupt */
	drv_data->gpio = device_get_binding(CONFIG_BMA280_GPIO_DEV_NAME);
	if (drv_data->gpio == NULL) {
		DBG("Cannot get pointer to %s device\n",
		    CONFIG_BMA280_GPIO_DEV_NAME);
		return DEV_INVALID_CONF;
	}

	gpio_pin_configure(drv_data->gpio, CONFIG_BMA280_GPIO_PIN_NUM,
			   GPIO_DIR_IN | GPIO_INT | GPIO_INT_LEVEL |
			   GPIO_INT_ACTIVE_HIGH | GPIO_INT_DEBOUNCE);

	rc = gpio_set_callback(drv_data->gpio, bma280_gpio_callback);
	if (rc != DEV_OK) {
		DBG("Could not set gpio callback\n");
		return DEV_FAIL;
	}

	/* map data ready interrupt to INT1 */
	rc = bma280_reg_update(drv_data, BMA280_REG_INT_MAP_1,
			       BMA280_INT_MAP_1_BIT_DATA,
			       BMA280_INT_MAP_1_BIT_DATA);
	if (rc != DEV_OK) {
		DBG("Could not map data ready interrupt pin\n");
		return DEV_FAIL;
	}

	/* map any-motion interrupt to INT1 */
	rc = bma280_reg_update(drv_data, BMA280_REG_INT_MAP_0,
			       BMA280_INT_MAP_0_BIT_SLOPE,
			       BMA280_INT_MAP_0_BIT_SLOPE);
	if (rc != DEV_OK) {
		DBG("Could not map any-motion interrupt pin\n");
		return DEV_FAIL;
	}

	/* disable data ready interrupt */
	rc = bma280_reg_update(drv_data, BMA280_REG_INT_EN_1,
			       BMA280_BIT_DATA_EN, 0);
	if (rc != DEV_OK) {
		DBG("Could not disable data ready interrupt\n");
		return DEV_FAIL;
	}

	/* disable any-motion interrupt */
	rc = bma280_reg_update(drv_data, BMA280_REG_INT_EN_0,
			       BMA280_SLOPE_EN_XYZ, 0);
	if (rc != DEV_OK) {
		DBG("Could not disable data ready interrupt\n");
		return DEV_FAIL;
	}

#if defined(CONFIG_BMA280_TRIGGER_OWN_FIBER)
	nano_sem_init(&drv_data->gpio_sem);

	fiber_start(drv_data->fiber_stack, CONFIG_BMA280_FIBER_STACK_SIZE,
		    (nano_fiber_entry_t)bma280_fiber, POINTER_TO_INT(dev),
		    0, CONFIG_BMA280_FIBER_PRIORITY, 0);
#elif defined(CONFIG_BMA280_TRIGGER_GLOBAL_FIBER)
	drv_data->work.handler = bma280_fiber_cb;
	drv_data->work.arg = dev;
#endif

	gpio_pin_enable_callback(drv_data->gpio, CONFIG_BMA280_GPIO_PIN_NUM);

	return DEV_OK;
}
