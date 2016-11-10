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
#include <i2c.h>
#include <misc/__assert.h>
#include <misc/util.h>
#include <kernel.h>
#include <sensor.h>

#include "hmc5883l.h"

int hmc5883l_trigger_set(struct device *dev,
			 const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler)
{
	struct hmc5883l_data *drv_data = dev->driver_data;

	__ASSERT_NO_MSG(trig->type == SENSOR_TRIG_DATA_READY);

	gpio_pin_disable_callback(drv_data->gpio, CONFIG_HMC5883L_GPIO_PIN_NUM);

	drv_data->data_ready_handler = handler;
	if (handler == NULL) {
		return 0;
	}

	drv_data->data_ready_trigger = *trig;

	gpio_pin_enable_callback(drv_data->gpio, CONFIG_HMC5883L_GPIO_PIN_NUM);

	return 0;
}

static void hmc5883l_gpio_callback(struct device *dev,
				   struct gpio_callback *cb, uint32_t pins)
{
	struct hmc5883l_data *drv_data =
		CONTAINER_OF(cb, struct hmc5883l_data, gpio_cb);

	ARG_UNUSED(pins);

	gpio_pin_disable_callback(dev, CONFIG_HMC5883L_GPIO_PIN_NUM);

#if defined(CONFIG_HMC5883L_TRIGGER_OWN_THREAD)
	k_sem_give(&drv_data->gpio_sem);
#elif defined(CONFIG_HMC5883L_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&drv_data->work);
#endif
}

static void hmc5883l_thread_cb(void *arg)
{
	struct device *dev = arg;
	struct hmc5883l_data *drv_data = dev->driver_data;

	if (drv_data->data_ready_handler != NULL) {
		drv_data->data_ready_handler(dev,
					     &drv_data->data_ready_trigger);
	}

	gpio_pin_enable_callback(drv_data->gpio, CONFIG_HMC5883L_GPIO_PIN_NUM);
}

#ifdef CONFIG_HMC5883L_TRIGGER_OWN_THREAD
static void hmc5883l_thread(int dev_ptr, int unused)
{
	struct device *dev = INT_TO_POINTER(dev_ptr);
	struct hmc5883l_data *drv_data = dev->driver_data;

	ARG_UNUSED(unused);

	while (1) {
		k_sem_take(&drv_data->gpio_sem, K_FOREVER);
		hmc5883l_thread_cb(dev);
	}
}
#endif

#ifdef CONFIG_HMC5883L_TRIGGER_GLOBAL_THREAD
static void hmc5883l_work_cb(struct k_work *work)
{
	struct hmc5883l_data *drv_data =
		CONTAINER_OF(work, struct hmc5883l_data, work);

	hmc5883l_thread_cb(drv_data->dev);
}
#endif

int hmc5883l_init_interrupt(struct device *dev)
{
	struct hmc5883l_data *drv_data = dev->driver_data;

	/* setup data ready gpio interrupt */
	drv_data->gpio = device_get_binding(CONFIG_HMC5883L_GPIO_DEV_NAME);
	if (drv_data->gpio == NULL) {
		SYS_LOG_ERR("Failed to get pointer to %s device.",
			    CONFIG_HMC5883L_GPIO_DEV_NAME);
		return -EINVAL;
	}

	gpio_pin_configure(drv_data->gpio, CONFIG_HMC5883L_GPIO_PIN_NUM,
			   GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			   GPIO_INT_ACTIVE_LOW | GPIO_INT_DEBOUNCE);

	gpio_init_callback(&drv_data->gpio_cb,
			   hmc5883l_gpio_callback,
			   BIT(CONFIG_HMC5883L_GPIO_PIN_NUM));

	if (gpio_add_callback(drv_data->gpio, &drv_data->gpio_cb) < 0) {
		SYS_LOG_ERR("Failed to set gpio callback.");
		return -EIO;
	}

#if defined(CONFIG_HMC5883L_TRIGGER_OWN_THREAD)
	k_sem_init(&drv_data->gpio_sem, 0, UINT_MAX);

	k_thread_spawn(drv_data->thread_stack, CONFIG_HMC5883L_THREAD_STACK_SIZE,
		    (k_thread_entry_t)hmc5883l_thread, POINTER_TO_INT(dev),
		    0, NULL, K_PRIO_COOP(CONFIG_HMC5883L_THREAD_PRIORITY), 0, 0);
#elif defined(CONFIG_HMC5883L_TRIGGER_GLOBAL_THREAD)
	drv_data->work.handler = hmc5883l_work_cb;
	drv_data->dev = dev;
#endif

	gpio_pin_enable_callback(drv_data->gpio, CONFIG_HMC5883L_GPIO_PIN_NUM);

	return 0;
}
