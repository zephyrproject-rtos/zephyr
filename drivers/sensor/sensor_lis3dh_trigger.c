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

#include "sensor_lis3dh.h"

extern struct lis3dh_data lis3dh_driver;

int lis3dh_trigger_set(struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler)
{
	struct lis3dh_data *drv_data = dev->driver_data;

	if (trig->type != SENSOR_TRIG_DATA_READY) {
		return -ENOTSUP;
	}

	gpio_pin_disable_callback(drv_data->gpio, CONFIG_LIS3DH_GPIO_PIN_NUM);

	drv_data->data_ready_handler = handler;
	if (handler == NULL) {
		return 0;
	}
	drv_data->data_ready_trigger = *trig;

	gpio_pin_enable_callback(drv_data->gpio, CONFIG_LIS3DH_GPIO_PIN_NUM);

	return 0;
}

static void lis3dh_gpio_callback(struct device *dev, uint32_t pin)
{
	gpio_pin_disable_callback(dev, pin);

#if defined(CONFIG_LIS3DH_TRIGGER_OWN_FIBER)
	nano_sem_give(&lis3dh_driver.gpio_sem);
#elif defined(CONFIG_LIS3DH_TRIGGER_GLOBAL_FIBER)
	nano_isr_fifo_put(sensor_get_work_fifo(), &lis3dh_driver.work);
#endif
}

static void lis3dh_fiber_cb(void *arg)
{
	struct device *dev = arg;
	struct lis3dh_data *drv_data = dev->driver_data;

	if (drv_data->data_ready_handler != NULL) {
		drv_data->data_ready_handler(dev,
					     &drv_data->data_ready_trigger);
	}

	gpio_pin_enable_callback(drv_data->gpio, CONFIG_LIS3DH_GPIO_PIN_NUM);
}

#ifdef CONFIG_LIS3DH_TRIGGER_OWN_FIBER
static void lis3dh_fiber(int dev_ptr, int unused)
{
	struct device *dev = INT_TO_POINTER(dev_ptr);
	struct lis3dh_data *drv_data = dev->driver_data;

	ARG_UNUSED(unused);

	while (1) {
		nano_fiber_sem_take(&drv_data->gpio_sem, TICKS_UNLIMITED);
		lis3dh_fiber_cb(dev);
	}
}
#endif

int lis3dh_init_interrupt(struct device *dev)
{
	struct lis3dh_data *drv_data = dev->driver_data;
	int rc;

	/* setup data ready gpio interrupt */
	drv_data->gpio = device_get_binding(CONFIG_LIS3DH_GPIO_DEV_NAME);
	if (drv_data->gpio == NULL) {
		DBG("Cannot get pointer to %s device\n",
		    CONFIG_LIS3DH_GPIO_DEV_NAME);
		return -EINVAL;
	}

	gpio_pin_configure(drv_data->gpio, CONFIG_LIS3DH_GPIO_PIN_NUM,
			   GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			   GPIO_INT_ACTIVE_HIGH | GPIO_INT_DEBOUNCE);

	rc = gpio_set_callback(drv_data->gpio, lis3dh_gpio_callback);
	if (rc != 0) {
		DBG("Could not set gpio callback\n");
		return -EIO;
	}

	/* clear data ready interrupt line by reading sample data */
	rc = lis3dh_sample_fetch(dev);
	if (rc != 0) {
		DBG("Could not clear data ready interrupt line.\n");
		return -EIO;
	}

	/* enable data ready interrupt on INT1 line */
	rc = lis3dh_reg_write(drv_data, LIS3DH_REG_CTRL3,
			      LIS3DH_EN_DRDY1_INT1);
	if (rc != 0) {
		DBG("Failed to enable data ready interrupt.\n");
		return -EIO;
	}

#if defined(CONFIG_LIS3DH_TRIGGER_OWN_FIBER)
	nano_sem_init(&drv_data->gpio_sem);

	fiber_start(drv_data->fiber_stack, CONFIG_LIS3DH_FIBER_STACK_SIZE,
		    (nano_fiber_entry_t)lis3dh_fiber, POINTER_TO_INT(dev),
		    0, CONFIG_LIS3DH_FIBER_PRIORITY, 0);
#elif defined(CONFIG_LIS3DH_TRIGGER_GLOBAL_FIBER)
	drv_data->work.handler = lis3dh_fiber_cb;
	drv_data->work.arg = dev;
#endif

	gpio_pin_enable_callback(drv_data->gpio, CONFIG_LIS3DH_GPIO_PIN_NUM);

	return 0;
}
