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

#include <nanokernel.h>
#include <i2c.h>
#include <sensor.h>
#include <gpio.h>
#include <misc/util.h>
#include "sensor_sx9500.h"

#ifdef CONFIG_SX9500_TRIGGER_OWN_FIBER
static char __stack sx9500_fiber_stack[CONFIG_SX9500_FIBER_STACK_SIZE];
#endif

int sx9500_trigger_set(struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler)
{
	struct sx9500_data *data = dev->driver_data;
	int ret;

	switch (trig->type) {
	case SENSOR_TRIG_DATA_READY:
		ret = i2c_reg_update_byte(data->i2c_master,
					  data->i2c_slave_addr,
					  SX9500_REG_IRQ_MSK,
					  SX9500_CONV_DONE_IRQ,
					  SX9500_CONV_DONE_IRQ);
		if (ret)
			return ret;
		data->handler_drdy = handler;
		data->trigger_drdy = *trig;
		break;

	case SENSOR_TRIG_NEAR_FAR:
		ret = i2c_reg_update_byte(data->i2c_master,
					  data->i2c_slave_addr,
					  SX9500_REG_IRQ_MSK,
					  SX9500_NEAR_FAR_IRQ,
					  SX9500_NEAR_FAR_IRQ);
		if (ret)
			return ret;
		data->handler_near_far = handler;
		data->trigger_near_far = *trig;
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

#ifdef CONFIG_SX9500_TRIGGER_OWN_FIBER

static void sx9500_gpio_cb(struct device *port,
			   struct gpio_callback *cb, uint32_t pins)
{
	struct sx9500_data *data =
		CONTAINER_OF(cb, struct sx9500_data, gpio_cb);

	ARG_UNUSED(pins);

	nano_isr_sem_give(&data->sem);
}

static void sx9500_fiber_main(int arg1, int unused)
{
	struct device *dev = INT_TO_POINTER(arg1);
	struct sx9500_data *data = dev->driver_data;
	uint8_t reg_val;
	int ret;

	ARG_UNUSED(unused);

	while (1) {
		nano_fiber_sem_take(&data->sem, TICKS_UNLIMITED);

		ret = i2c_reg_read_byte(data->i2c_master, data->i2c_slave_addr,
					SX9500_REG_IRQ_SRC, &reg_val);
		if (ret) {
			SYS_LOG_DBG("sx9500: error %d reading IRQ source register", ret);
			continue;
		}

		if ((reg_val & SX9500_CONV_DONE_IRQ) && data->handler_drdy) {
			data->handler_drdy(dev, &data->trigger_drdy);
		}

		if ((reg_val & SX9500_NEAR_FAR_IRQ) && data->handler_near_far) {
			data->handler_near_far(dev, &data->trigger_near_far);
		}
	}
}

#else /* CONFIG_SX9500_TRIGGER_GLOBAL_FIBER */

static void sx9500_gpio_cb(struct device *port,
			   struct gpio_callback *cb, uint32_t pins)
{
	struct sx9500_data *data =
		CONTAINER_OF(cb, struct sx9500_data, gpio_cb);

	ARG_UNUSED(pins);

	nano_isr_fifo_put(sensor_get_work_fifo(), &data->work);
}

static void sx9500_gpio_fiber_cb(void *arg)
{
	struct device *dev = arg;
	struct sx9500_data *data = dev->driver_data;
	uint8_t reg_val;
	int ret;

	ret = i2c_reg_read_byte(data->i2c_master, data->i2c_slave_addr,
				SX9500_REG_IRQ_SRC, &reg_val);
	if (ret) {
		SYS_LOG_DBG("sx9500: error %d reading IRQ source register", ret);
		return;
	}

	if ((reg_val & SX9500_CONV_DONE_IRQ) && data->handler_drdy) {
		data->handler_drdy(dev, &data->trigger_drdy);
	}

	if ((reg_val & SX9500_NEAR_FAR_IRQ) && data->handler_near_far) {
		data->handler_near_far(dev, &data->trigger_near_far);
	}
}
#endif /* CONFIG_SX9500_TRIGGER_GLOBAL_FIBER */

int sx9500_setup_interrupt(struct device *dev)
{
	struct sx9500_data *data = dev->driver_data;
	struct device *gpio;

#ifdef CONFIG_SX9500_TRIGGER_OWN_FIBER
	nano_sem_init(&data->sem);
#else
	data->work.handler = sx9500_gpio_fiber_cb;
	data->work.arg = dev;
#endif

	gpio = device_get_binding(CONFIG_SX9500_GPIO_CONTROLLER);
	if (!gpio) {
		SYS_LOG_DBG("sx9500: gpio controller %s not found",
			    CONFIG_SX9500_GPIO_CONTROLLER);
		return -EINVAL;
	}

	gpio_pin_configure(gpio, CONFIG_SX9500_GPIO_PIN,
			   GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			   GPIO_INT_ACTIVE_LOW | GPIO_INT_DEBOUNCE);

	gpio_init_callback(&data->gpio_cb,
			   sx9500_gpio_cb,
			   BIT(CONFIG_SX9500_GPIO_PIN));

	gpio_add_callback(gpio, &data->gpio_cb);
	gpio_pin_enable_callback(gpio, CONFIG_SX9500_GPIO_PIN);

#ifdef CONFIG_SX9500_TRIGGER_OWN_FIBER
	fiber_fiber_start(sx9500_fiber_stack, CONFIG_SX9500_FIBER_STACK_SIZE,
			  sx9500_fiber_main, POINTER_TO_INT(dev), 0,
			  CONFIG_SX9500_FIBER_PRIORITY, 0);
#endif

	return 0;
}
