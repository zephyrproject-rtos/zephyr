/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>

#include "fxas21002.h"

LOG_MODULE_DECLARE(FXAS21002, CONFIG_SENSOR_LOG_LEVEL);

static void fxas21002_gpio_callback(struct device *dev,
				   struct gpio_callback *cb,
				   u32_t pin_mask)
{
	struct fxas21002_data *data =
		CONTAINER_OF(cb, struct fxas21002_data, gpio_cb);

	if ((pin_mask & BIT(data->gpio_pin)) == 0U) {
		return;
	}

	gpio_pin_disable_callback(dev, data->gpio_pin);

#if defined(CONFIG_FXAS21002_TRIGGER_OWN_THREAD)
	k_sem_give(&data->trig_sem);
#elif defined(CONFIG_FXAS21002_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
}

static int fxas21002_handle_drdy_int(struct device *dev)
{
	struct fxas21002_data *data = dev->driver_data;

	struct sensor_trigger drdy_trig = {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_ALL,
	};

	if (data->drdy_handler) {
		data->drdy_handler(dev, &drdy_trig);
	}

	return 0;
}

static void fxas21002_handle_int(void *arg)
{
	struct device *dev = (struct device *)arg;
	const struct fxas21002_config *config = dev->config->config_info;
	struct fxas21002_data *data = dev->driver_data;
	u8_t int_source;

	k_sem_take(&data->sem, K_FOREVER);

	if (i2c_reg_read_byte(data->i2c, config->i2c_address,
			      FXAS21002_REG_INT_SOURCE,
			      &int_source)) {
		LOG_ERR("Could not read interrupt source");
		int_source = 0U;
	}

	k_sem_give(&data->sem);

	if (int_source & FXAS21002_INT_SOURCE_DRDY_MASK) {
		fxas21002_handle_drdy_int(dev);
	}

	gpio_pin_enable_callback(data->gpio, config->gpio_pin);
}

#ifdef CONFIG_FXAS21002_TRIGGER_OWN_THREAD
static void fxas21002_thread_main(void *arg1, void *unused1, void *unused2)
{
	struct device *dev = (struct device *)arg1;
	struct fxas21002_data *data = dev->driver_data;

	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);

	while (true) {
		k_sem_take(&data->trig_sem, K_FOREVER);
		fxas21002_handle_int(dev);
	}
}
#endif

#ifdef CONFIG_FXAS21002_TRIGGER_GLOBAL_THREAD
static void fxas21002_work_handler(struct k_work *work)
{
	struct fxas21002_data *data =
		CONTAINER_OF(work, struct fxas21002_data, work);

	fxas21002_handle_int(data->dev);
}
#endif

int fxas21002_trigger_set(struct device *dev,
			 const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler)
{
	const struct fxas21002_config *config = dev->config->config_info;
	struct fxas21002_data *data = dev->driver_data;
	enum fxas21002_power power = FXAS21002_POWER_STANDBY;
	u32_t transition_time;
	u8_t mask;
	int ret = 0;

	k_sem_take(&data->sem, K_FOREVER);

	switch (trig->type) {
	case SENSOR_TRIG_DATA_READY:
		mask = FXAS21002_CTRLREG2_CFG_EN_MASK;
		data->drdy_handler = handler;
		break;
	default:
		LOG_ERR("Unsupported sensor trigger");
		ret = -ENOTSUP;
		goto exit;
	}

	/* The sensor must be in standby or ready mode when writing the
	 * configuration registers, therefore get the current power mode so we
	 * can restore it later.
	 */
	if (fxas21002_get_power(dev, &power)) {
		LOG_ERR("Could not get power mode");
		ret = -EIO;
		goto exit;
	}

	/* Put the sensor in ready mode */
	if (fxas21002_set_power(dev, FXAS21002_POWER_READY)) {
		LOG_ERR("Could not set ready mode");
		ret = -EIO;
		goto exit;
	}

	/* Configure the sensor interrupt */
	if (i2c_reg_update_byte(data->i2c, config->i2c_address,
				FXAS21002_REG_CTRLREG2,
				mask,
				handler ? mask : 0)) {
		LOG_ERR("Could not configure interrupt");
		ret = -EIO;
		goto exit;
	}

	/* Restore the previous power mode */
	if (fxas21002_set_power(dev, power)) {
		LOG_ERR("Could not restore power mode");
		ret = -EIO;
		goto exit;
	}

	/* Wait the transition time from ready mode */
	transition_time = fxas21002_get_transition_time(FXAS21002_POWER_READY,
							power,
							config->dr);
	k_busy_wait(transition_time);

exit:
	k_sem_give(&data->sem);

	return ret;
}

int fxas21002_trigger_init(struct device *dev)
{
	const struct fxas21002_config *config = dev->config->config_info;
	struct fxas21002_data *data = dev->driver_data;
	u8_t ctrl_reg2;

#if defined(CONFIG_FXAS21002_TRIGGER_OWN_THREAD)
	k_sem_init(&data->trig_sem, 0, UINT_MAX);
	k_thread_create(&data->thread, data->thread_stack,
			CONFIG_FXAS21002_THREAD_STACK_SIZE,
			fxas21002_thread_main, dev, 0, NULL,
			K_PRIO_COOP(CONFIG_FXAS21002_THREAD_PRIORITY), 0, K_NO_WAIT);
#elif defined(CONFIG_FXAS21002_TRIGGER_GLOBAL_THREAD)
	data->work.handler = fxas21002_work_handler;
	data->dev = dev;
#endif

	/* Route the interrupts to INT1/INT2 pins */
	ctrl_reg2 = 0U;
#if CONFIG_FXAS21002_DRDY_INT1
	ctrl_reg2 |= FXAS21002_CTRLREG2_CFG_DRDY_MASK;
#endif

	if (i2c_reg_write_byte(data->i2c, config->i2c_address,
			       FXAS21002_REG_CTRLREG2, ctrl_reg2)) {
		LOG_ERR("Could not configure interrupt pin routing");
		return -EIO;
	}

	/* Get the GPIO device */
	data->gpio = device_get_binding(config->gpio_name);
	if (data->gpio == NULL) {
		LOG_ERR("Could not find GPIO device");
		return -EINVAL;
	}

	data->gpio_pin = config->gpio_pin;

	gpio_pin_configure(data->gpio, config->gpio_pin,
			   GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			   GPIO_INT_ACTIVE_LOW | GPIO_INT_DEBOUNCE);

	gpio_init_callback(&data->gpio_cb, fxas21002_gpio_callback,
			   BIT(config->gpio_pin));

	gpio_add_callback(data->gpio, &data->gpio_cb);

	gpio_pin_enable_callback(data->gpio, config->gpio_pin);

	return 0;
}
