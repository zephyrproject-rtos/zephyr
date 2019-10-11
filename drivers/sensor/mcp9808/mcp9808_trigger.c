/* sensor_mcp9808_trigger.c - Trigger support for MCP9808 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <kernel.h>
#include <drivers/i2c.h>
#include <sys/byteorder.h>
#include <sys/__assert.h>
#include <logging/log.h>
#include "mcp9808.h"

LOG_MODULE_DECLARE(MCP9808, CONFIG_SENSOR_LOG_LEVEL);

static int mcp9808_reg_write(struct mcp9808_data *data, u8_t reg, u16_t val)
{
	u16_t be_val = sys_cpu_to_be16(val);

	struct i2c_msg msgs[2] = {
		{
			.buf = &reg,
			.len = 1,
			.flags = I2C_MSG_WRITE | I2C_MSG_RESTART,
		},
		{
			.buf = (u8_t *) &be_val,
			.len = 2,
			.flags = I2C_MSG_WRITE | I2C_MSG_STOP,
		},
	};

	return i2c_transfer(data->i2c_master, msgs, 2, data->i2c_slave_addr);
}

static int mcp9808_reg_update(struct mcp9808_data *data, u8_t reg,
			      u16_t mask, u16_t val)
{
	u16_t old_val, new_val;

	if (mcp9808_reg_read(data, reg, &old_val) < 0) {
		return -EIO;
	}

	new_val = old_val & ~mask;
	new_val |= val;

	if (new_val == old_val) {
		return 0;
	}

	return mcp9808_reg_write(data, reg, new_val);
}

int mcp9808_attr_set(struct device *dev, enum sensor_channel chan,
		     enum sensor_attribute attr,
		     const struct sensor_value *val)
{
	struct mcp9808_data *data = dev->driver_data;
	u16_t reg_val = 0U;
	u8_t reg_addr;
	s32_t val2;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_AMBIENT_TEMP);

	val2 = val->val2;
	while (val2 > 0) {
		reg_val += (1 << 2);
		val2 -= 250000;
	}
	reg_val |= val->val1 << 4;

	switch (attr) {
	case SENSOR_ATTR_LOWER_THRESH:
		reg_addr = MCP9808_REG_LOWER_LIMIT;
		break;
	case SENSOR_ATTR_UPPER_THRESH:
		reg_addr = MCP9808_REG_UPPER_LIMIT;
		break;
	default:
		return -EINVAL;
	}

	return mcp9808_reg_write(data, reg_addr, reg_val);
}

int mcp9808_trigger_set(struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
	struct mcp9808_data *data = dev->driver_data;

	data->trig = *trig;
	data->trigger_handler = handler;

	return mcp9808_reg_update(data, MCP9808_REG_CONFIG, MCP9808_ALERT_CNT,
				  handler == NULL ? 0 : MCP9808_ALERT_CNT);
}

#ifdef CONFIG_MCP9808_TRIGGER_OWN_THREAD

static void mcp9808_gpio_cb(struct device *dev,
			    struct gpio_callback *cb, u32_t pins)
{
	struct mcp9808_data *data =
		CONTAINER_OF(cb, struct mcp9808_data, gpio_cb);

	ARG_UNUSED(pins);

	k_sem_give(&data->sem);
}

static void mcp9808_thread_main(int arg1, int arg2)
{
	struct device *dev = INT_TO_POINTER(arg1);
	struct mcp9808_data *data = dev->driver_data;

	ARG_UNUSED(arg2);

	while (1) {
		k_sem_take(&data->sem, K_FOREVER);
		data->trigger_handler(dev, &data->trig);
		mcp9808_reg_update(data, MCP9808_REG_CONFIG,
				   MCP9808_INT_CLEAR, MCP9808_INT_CLEAR);
	}
}

static K_THREAD_STACK_DEFINE(mcp9808_thread_stack, CONFIG_MCP9808_THREAD_STACK_SIZE);
static struct k_thread mcp9808_thread;
#else /* CONFIG_MCP9808_TRIGGER_GLOBAL_THREAD */

static void mcp9808_gpio_cb(struct device *dev,
			    struct gpio_callback *cb, u32_t pins)
{
	struct mcp9808_data *data =
		CONTAINER_OF(cb, struct mcp9808_data, gpio_cb);

	ARG_UNUSED(pins);

	k_work_submit(&data->work);
}

static void mcp9808_gpio_thread_cb(struct k_work *work)
{
	struct mcp9808_data *data =
		CONTAINER_OF(work, struct mcp9808_data, work);
	struct device *dev = data->dev;

	data->trigger_handler(dev, &data->trig);
	mcp9808_reg_update(data, MCP9808_REG_CONFIG,
			   MCP9808_INT_CLEAR, MCP9808_INT_CLEAR);
}

#endif /* CONFIG_MCP9808_TRIGGER_GLOBAL_THREAD */

void mcp9808_setup_interrupt(struct device *dev)
{
	struct mcp9808_data *data = dev->driver_data;
	struct device *gpio;

	mcp9808_reg_update(data, MCP9808_REG_CONFIG, MCP9808_ALERT_INT,
			   MCP9808_ALERT_INT);
	mcp9808_reg_write(data, MCP9808_REG_CRITICAL, MCP9808_TEMP_MAX);
	mcp9808_reg_update(data, MCP9808_REG_CONFIG, MCP9808_INT_CLEAR,
			   MCP9808_INT_CLEAR);

#ifdef CONFIG_MCP9808_TRIGGER_OWN_THREAD
	k_sem_init(&data->sem, 0, UINT_MAX);

	k_thread_create(&mcp9808_thread, mcp9808_thread_stack,
			CONFIG_MCP9808_THREAD_STACK_SIZE,
			(k_thread_entry_t)mcp9808_thread_main, dev, 0, NULL,
			K_PRIO_COOP(CONFIG_MCP9808_THREAD_PRIORITY), 0, K_NO_WAIT);
#else /* CONFIG_MCP9808_TRIGGER_GLOBAL_THREAD */
	data->work.handler = mcp9808_gpio_thread_cb;
	data->dev = dev;
#endif

	gpio = device_get_binding(CONFIG_MCP9808_GPIO_CONTROLLER);
	gpio_pin_configure(gpio, CONFIG_MCP9808_GPIO_PIN,
			   GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			   GPIO_INT_ACTIVE_LOW | GPIO_INT_DEBOUNCE);

	gpio_init_callback(&data->gpio_cb,
			   mcp9808_gpio_cb,
			   BIT(CONFIG_MCP9808_GPIO_PIN));

	gpio_add_callback(gpio, &data->gpio_cb);
	gpio_pin_enable_callback(gpio, CONFIG_MCP9808_GPIO_PIN);
}
