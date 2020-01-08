/*
 * Copyright (c) 2020 Intive
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/i2c.h>
#include <sys/byteorder.h>
#include <sys/util.h>
#include <kernel.h>
#include <logging/log.h>

#include "isl29125.h"

LOG_MODULE_DECLARE(ISL29125, CONFIG_SENSOR_LOG_LEVEL);

static int isl29125_write16(struct device *i2c_dev, u8_t reg, u16_t data)
{
	u8_t buf[3];

	data = sys_cpu_to_le16(data);
	buf[0] = reg;
	buf[1] = data & 0xFF;
	buf[2] = data >> 8;
	int result = i2c_write(i2c_dev, buf, 3, ISL29125_I2C_ADDR);

	if (result) {
		LOG_ERR("Error writing register %d at Addr:0x%x", reg,
			ISL29125_I2C_ADDR);
	}

	return result;
}

int isl29125_attr_set(struct device *dev, enum sensor_channel chan,
		      enum sensor_attribute attr,
		      const struct sensor_value *val)
{
	struct isl29125_data *drv_data = dev->driver_data;

	drv_data->dev_config_3 &= ~(ISL29125_CFG3_TH_IRQ_MASK);

	switch (chan) {
	case SENSOR_CHAN_RED:
		drv_data->dev_config_3 |= ISL29125_CFG3_R_INT;
		LOG_INF("Threshold for channel RED");
		break;
	case SENSOR_CHAN_GREEN:
		drv_data->dev_config_3 |= ISL29125_CFG3_G_INT;
		LOG_INF("Threshold for channel GREEN");
		break;
	case SENSOR_CHAN_BLUE:
		drv_data->dev_config_3 |= ISL29125_CFG3_B_INT;
		LOG_INF("Threshold for channel BLUE");
		break;
	default:
		return -EINVAL;
	}

	drv_data->dev_config_3 &= ~(ISL29125_CFG3_INT_MASK);
	drv_data->dev_config_3 |= ISL29125_CFG3_INT_PRST8;

	if (attr == SENSOR_ATTR_UPPER_THRESH) {
		LOG_INF("Upper threshold set to: %d", val->val1);
		if (isl29125_write16(drv_data->i2c, ISL29125_THRESHOLD_HL,
				     val->val1) != 0) {
			LOG_ERR("Threshold set failed");
			return -EIO;
		}

	} else if (attr == SENSOR_ATTR_LOWER_THRESH) {
		LOG_INF("Lower threshold set to: %d", val->val1);
		if (isl29125_write16(drv_data->i2c, ISL29125_THRESHOLD_LL,
				     val->val1) != 0) {
			LOG_ERR("Threshold set failed");
			return -EIO;
		}
	} else {
		return -EINVAL;
	}

	return isl29125_set_config(drv_data);
}

static void isl29125_gpio_callback(struct device *dev, struct gpio_callback *cb,
				   u32_t pins)
{
	ARG_UNUSED(pins);

	struct isl29125_data *drv_data =
		CONTAINER_OF(cb, struct isl29125_data, gpio_callback);
	gpio_pin_disable_callback(dev, DT_INST_0_ISIL_ISL29125_INT_GPIOS_PIN);

	LOG_ERR("Trigger INT");

#if defined(CONFIG_ISL29125_TRIGGER_OWN_THREAD)
	k_sem_give(&drv_data->gpio_sem);
#elif defined(CONFIG_ISL29125_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&drv_data->work_item);
#endif
}

static void isl29125_on_trigger(struct device *dev)
{
	struct isl29125_data *drv_data = dev->driver_data;
	u8_t val;

	/* clear interrupt */
	if (i2c_reg_read_byte(drv_data->i2c, ISL29125_I2C_ADDR, ISL29125_STATUS,
			      &val) < 0) {
		LOG_ERR("isl29125: Error reading status register");
		return;
	}
	if (drv_data->handler != NULL) {
		drv_data->handler(dev, &drv_data->trigger);
	}

	gpio_pin_enable_callback(drv_data->gpio,
				 DT_INST_0_ISIL_ISL29125_INT_GPIOS_PIN);
}

#ifdef CONFIG_ISL29125_TRIGGER_OWN_THREAD
static void isl29125_thread_main(int ptr, int unused)
{
	struct device *dev = INT_TO_POINTER(ptr);
	struct isl29125_data *drv_data = dev->driver_data;

	ARG_UNUSED(unused);

	while (true) {
		k_sem_take(&drv_data->gpio_sem, K_FOREVER);
		isl29125_on_trigger(dev);
	}
}
#endif

#ifdef CONFIG_ISL29125_TRIGGER_GLOBAL_THREAD
static void isl29125_work_item_callback(struct k_work *work)
{
	struct isl29125_data *drv_data =
		CONTAINER_OF(work, struct isl29125_data, work_item);

	isl29125_on_trigger(drv_data->device);
}
#endif

int isl29125_trigger_set(struct device *dev, const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler)
{
	struct isl29125_data *drv_data = dev->driver_data;

	LOG_INF("Trigger setup");
	/* disable interrupt callback while changing parameters */
	gpio_pin_disable_callback(drv_data->gpio,
				  DT_INST_0_ISIL_ISL29125_INT_GPIOS_PIN);

	drv_data->handler = handler;
	drv_data->trigger = *trig;

	/* enable interrupt callback */
	gpio_pin_enable_callback(drv_data->gpio,
				 DT_INST_0_ISIL_ISL29125_INT_GPIOS_PIN);

	return 0;
}

int isl29125_init_interrupt(struct device *dev)
{
	struct isl29125_data *drv_data = dev->driver_data;

	LOG_INF("Configuring trigger submodule");
	/* update config for interrupt persistence */
	drv_data->dev_config_3 &= ~(ISL29125_CFG3_INT_MASK);
	drv_data->dev_config_3 |= ISL29125_CFG3_INT_PRST8;
	/*
	 * Note: No need to call isl29125_set_config() will be called after
	 * this method
	 */

	/* setup gpio interrupt */
	drv_data->gpio = device_get_binding(
		DT_INST_0_ISIL_ISL29125_INT_GPIOS_CONTROLLER);
	if (drv_data->gpio == NULL) {
		LOG_ERR("Failed to get GPIO device.");
		return -EINVAL;
	}

	gpio_pin_configure(drv_data->gpio,
			   DT_INST_0_ISIL_ISL29125_INT_GPIOS_PIN,
			   GPIO_DIR_IN | GPIO_INT | GPIO_INT_LEVEL |
			   GPIO_INT_ACTIVE_LOW | GPIO_INT_DEBOUNCE);

	gpio_init_callback(&drv_data->gpio_callback, isl29125_gpio_callback,
			   BIT(DT_INST_0_ISIL_ISL29125_INT_GPIOS_PIN));

	if (gpio_add_callback(drv_data->gpio, &drv_data->gpio_callback) < 0) {
		LOG_ERR("Failed to set gpio callback.");
		return -EIO;
	}
#if defined(CONFIG_ISL29125_TRIGGER_OWN_THREAD)
	k_sem_init(&drv_data->gpio_sem, 0, UINT_MAX);

	k_thread_create(&drv_data->thread, drv_data->thread_stack,
			CONFIG_ISL29125_THREAD_STACK_SIZE,
			(k_thread_entry_t)isl29125_thread_main, dev, 0, NULL,
			K_PRIO_COOP(CONFIG_ISL29125_THREAD_PRIORITY), 0,
			K_NO_WAIT);
#elif defined(CONFIG_ISL29125_TRIGGER_GLOBAL_THREAD)
	drv_data->work_item.handler = isl29125_work_item_callback;
	drv_data->device = dev;
#endif

	return 0;
}
