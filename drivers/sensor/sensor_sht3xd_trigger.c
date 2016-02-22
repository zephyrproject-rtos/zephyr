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

#include "sensor_sht3xd.h"

extern struct sht3xd_data sht3xd_driver;

static uint16_t sht3xd_temp_processed_to_raw(const struct sensor_value *val)
{
	uint32_t val2;
	uint64_t uval;

	if (val->type == SENSOR_TYPE_INT) {
		val2 = 0;
	} else {
		val2 = val->val2;
	}

	/* ret = (val + 45) * (2^16 - 1) / 175 */
	uval = (uint64_t)(val->val1 + 45) * 1000000 + val2;
	return ((uval * 0xFFFF) / 175) / 1000000;
}

static int sht3xd_rh_processed_to_raw(const struct sensor_value *val)
{
	uint32_t val2;
	uint64_t uval;

	if (val->type == SENSOR_TYPE_INT) {
		val2 = 0;
	} else {
		val2 = val->val2;
	}

	/* ret = val * (2^16 -1) / 100000 */
	uval = (uint64_t)val->val1 * 1000000 + val2;
	return ((uval * 0xFFFF) / 100000) / 1000000;
}

int sht3xd_attr_set(struct device *dev,
		    enum sensor_channel chan,
		    enum sensor_attribute attr,
		    const struct sensor_value *val)
{
	struct sht3xd_data *drv_data = dev->driver_data;
	uint16_t set_cmd, clear_cmd, reg_val, temp, rh;

	if (val->type != SENSOR_TYPE_INT &&
	    val->type != SENSOR_TYPE_INT_PLUS_MICRO) {
		return DEV_INVALID_OP;
	}

	if (attr == SENSOR_ATTR_LOWER_THRESH) {
		if (chan == SENSOR_CHAN_TEMP) {
			drv_data->t_low = sht3xd_temp_processed_to_raw(val);
		} else if (chan == SENSOR_CHAN_HUMIDITY) {
			drv_data->rh_low = sht3xd_rh_processed_to_raw(val);
		} else {
			return DEV_INVALID_OP;
		}

		set_cmd = SHT3XD_CMD_WRITE_TH_LOW_SET;
		clear_cmd = SHT3XD_CMD_WRITE_TH_LOW_CLEAR;
		temp = drv_data->t_low;
		rh = drv_data->rh_low;
	} else if (attr == SENSOR_ATTR_UPPER_THRESH) {
		if (chan == SENSOR_CHAN_TEMP) {
			drv_data->t_high = sht3xd_temp_processed_to_raw(val);
		} else if (chan == SENSOR_CHAN_HUMIDITY) {
			drv_data->rh_high = sht3xd_rh_processed_to_raw(val);
		} else {
			return DEV_INVALID_OP;
		}

		set_cmd = SHT3XD_CMD_WRITE_TH_HIGH_SET;
		clear_cmd = SHT3XD_CMD_WRITE_TH_HIGH_CLEAR;
		temp = drv_data->t_high;
		rh = drv_data->rh_high;
	} else {
		return DEV_INVALID_OP;
	}

	reg_val = (rh & 0xFE00) | ((temp & 0xFF80) >> 7);

	if (sht3xd_write_reg(drv_data, set_cmd, reg_val) != DEV_OK ||
	    sht3xd_write_reg(drv_data, clear_cmd, reg_val) != DEV_OK) {
		DBG("Failed to write threshold value!\n");
		return DEV_FAIL;
	}

	return DEV_OK;
}

static void sht3xd_gpio_callback(struct device *dev, uint32_t pin)
{
	gpio_pin_disable_callback(dev, pin);

#if defined(CONFIG_SHT3XD_TRIGGER_OWN_FIBER)
	nano_sem_give(&sht3xd_driver.gpio_sem);
#elif defined(CONFIG_SHT3XD_TRIGGER_GLOBAL_FIBER)
	nano_isr_fifo_put(sensor_get_work_fifo(), &sht3xd_driver.work);
#endif
}

static void sht3xd_fiber_cb(void *arg)
{
	struct device *dev = arg;
	struct sht3xd_data *drv_data = dev->driver_data;

	if (drv_data->handler != NULL) {
		drv_data->handler(dev, &drv_data->trigger);
	}

	gpio_pin_enable_callback(drv_data->gpio, CONFIG_SHT3XD_GPIO_PIN_NUM);
}

#ifdef CONFIG_SHT3XD_TRIGGER_OWN_FIBER
static void sht3xd_fiber(int dev_ptr, int unused)
{
	struct device *dev = INT_TO_POINTER(dev_ptr);
	struct sht3xd_data *drv_data = dev->driver_data;

	ARG_UNUSED(unused);

	while (1) {
		nano_fiber_sem_take(&drv_data->gpio_sem, TICKS_UNLIMITED);
		sht3xd_fiber_cb(dev);
	}
}
#endif

int sht3xd_trigger_set(struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler)
{
	struct sht3xd_data *drv_data = dev->driver_data;

	if (trig->type != SENSOR_TRIG_THRESHOLD) {
		return DEV_INVALID_OP;
	}

	gpio_pin_disable_callback(drv_data->gpio, CONFIG_SHT3XD_GPIO_PIN_NUM);
	drv_data->handler = handler;
	drv_data->trigger = *trig;
	gpio_pin_enable_callback(drv_data->gpio, CONFIG_SHT3XD_GPIO_PIN_NUM);

	return DEV_OK;
}

int sht3xd_init_interrupt(struct device *dev)
{
	struct sht3xd_data *drv_data = dev->driver_data;
	int rc;

	drv_data->t_low = 0;
	drv_data->rh_low = 0;
	drv_data->t_high = 0xFFFF;
	drv_data->rh_high = 0xFFFF;

	/* set alert thresholds to match reamsurement ranges */
	rc = sht3xd_write_reg(drv_data, SHT3XD_CMD_WRITE_TH_HIGH_SET, 0xFFFF);
	if (rc != DEV_OK) {
		DBG("Failed to write threshold high set value!\n");
		return DEV_FAIL;
	}

	rc = sht3xd_write_reg(drv_data, SHT3XD_CMD_WRITE_TH_HIGH_CLEAR,
			      0xFFFF);
	if (rc != DEV_OK) {
		DBG("Failed to write threshold high clear value!\n");
		return DEV_FAIL;
	}

	rc = sht3xd_write_reg(drv_data, SHT3XD_CMD_WRITE_TH_LOW_SET, 0);
	if (rc != DEV_OK) {
		DBG("Failed to write threshold low set value!\n");
		return DEV_FAIL;
	}

	rc = sht3xd_write_reg(drv_data, SHT3XD_CMD_WRITE_TH_LOW_SET, 0);
	if (rc != DEV_OK) {
		DBG("Failed to write threshold low clear value!\n");
		return DEV_FAIL;
	}

	/* setup gpio interrupt */
	drv_data->gpio = device_get_binding(CONFIG_SHT3XD_GPIO_DEV_NAME);
	if (drv_data->gpio == NULL) {
		DBG("Failed to get pointer to %s device!\n",
		    CONFIG_SHT3XD_GPIO_DEV_NAME);
		return DEV_INVALID_CONF;
	}

	gpio_pin_configure(drv_data->gpio, CONFIG_SHT3XD_GPIO_PIN_NUM,
			   GPIO_DIR_IN | GPIO_INT | GPIO_INT_LEVEL |
			   GPIO_INT_ACTIVE_HIGH | GPIO_INT_DEBOUNCE);

	rc = gpio_set_callback(drv_data->gpio, sht3xd_gpio_callback);
	if (rc != DEV_OK) {
		DBG("Failed to set gpio callback!\n");
		return DEV_FAIL;
	}

#if defined(CONFIG_SHT3XD_TRIGGER_OWN_FIBER)
	nano_sem_init(&drv_data->gpio_sem);

	fiber_start(drv_data->fiber_stack, CONFIG_SHT3XD_FIBER_STACK_SIZE,
		    (nano_fiber_entry_t)sht3xd_fiber, POINTER_TO_INT(dev),
		    0, CONFIG_SHT3XD_FIBER_PRIORITY, 0);
#elif defined(CONFIG_SHT3XD_TRIGGER_GLOBAL_FIBER)
	drv_data->work.handler = sht3xd_fiber_cb;
	drv_data->work.arg = dev;
#endif

	return DEV_OK;
}
