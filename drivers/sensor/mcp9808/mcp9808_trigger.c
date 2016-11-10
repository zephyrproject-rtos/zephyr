/* sensor_mcp9808_trigger.c - Trigger support for MCP9808 */

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

#include <errno.h>
#include <kernel.h>
#include <i2c.h>
#include <misc/byteorder.h>
#include <misc/__assert.h>

#include "mcp9808.h"

static int mcp9808_reg_write(struct mcp9808_data *data, uint8_t reg, uint16_t val)
{
	uint16_t be_val = sys_cpu_to_be16(val);

	struct i2c_msg msgs[2] = {
		{
			.buf = &reg,
			.len = 1,
			.flags = I2C_MSG_WRITE | I2C_MSG_RESTART,
		},
		{
			.buf = (uint8_t *) &be_val,
			.len = 2,
			.flags = I2C_MSG_WRITE | I2C_MSG_STOP,
		},
	};

	return i2c_transfer(data->i2c_master, msgs, 2, data->i2c_slave_addr);
}

static int mcp9808_reg_update(struct mcp9808_data *data, uint8_t reg,
			      uint16_t mask, uint16_t val)
{
	uint16_t old_val, new_val;

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
	uint16_t reg_val = 0;
	uint8_t reg_addr;
	int32_t val2;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_TEMP);

	switch (val->type) {
	case SENSOR_VALUE_TYPE_INT_PLUS_MICRO:
		val2 = val->val2;
		while (val2 > 0) {
			reg_val += (1 << 2);
			val2 -= 250000;
		}
		/* Fall through. */
	case SENSOR_VALUE_TYPE_INT:
		reg_val |= val->val1 << 4;
		break;
	default:
		return -EINVAL;
	}

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
			    struct gpio_callback *cb, uint32_t pins)
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

static char __stack mcp9808_thread_stack[CONFIG_MCP9808_THREAD_STACK_SIZE];

#else /* CONFIG_MCP9808_TRIGGER_GLOBAL_THREAD */

static void mcp9808_gpio_cb(struct device *dev,
			    struct gpio_callback *cb, uint32_t pins)
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

	k_thread_spawn(mcp9808_thread_stack,
			  CONFIG_MCP9808_THREAD_STACK_SIZE,
			  mcp9808_thread_main, POINTER_TO_INT(dev), 0, NULL,
			  K_PRIO_COOP(CONFIG_MCP9808_THREAD_PRIORITY), 0, 0);
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
