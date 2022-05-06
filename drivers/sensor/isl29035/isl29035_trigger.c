/* sensor_isl29035.c - trigger support for ISL29035 light sensor */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT isil_isl29035

#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "isl29035.h"

extern struct isl29035_driver_data isl29035_data;

LOG_MODULE_DECLARE(ISL29035, CONFIG_SENSOR_LOG_LEVEL);

static inline void setup_int(struct isl29035_driver_data *drv_data,
			     bool enable)
{
	unsigned int flags = enable
		? GPIO_INT_EDGE_TO_ACTIVE
		: GPIO_INT_DISABLE;

	gpio_pin_interrupt_configure(drv_data->gpio,
				     DT_INST_GPIO_PIN(0, int_gpios),
				     flags);
}

static inline void handle_int(struct isl29035_driver_data *drv_data)
{
	setup_int(drv_data, false);

#if defined(CONFIG_ISL29035_TRIGGER_OWN_THREAD)
	k_sem_give(&drv_data->gpio_sem);
#elif defined(CONFIG_ISL29035_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&drv_data->work);
#endif
}

static uint16_t isl29035_lux_processed_to_raw(struct sensor_value const *val)
{
	uint64_t raw_val;

	/* raw_val = val * (2 ^ adc_data_bits) / lux_range */
	raw_val = (((uint64_t)val->val1) << ISL29035_ADC_DATA_BITS) +
		  (((uint64_t)val->val2) << ISL29035_ADC_DATA_BITS) / 1000000U;

	return raw_val / ISL29035_LUX_RANGE;
}

int isl29035_attr_set(const struct device *dev,
		      enum sensor_channel chan,
		      enum sensor_attribute attr,
		      const struct sensor_value *val)
{
	struct isl29035_driver_data *drv_data = dev->data;
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
		LOG_DBG("Failed to set attribute.");
		return -EIO;
	}

	return 0;
}

static void isl29035_gpio_callback(const struct device *dev,
				   struct gpio_callback *cb, uint32_t pins)
{
	struct isl29035_driver_data *drv_data =
		CONTAINER_OF(cb, struct isl29035_driver_data, gpio_cb);


	ARG_UNUSED(pins);
	handle_int(drv_data);
}

static void isl29035_thread_cb(const struct device *dev)
{
	struct isl29035_driver_data *drv_data = dev->data;
	uint8_t val;

	/* clear interrupt */
	if (i2c_reg_read_byte(drv_data->i2c, ISL29035_I2C_ADDRESS,
			      ISL29035_COMMAND_I_REG, &val) < 0) {
		LOG_ERR("isl29035: Error reading command register");
		return;
	}

	if (drv_data->th_handler != NULL) {
		drv_data->th_handler(dev, &drv_data->th_trigger);
	}

	setup_int(drv_data, true);
}

#ifdef CONFIG_ISL29035_TRIGGER_OWN_THREAD
static void isl29035_thread(struct isl29035_driver_data *drv_data)
{
	while (1) {
		k_sem_take(&drv_data->gpio_sem, K_FOREVER);
		isl29035_thread_cb(drv_data->dev);
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

int isl29035_trigger_set(const struct device *dev,
			 const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler)
{
	struct isl29035_driver_data *drv_data = dev->data;

	/* disable interrupt callback while changing parameters */
	setup_int(drv_data, false);

	drv_data->th_handler = handler;
	drv_data->th_trigger = *trig;

	/* enable interrupt callback */
	setup_int(drv_data, true);
	if (gpio_pin_get(drv_data->gpio,
			 DT_INST_GPIO_PIN(0, int_gpios)) > 0) {
		handle_int(drv_data);
	}

	return 0;
}

int isl29035_init_interrupt(const struct device *dev)
{
	struct isl29035_driver_data *drv_data = dev->data;

	/* set interrupt persistence */
	if (i2c_reg_update_byte(drv_data->i2c, ISL29035_I2C_ADDRESS,
				ISL29035_COMMAND_I_REG,
				ISL29035_INT_PRST_MASK,
				ISL29035_INT_PRST_BITS) < 0) {
		LOG_DBG("Failed to set interrupt persistence cycles.");
		return -EIO;
	}

	/* setup gpio interrupt */
	drv_data->gpio =
		device_get_binding(DT_INST_GPIO_LABEL(0, int_gpios));
	if (drv_data->gpio == NULL) {
		LOG_DBG("Failed to get GPIO device.");
		return -EINVAL;
	}

	gpio_pin_configure(drv_data->gpio, DT_INST_GPIO_PIN(0, int_gpios),
			   GPIO_INPUT | DT_INST_GPIO_FLAGS(0, int_gpios));

	gpio_init_callback(&drv_data->gpio_cb,
			   isl29035_gpio_callback,
			   BIT(DT_INST_GPIO_PIN(0, int_gpios)));

	if (gpio_add_callback(drv_data->gpio, &drv_data->gpio_cb) < 0) {
		LOG_DBG("Failed to set gpio callback.");
		return -EIO;
	}

	drv_data->dev = dev;

#if defined(CONFIG_ISL29035_TRIGGER_OWN_THREAD)
	k_sem_init(&drv_data->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&drv_data->thread, drv_data->thread_stack,
			CONFIG_ISL29035_THREAD_STACK_SIZE,
			(k_thread_entry_t)isl29035_thread,
			drv_data, NULL, NULL,
			K_PRIO_COOP(CONFIG_ISL29035_THREAD_PRIORITY),
			0, K_NO_WAIT);
#elif defined(CONFIG_ISL29035_TRIGGER_GLOBAL_THREAD)
	drv_data->work.handler = isl29035_work_cb;
#endif

	setup_int(drv_data, true);

	return 0;
}
