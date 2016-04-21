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

 #include <sensor.h>
 #include <nanokernel.h>
 #include <device.h>
 #include <init.h>
 #include <i2c.h>
 #include <misc/byteorder.h>
 #include <misc/__assert.h>

 #include <gpio.h>

 #include "sensor_lsm9ds0_gyro.h"

extern struct lsm9ds0_gyro_data lsm9ds0_gyro_data;

int lsm9ds0_gyro_trigger_set(struct device *dev,
			     const struct sensor_trigger *trig,
			     sensor_trigger_handler_t handler)
{
	struct lsm9ds0_gyro_data *data = dev->driver_data;
	const struct lsm9ds0_gyro_config * const config =
					 dev->config->config_info;
	uint8_t state;

	if (trig->type == SENSOR_TRIG_DATA_READY) {
		gpio_pin_disable_callback(data->gpio_drdy,
					  config->gpio_drdy_int_pin);

		state = 0;
		if (handler) {
			state = 1;
		}

		data->handler_drdy = handler;
		data->trigger_drdy = *trig;

		if (i2c_reg_update_byte(data->i2c_master,
					config->i2c_slave_addr,
					LSM9DS0_GYRO_REG_CTRL_REG3_G,
					LSM9DS0_GYRO_MASK_CTRL_REG3_G_I2_DRDY,
					state << LSM9DS0_GYRO_SHIFT_CTRL_REG3_G_I2_DRDY)
					!= 0) {
			sensor_dbg("failed to set DRDY interrupt\n");
			return -EIO;
		}

		gpio_pin_enable_callback(data->gpio_drdy,
					 config->gpio_drdy_int_pin);
		return 0;
	}

	return -ENOTSUP;
}

static void lsm9ds0_gyro_gpio_drdy_callback(struct device *dev,
					    struct gpio_callback *cb, uint32_t pins)
{
	struct lsm9ds0_gyro_data *data =
		CONTAINER_OF(cb, struct lsm9ds0_gyro_data, gpio_cb);
	const struct lsm9ds0_gyro_config * const config =
		data->dev->config->config_info;

	gpio_pin_disable_callback(dev, config->gpio_drdy_int_pin);

	nano_isr_sem_give(&data->sem);
}

static void lsm9ds0_gyro_fiber_main(int arg1, int gpio_pin)
{
	struct device *dev = (struct device *) arg1;
	struct lsm9ds0_gyro_data *data = dev->driver_data;

	while (1) {
		nano_fiber_sem_take(&data->sem, TICKS_UNLIMITED);

		if (data->handler_drdy) {
			data->handler_drdy(dev, &data->trigger_drdy);
		}

		gpio_pin_enable_callback(data->gpio_drdy, gpio_pin);
	}
}

int lsm9ds0_gyro_init_interrupt(struct device *dev)
{
	const struct lsm9ds0_gyro_config * const config =
					   dev->config->config_info;
	struct lsm9ds0_gyro_data *data = dev->driver_data;

	nano_sem_init(&data->sem);

	task_fiber_start(data->fiber_stack,
			 CONFIG_LSM9DS0_GYRO_FIBER_STACK_SIZE,
			 lsm9ds0_gyro_fiber_main, (int) dev,
			 config->gpio_drdy_int_pin, 10, 0);

	data->gpio_drdy = device_get_binding(config->gpio_drdy_dev_name);
	if (!data->gpio_drdy) {
		sensor_dbg("gpio controller %s not found\n",
			   config->gpio_drdy_dev_name);
		return -EINVAL;
	}

	gpio_pin_configure(data->gpio_drdy, config->gpio_drdy_int_pin,
			   GPIO_DIR_IN | GPIO_INT |
			   GPIO_INT_ACTIVE_HIGH | GPIO_INT_DEBOUNCE);

	gpio_init_callback(&data->gpio_cb,
			   lsm9ds0_gyro_gpio_drdy_callback,
			   BIT(config->gpio_drdy_int_pin));

	if (gpio_add_callback(data->gpio_drdy, &data->gpio_cb) != 0) {
		sensor_dbg("failed to set gpio callback\n");
		return -EINVAL;
	}

	return 0;
}
