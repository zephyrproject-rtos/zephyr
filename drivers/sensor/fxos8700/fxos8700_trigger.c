/*
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
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

#include <fxos8700.h>

static void fxos8700_gpio_callback(struct device *dev,
				   struct gpio_callback *cb,
				   uint32_t pin)
{
	struct fxos8700_data *data =
		CONTAINER_OF(cb, struct fxos8700_data, gpio_cb);

	gpio_pin_disable_callback(dev, pin);

#if defined(CONFIG_FXOS8700_TRIGGER_OWN_THREAD)
	k_sem_give(&data->trig_sem);
#elif defined(CONFIG_FXOS8700_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
}

static int fxos8700_handle_drdy_int(struct device *dev)
{
	struct fxos8700_data *data = dev->driver_data;

	struct sensor_trigger drdy_trig = {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_ALL,
	};

	if (data->drdy_handler) {
		data->drdy_handler(dev, &drdy_trig);
	}

	return 0;
}

static void fxos8700_handle_int(void *arg)
{
	struct device *dev = (struct device *)arg;
	const struct fxos8700_config *config = dev->config->config_info;
	struct fxos8700_data *data = dev->driver_data;
	uint8_t int_source;

	k_sem_take(&data->sem, K_FOREVER);

	if (i2c_reg_read_byte(data->i2c, config->i2c_address,
			      FXOS8700_REG_INT_SOURCE,
			      &int_source)) {
		SYS_LOG_DBG("Could not read interrupt source");
		int_source = 0;
	}

	k_sem_give(&data->sem);

	if (int_source & FXOS8700_DRDY_MASK) {
		fxos8700_handle_drdy_int(dev);
	}

	gpio_pin_enable_callback(data->gpio, config->gpio_pin);
}

#ifdef CONFIG_FXOS8700_TRIGGER_OWN_THREAD
static void fxos8700_thread_main(void *arg1, void *unused1, void *unused2)
{
	struct device *dev = (struct device *)arg1;
	struct fxos8700_data *data = dev->driver_data;

	while (true) {
		k_sem_take(&data->trig_sem, K_FOREVER);
		fxos8700_handle_int(dev);
	}
}
#endif

#ifdef CONFIG_FXOS8700_TRIGGER_GLOBAL_THREAD
static void fxos8700_work_handler(struct k_work *work)
{
	struct fxos8700_data *data =
		CONTAINER_OF(work, struct fxos8700_data, work);

	fxos8700_handle_int(data->dev);
}
#endif

int fxos8700_trigger_set(struct device *dev,
			 const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler)
{
	const struct fxos8700_config *config = dev->config->config_info;
	struct fxos8700_data *data = dev->driver_data;
	enum fxos8700_power power = FXOS8700_POWER_STANDBY;
	uint8_t mask;
	int ret = 0;

	k_sem_take(&data->sem, K_FOREVER);

	switch (trig->type) {
	case SENSOR_TRIG_DATA_READY:
		mask = FXOS8700_DRDY_MASK;
		data->drdy_handler = handler;
		break;
	default:
		SYS_LOG_DBG("Unsupported sensor trigger");
		ret = -ENOTSUP;
		goto exit;
	}

	/* The sensor must be in standby mode when writing the configuration
	 * registers, therefore get the current power mode so we can restore it
	 * later.
	 */
	if (fxos8700_get_power(dev, &power)) {
		SYS_LOG_DBG("Could not get power mode");
		ret = -EIO;
		goto exit;
	}

	/* Put the sensor in standby mode */
	if (fxos8700_set_power(dev, FXOS8700_POWER_STANDBY)) {
		SYS_LOG_DBG("Could not set standby mode");
		ret = -EIO;
		goto exit;
	}

	/* Configure the sensor interrupt */
	if (i2c_reg_update_byte(data->i2c, config->i2c_address,
				FXOS8700_REG_CTRLREG4,
				mask,
				handler ? mask : 0)) {
		SYS_LOG_DBG("Could not configure interrupt");
		ret = -EIO;
		goto exit;
	}

	/* Restore the previous power mode */
	if (fxos8700_set_power(dev, power)) {
		SYS_LOG_DBG("Could not restore power mode");
		ret = -EIO;
		goto exit;
	}

exit:
	k_sem_give(&data->sem);

	return ret;
}

int fxos8700_trigger_init(struct device *dev)
{
	const struct fxos8700_config *config = dev->config->config_info;
	struct fxos8700_data *data = dev->driver_data;
	uint8_t ctrl_reg5;

#if defined(CONFIG_FXOS8700_TRIGGER_OWN_THREAD)
	k_sem_init(&data->trig_sem, 0, UINT_MAX);
	k_thread_spawn(data->thread_stack, CONFIG_FXOS8700_THREAD_STACK_SIZE,
		       fxos8700_thread_main, dev, 0, NULL,
		       K_PRIO_COOP(CONFIG_FXOS8700_THREAD_PRIORITY), 0, 0);
#elif defined(CONFIG_FXOS8700_TRIGGER_GLOBAL_THREAD)
	data->work.handler = fxos8700_work_handler;
	data->dev = dev;
#endif

	/* Route the interrupts to INT1/INT2 pins */
	ctrl_reg5 = 0;
#if CONFIG_FXOS8700_DRDY_INT1
	ctrl_reg5 |= FXOS8700_DRDY_MASK;
#endif
	if (i2c_reg_write_byte(data->i2c, config->i2c_address,
			       FXOS8700_REG_CTRLREG5, ctrl_reg5)) {
		SYS_LOG_DBG("Could not configure interrupt pin routing");
		return -EIO;
	}

	/* Get the GPIO device */
	data->gpio = device_get_binding(config->gpio_name);
	if (data->gpio == NULL) {
		SYS_LOG_DBG("Could not find GPIO device");
		return -EINVAL;
	}

	gpio_pin_configure(data->gpio, config->gpio_pin,
			   GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			   GPIO_INT_ACTIVE_LOW | GPIO_INT_DEBOUNCE);

	gpio_init_callback(&data->gpio_cb, fxos8700_gpio_callback,
			   BIT(config->gpio_pin));

	gpio_add_callback(data->gpio, &data->gpio_cb);

	gpio_pin_enable_callback(data->gpio, config->gpio_pin);

	return 0;
}
