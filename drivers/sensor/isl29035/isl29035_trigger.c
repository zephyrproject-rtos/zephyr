/* sensor_isl29035.c - trigger support for ISL29035 light sensor */

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

#include <i2c.h>
#include <misc/util.h>
#include <kernel.h>

#include "isl29035.h"

extern struct isl29035_driver_data isl29035_data;

static uint16_t isl29035_lux_processed_to_raw(struct sensor_value const *val)
{
	uint64_t raw_val, ival, uval;

	ival = val->val1;
	if (val->type == SENSOR_VALUE_TYPE_INT) {
		uval = 0;
	} else {
		uval = val->val2;
	}

	/* raw_val = val * (2 ^ adc_data_bits) / lux_range */
	raw_val = (ival << ISL29035_ADC_DATA_BITS) +
		  (uval << ISL29035_ADC_DATA_BITS) / 1000000;

	return raw_val / ISL29035_LUX_RANGE;
}

int isl29035_attr_set(struct device *dev,
		      enum sensor_channel chan,
		      enum sensor_attribute attr,
		      const struct sensor_value *val)
{
	struct isl29035_driver_data *drv_data = dev->driver_data;
	uint8_t lsb_reg, msb_reg;
	uint16_t raw_val;

	if (attr == SENSOR_ATTR_UPPER_THRESH) {
		lsb_reg = ISL29035_INT_HT_LSB_REG;
		msb_reg = ISL29035_INT_HT_MSB_REG;
	} else if (attr == SENSOR_ATTR_LOWER_THRESH) {
		lsb_reg = ISL29035_INT_LT_LSB_REG;
		msb_reg = ISL29035_INT_LT_MSB_REG;
	} else {
		return -ENOTSUP;
	}

	raw_val = isl29035_lux_processed_to_raw(val);

	if (i2c_reg_write_byte(drv_data->i2c, ISL29035_I2C_ADDRESS,
			       lsb_reg, raw_val & 0xFF) < 0 ||
	    i2c_reg_write_byte(drv_data->i2c, ISL29035_I2C_ADDRESS,
			       msb_reg, raw_val >> 8) < 0) {
		SYS_LOG_DBG("Failed to set attribute.");
		return -EIO;
	}

	return 0;
}

static void isl29035_gpio_callback(struct device *dev,
				   struct gpio_callback *cb, uint32_t pins)
{
	struct isl29035_driver_data *drv_data =
		CONTAINER_OF(cb, struct isl29035_driver_data, gpio_cb);

	ARG_UNUSED(pins);

	gpio_pin_disable_callback(dev, CONFIG_ISL29035_GPIO_PIN_NUM);

#if defined(CONFIG_ISL29035_TRIGGER_OWN_THREAD)
	k_sem_give(&drv_data->gpio_sem);
#elif defined(CONFIG_ISL29035_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&drv_data->work);
#endif
}

static void isl29035_thread_cb(struct device *dev)
{
	struct isl29035_driver_data *drv_data = dev->driver_data;
	uint8_t val;

	/* clear interrupt */
	i2c_reg_read_byte(drv_data->i2c, ISL29035_I2C_ADDRESS,
			  ISL29035_COMMAND_I_REG, &val);

	if (drv_data->th_handler != NULL) {
		drv_data->th_handler(dev, &drv_data->th_trigger);
	}

	gpio_pin_enable_callback(drv_data->gpio, CONFIG_ISL29035_GPIO_PIN_NUM);
}

#ifdef CONFIG_ISL29035_TRIGGER_OWN_THREAD
static void isl29035_thread(int ptr, int unused)
{
	struct device *dev = INT_TO_POINTER(ptr);
	struct isl29035_driver_data *drv_data = dev->driver_data;

	ARG_UNUSED(unused);

	while (1) {
		k_sem_take(&drv_data->gpio_sem, K_FOREVER);
		isl29035_thread_cb(dev);
	}
}
#endif

#ifdef CONFIG_ISL29035_TRIGGER_GLOBAL_THREAD
static void isl29035_work_cb(struct k_work *work)
{
	struct isl29035_driver_data *drv_data =
		CONTAINER_OF(work, struct isl29035_driver_data, work);

	isl29035_thread_cb(drv_data->dev);
}
#endif

int isl29035_trigger_set(struct device *dev,
			 const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler)
{
	struct isl29035_driver_data *drv_data = dev->driver_data;

	/* disable interrupt callback while changing parameters */
	gpio_pin_disable_callback(drv_data->gpio, CONFIG_ISL29035_GPIO_PIN_NUM);

	drv_data->th_handler = handler;
	drv_data->th_trigger = *trig;

	/* enable interrupt callback */
	gpio_pin_enable_callback(drv_data->gpio, CONFIG_ISL29035_GPIO_PIN_NUM);

	return 0;
}

int isl29035_init_interrupt(struct device *dev)
{
	struct isl29035_driver_data *drv_data = dev->driver_data;

	/* set interrupt persistence */
	if (i2c_reg_update_byte(drv_data->i2c, ISL29035_I2C_ADDRESS,
				ISL29035_COMMAND_I_REG,
				ISL29035_INT_PRST_MASK,
				ISL29035_INT_PRST_BITS) < 0) {
		SYS_LOG_DBG("Failed to set interrupt persistence cycles.");
		return -EIO;
	}

	/* setup gpio interrupt */
	drv_data->gpio = device_get_binding(CONFIG_ISL29035_GPIO_DEV_NAME);
	if (drv_data->gpio == NULL) {
		SYS_LOG_DBG("Failed to get GPIO device.");
		return -EINVAL;
	}

	gpio_pin_configure(drv_data->gpio, CONFIG_ISL29035_GPIO_PIN_NUM,
			   GPIO_DIR_IN | GPIO_INT | GPIO_INT_LEVEL |
			   GPIO_INT_ACTIVE_LOW | GPIO_INT_DEBOUNCE);

	gpio_init_callback(&drv_data->gpio_cb,
			   isl29035_gpio_callback,
			   BIT(CONFIG_ISL29035_GPIO_PIN_NUM));

	if (gpio_add_callback(drv_data->gpio, &drv_data->gpio_cb) < 0) {
		SYS_LOG_DBG("Failed to set gpio callback.");
		return -EIO;
	}

#if defined(CONFIG_ISL29035_TRIGGER_OWN_THREAD)
	k_sem_init(&drv_data->gpio_sem, 0, UINT_MAX);

	k_thread_spawn(drv_data->thread_stack, CONFIG_ISL29035_THREAD_STACK_SIZE,
		    (k_thread_entry_t)isl29035_thread, POINTER_TO_INT(dev),
		    0, NULL, K_PRIO_COOP(CONFIG_ISL29035_THREAD_PRIORITY), 0, 0);
#elif defined(CONFIG_ISL29035_TRIGGER_GLOBAL_THREAD)
	drv_data->work.handler = isl29035_work_cb;
	drv_data->dev = dev;
#endif

	return 0;
}
