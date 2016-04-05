/* sensor_lsm9ds0_gyro.c - Driver for LSM9DS0 gyroscope sensor */

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

#ifdef CONFIG_SENSOR_DEBUG
#include <misc/printk.h>
#define sensor_dbg(fmt, ...) printk("lsm9ds0_gyro: " fmt, ##__VA_ARGS__)
#else
#define sensor_dbg(fmt, ...) do { } while (0)
#endif /* CONFIG_SENSOR_DEBUG */

static inline int lsm9ds0_gyro_power_ctrl(struct device *dev, int power,
					  int x_en, int y_en, int z_en)
{
	struct lsm9ds0_gyro_data *data = dev->driver_data;
	struct lsm9ds0_gyro_config *config = dev->config->config_info;
	uint8_t state = (power << LSM9DS0_GYRO_SHIFT_CTRL_REG1_G_PD) |
			(x_en << LSM9DS0_GYRO_SHIFT_CTRL_REG1_G_XEN) |
			(y_en << LSM9DS0_GYRO_SHIFT_CTRL_REG1_G_YEN) |
			(z_en << LSM9DS0_GYRO_SHIFT_CTRL_REG1_G_ZEN);

	return i2c_reg_update_byte(data->i2c_master, config->i2c_slave_addr,
				   LSM9DS0_GYRO_REG_CTRL_REG1_G,
				   LSM9DS0_GYRO_MASK_CTRL_REG1_G_PD |
				   LSM9DS0_GYRO_MASK_CTRL_REG1_G_XEN |
				   LSM9DS0_GYRO_MASK_CTRL_REG1_G_YEN |
				   LSM9DS0_GYRO_MASK_CTRL_REG1_G_ZEN,
				   state);
}

static int lsm9ds0_gyro_set_fs_raw(struct device *dev, uint8_t fs)
{
	struct lsm9ds0_gyro_data *data = dev->driver_data;
	struct lsm9ds0_gyro_config *config = dev->config->config_info;

	if (i2c_reg_update_byte(data->i2c_master, config->i2c_slave_addr,
				LSM9DS0_GYRO_REG_CTRL_REG4_G,
				LSM9DS0_GYRO_MASK_CTRL_REG4_G_FS,
				fs << LSM9DS0_GYRO_SHIFT_CTRL_REG4_G_FS) != 0) {
		return -EIO;
	}

#if defined(CONFIG_LSM9DS0_GYRO_FULLSCALE_RUNTIME)
	data->fs = fs;
#endif

	return 0;
}

#if defined(CONFIG_LSM9DS0_GYRO_FULLSCALE_RUNTIME)
static const struct {
	int fs;
	uint8_t reg_val;
} lsm9ds0_gyro_fs_table[] = { {245, 0},
			      {500, 1},
			      {2000, 2} };

static int lsm9ds0_gyro_set_fs(struct device *dev, int fs)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(lsm9ds0_gyro_fs_table); ++i) {
		if (fs <= lsm9ds0_gyro_fs_table[i].fs) {
			return lsm9ds0_gyro_set_fs_raw(dev, lsm9ds0_gyro_fs_table[i].reg_val);
		}
	}

	return -ENOTSUP;
}
#endif

static inline int lsm9ds0_gyro_set_odr_raw(struct device *dev, uint8_t odr)
{
	struct lsm9ds0_gyro_data *data = dev->driver_data;
	struct lsm9ds0_gyro_config *config = dev->config->config_info;

	return i2c_reg_update_byte(data->i2c_master, config->i2c_slave_addr,
				   LSM9DS0_GYRO_REG_CTRL_REG1_G,
				   LSM9DS0_GYRO_MASK_CTRL_REG1_G_DR,
				   odr << LSM9DS0_GYRO_SHIFT_CTRL_REG1_G_BW);
}

#if defined(CONFIG_LSM9DS0_GYRO_SAMPLING_RATE_RUNTIME)
static const struct {
	int freq;
	uint8_t reg_val;
} lsm9ds0_gyro_samp_freq_table[] = { {95, 0},
				     {190, 1},
				     {380, 2},
				     {760, 3} };

static int lsm9ds0_gyro_set_odr(struct device *dev, int odr)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(lsm9ds0_gyro_samp_freq_table); ++i) {
		if (odr <= lsm9ds0_gyro_samp_freq_table[i].freq) {
			return lsm9ds0_gyro_set_odr_raw(dev,
							lsm9ds0_gyro_samp_freq_table[i].
							reg_val);
		}
	}

	return -ENOTSUP;
}
#endif

static int lsm9ds0_gyro_sample_fetch(struct device *dev,
				     enum sensor_channel chan)
{
	struct lsm9ds0_gyro_data *data = dev->driver_data;
	struct lsm9ds0_gyro_config *config = dev->config->config_info;
	uint8_t x_l, x_h, y_l, y_h, z_l, z_h;

	__ASSERT(chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_GYRO_ANY);

	if (i2c_reg_read_byte(data->i2c_master, config->i2c_slave_addr,
			      LSM9DS0_GYRO_REG_OUT_X_L_G, &x_l) != 0 ||
	    i2c_reg_read_byte(data->i2c_master, config->i2c_slave_addr,
			      LSM9DS0_GYRO_REG_OUT_X_H_G, &x_h) != 0 ||
	    i2c_reg_read_byte(data->i2c_master, config->i2c_slave_addr,
			      LSM9DS0_GYRO_REG_OUT_Y_L_G, &y_l) != 0 ||
	    i2c_reg_read_byte(data->i2c_master, config->i2c_slave_addr,
			      LSM9DS0_GYRO_REG_OUT_Y_H_G, &y_h) != 0 ||
	    i2c_reg_read_byte(data->i2c_master, config->i2c_slave_addr,
			      LSM9DS0_GYRO_REG_OUT_Z_L_G, &z_l) != 0 ||
	    i2c_reg_read_byte(data->i2c_master, config->i2c_slave_addr,
			      LSM9DS0_GYRO_REG_OUT_Z_H_G, &z_h) != 0) {
		sensor_dbg("failed to read sample\n");
		return -EIO;
	}

	data->sample_x = (int16_t)((uint16_t)(x_l) | ((uint16_t)(x_h) << 8));
	data->sample_y = (int16_t)((uint16_t)(y_l) | ((uint16_t)(y_h) << 8));
	data->sample_z = (int16_t)((uint16_t)(z_l) | ((uint16_t)(z_h) << 8));

#if defined(CONFIG_LSM9DS0_GYRO_FULLSCALE_RUNTIME)
	data->sample_fs = data->fs;
#endif

	return 0;
}

static inline int lsm9ds0_gyro_get_channel(enum sensor_channel chan,
					   struct sensor_value *val,
					   struct lsm9ds0_gyro_data *data,
					   float numerator)
{
	switch (chan) {
	case SENSOR_CHAN_GYRO_X:
		val->dval = (double)(data->sample_x) *
			numerator / 1000.0 * DEG2RAD;
		break;
	case SENSOR_CHAN_GYRO_Y:
		val->dval = (double)(data->sample_y) *
			numerator / 1000.0 * DEG2RAD;
		break;
	case SENSOR_CHAN_GYRO_Z:
		val->dval = (double)(data->sample_z) *
			numerator / 1000.0 * DEG2RAD;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int lsm9ds0_gyro_channel_get(struct device *dev,
				    enum sensor_channel chan,
				    struct sensor_value *val)
{
	struct lsm9ds0_gyro_data *data = dev->driver_data;

	val->type = SENSOR_VALUE_TYPE_DOUBLE;

#if defined(CONFIG_LSM9DS0_GYRO_FULLSCALE_RUNTIME)
	switch (data->sample_fs) {
	case 0:
		return lsm9ds0_gyro_get_channel(chan, val, data, 8.75f);
	case 1:
		return lsm9ds0_gyro_get_channel(chan, val, data, 17.50f);
	default:
		return lsm9ds0_gyro_get_channel(chan, val, data, 70.0f);
	}
#elif defined(CONFIG_LSM9DS0_GYRO_FULLSCALE_245)
	return lsm9ds0_gyro_get_channel(chan, val, data, 8.75f);
#elif defined(CONFIG_LSM9DS0_GYRO_FULLSCALE_500)
	return lsm9ds0_gyro_get_channel(chan, val, data, 17.50f);
#elif defined(CONFIG_LSM9DS0_GYRO_FULLSCALE_2000)
	return lsm9ds0_gyro_get_channel(chan, val, data, 70.0f);
#endif
	return 0;
}

#if defined(LSM9DS0_GYRO_SET_ATTR)
static int lsm9ds0_gyro_attr_set(struct device *dev,
				 enum sensor_channel chan,
				 enum sensor_attribute attr,
				 const struct sensor_value *val)
{
	switch (attr) {
#if defined(CONFIG_LSM9DS0_GYRO_FULLSCALE_RUNTIME)
	case SENSOR_ATTR_FULL_SCALE:
		if (val->type != SENSOR_VALUE_TYPE_INT &&
		    val->type != SENSOR_VALUE_TYPE_INT_PLUS_MICRO) {
			return -ENOTSUP;
		}

		if (lsm9ds0_gyro_set_fs(dev, sensor_rad_to_degrees(val)) != 0) {
			sensor_dbg("full-scale value not supported\n");
			return -EIO;
		}
		break;
#endif
#if defined(CONFIG_LSM9DS0_GYRO_SAMPLING_RATE_RUNTIME)
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		if (val->type != SENSOR_VALUE_TYPE_INT) {
			return -ENOTSUP;
		}

		if (lsm9ds0_gyro_set_odr(dev, val->val1) != 0) {
			sensor_dbg("sampling frequency value not supported\n");
			return -EIO;
		}
		break;
#endif
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif

#if defined(CONFIG_LSM9DS0_GYRO_TRIGGERS)
static int lsm9ds0_gyro_trigger_set(struct device *dev,
				    const struct sensor_trigger *trig,
				    sensor_trigger_handler_t handler)
{
#if defined(CONFIG_LSM9DS0_GYRO_TRIGGER_DRDY)
	struct lsm9ds0_gyro_data *data = dev->driver_data;
	const struct lsm9ds0_gyro_config * const config =
					 dev->config->config_info;
	uint8_t state;
#endif

#if defined(CONFIG_LSM9DS0_GYRO_TRIGGER_DRDY)
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
#endif

	return -ENOTSUP;
}
#endif

#if defined(CONFIG_LSM9DS0_GYRO_TRIGGER_DRDY)
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
#endif

static struct sensor_driver_api lsm9ds0_gyro_api_funcs = {
	.sample_fetch = lsm9ds0_gyro_sample_fetch,
	.channel_get = lsm9ds0_gyro_channel_get,
#if defined(LSM9DS0_GYRO_SET_ATTR)
	.attr_set = lsm9ds0_gyro_attr_set,
#endif
#if defined(CONFIG_LSM9DS0_GYRO_TRIGGERS)
	.trigger_set = lsm9ds0_gyro_trigger_set,
#endif
};

static int lsm9ds0_gyro_init_chip(struct device *dev)
{
	struct lsm9ds0_gyro_data *data = dev->driver_data;
	struct lsm9ds0_gyro_config *config = dev->config->config_info;
	uint8_t chip_id;

	if (lsm9ds0_gyro_power_ctrl(dev, 0, 0, 0, 0) != 0) {
		sensor_dbg("failed to power off device\n");
		return -EIO;
	}

	if (lsm9ds0_gyro_power_ctrl(dev, 1, 1, 1, 1) != 0) {
		sensor_dbg("failed to power on device\n");
		return -EIO;
	}

	if (i2c_reg_read_byte(data->i2c_master, config->i2c_slave_addr,
			      LSM9DS0_GYRO_REG_WHO_AM_I_G, &chip_id) != 0) {
		sensor_dbg("failed reading chip id\n");
		goto err_poweroff;
	}
	if (chip_id != LSM9DS0_GYRO_VAL_WHO_AM_I_G) {
		sensor_dbg("invalid chip id 0x%x\n", chip_id);
		goto err_poweroff;
	}
	sensor_dbg("chip id 0x%x\n", chip_id);

	if (lsm9ds0_gyro_set_fs_raw(dev, LSM9DS0_GYRO_DEFAULT_FULLSCALE) != 0) {
		sensor_dbg("failed to set full-scale\n");
		goto err_poweroff;
	}

	if (lsm9ds0_gyro_set_odr_raw(dev, LSM9DS0_GYRO_DEFAULT_SAMPLING_RATE)
				     != 0) {
		sensor_dbg("failed to set sampling rate\n");
		goto err_poweroff;
	}

	if (i2c_reg_update_byte(data->i2c_master, config->i2c_slave_addr,
				LSM9DS0_GYRO_REG_CTRL_REG4_G,
				LSM9DS0_GYRO_MASK_CTRL_REG4_G_BDU |
				LSM9DS0_GYRO_MASK_CTRL_REG4_G_BLE,
				(1 << LSM9DS0_GYRO_SHIFT_CTRL_REG4_G_BDU) |
				(0 << LSM9DS0_GYRO_SHIFT_CTRL_REG4_G_BLE))
				!= 0) {
		sensor_dbg("failed to set BDU and BLE\n");
		goto err_poweroff;
	}

	return 0;

err_poweroff:
	lsm9ds0_gyro_power_ctrl(dev, 0, 0, 0, 0);
	return -EIO;
}

int lsm9ds0_gyro_init(struct device *dev)
{
	const struct lsm9ds0_gyro_config * const config =
					   dev->config->config_info;
	struct lsm9ds0_gyro_data *data = dev->driver_data;

	data->i2c_master = device_get_binding(config->i2c_master_dev_name);
	if (!data->i2c_master) {
		sensor_dbg("i2c master not found: %s\n",
			   config->i2c_master_dev_name);
		return -EINVAL;
	}

	if (lsm9ds0_gyro_init_chip(dev) != 0) {
		sensor_dbg("failed to initialize chip\n");
		return -EIO;
	}

#if defined(CONFIG_LSM9DS0_GYRO_TRIGGER_DRDY)
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

	data->dev = dev;
#endif

	dev->driver_api = &lsm9ds0_gyro_api_funcs;

	return 0;
}

static struct lsm9ds0_gyro_config lsm9ds0_gyro_config = {
	.i2c_master_dev_name = CONFIG_LSM9DS0_GYRO_I2C_MASTER_DEV_NAME,
	.i2c_slave_addr = LSM9DS0_GYRO_I2C_ADDR,
#if defined(CONFIG_LSM9DS0_GYRO_TRIGGER_DRDY)
	.gpio_drdy_dev_name = CONFIG_LSM9DS0_GYRO_GPIO_DRDY_DEV_NAME,
	.gpio_drdy_int_pin = CONFIG_LSM9DS0_GYRO_GPIO_DRDY_INT_PIN,
#endif
};

struct lsm9ds0_gyro_data lsm9ds0_gyro_data;

DEVICE_INIT(lsm9ds0_gyro, CONFIG_LSM9DS0_GYRO_DEV_NAME, lsm9ds0_gyro_init,
	    &lsm9ds0_gyro_data, &lsm9ds0_gyro_config, SECONDARY,
	    CONFIG_LSM9DS0_GYRO_INIT_PRIORITY);
