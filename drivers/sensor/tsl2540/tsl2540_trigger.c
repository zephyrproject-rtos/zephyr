/*
 * Copyright (c) 2022 T-Mobile USA, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "tsl2540.h"
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(tsl2540, CONFIG_SENSOR_LOG_LEVEL);

static void tsl2540_setup_int(const struct device *dev, bool enable)
{
	const struct tsl2540_config *config = dev->config;
	gpio_flags_t flags = enable
		? GPIO_INT_EDGE_TO_ACTIVE
		: GPIO_INT_DISABLE;

	gpio_pin_interrupt_configure_dt(&config->int_gpio, flags);
}

static void tsl2540_handle_int(const struct device *dev)
{
	struct tsl2540_data *drv_data = dev->data;

	tsl2540_setup_int(dev, false);

#if defined(CONFIG_TSL2540_TRIGGER_OWN_THREAD)
	k_sem_give(&drv_data->trig_sem);
#elif defined(CONFIG_TSL2540_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&drv_data->work);
#endif
}

static void tsl2540_gpio_callback(const struct device *dev, struct gpio_callback *cb,
				  uint32_t pin_mask)
{
	struct tsl2540_data *data = CONTAINER_OF(cb, struct tsl2540_data, gpio_cb);

	tsl2540_handle_int(data->dev);
}

static void tsl2540_process_int(const struct device *dev)
{
	const struct tsl2540_config *config = dev->config;
	struct tsl2540_data *data = dev->data;
	uint8_t status;

	/* Read the status, cleared automatically in CFG3 */
	int ret = i2c_reg_read_byte_dt(&config->i2c_spec, TSL2540_REG_STATUS, &status);

	if (ret) {
		LOG_ERR("Could not read status register (%#x), errno: %d", TSL2540_REG_STATUS,
			ret);
		return;
	}

	if (BIT(7) & status) { /* ASAT */
		LOG_ERR("Interrupt status(%#x): %#x: ASAT", TSL2540_REG_STATUS, status);
	}

	if (BIT(3) & status) { /* CINT */
		LOG_DBG("Interrupt status(%#x): %#x: CINT", TSL2540_REG_STATUS, status);
	}

	if (BIT(4) & status) { /* AINT */
		LOG_DBG("Interrupt status(%#x): %#x: AINT", TSL2540_REG_STATUS, status);
		if (data->als_handler != NULL) {
			data->als_handler(dev, data->als_trigger);
		}
	}

	tsl2540_setup_int(dev, true);

	/* Check for pin that may be asserted while we were busy */
	int pv = gpio_pin_get_dt(&config->int_gpio);

	if (pv > 0) {
		tsl2540_handle_int(dev);
	}
}

#ifdef CONFIG_TSL2540_TRIGGER_OWN_THREAD
static void tsl2540_thread_main(struct tsl2540_data *data)
{
	while (true) {
		k_sem_take(&data->trig_sem, K_FOREVER);
		tsl2540_process_int(data->dev);
	}
}
#endif

#ifdef CONFIG_TSL2540_TRIGGER_GLOBAL_THREAD
static void tsl2540_work_handler(struct k_work *work)
{
	struct tsl2540_data *data = CONTAINER_OF(work, struct tsl2540_data, work);

	tsl2540_process_int(data->dev);
}
#endif

int tsl2540_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
	const struct tsl2540_config *config = dev->config;
	struct tsl2540_data *data = dev->data;
	int ret;

	if (trig->type != SENSOR_TRIG_THRESHOLD) {
		LOG_ERR("Unsupported sensor trigger type: %d", trig->type);
		return -ENOTSUP;
	}

	if (trig->chan != SENSOR_CHAN_LIGHT) {
		LOG_ERR("Unsupported sensor trigger channel: %d", trig->chan);
		return -ENOTSUP;
	}


	const struct i2c_dt_spec *i2c_spec = &config->i2c_spec;

	ret = i2c_reg_update_byte_dt(i2c_spec, TSL2540_INTENAB_ADDR,
					TSL2540_INTENAB_MASK, TSL2540_INTENAB_CONF);
	if (ret) {
		LOG_ERR("%#x: I/O error: %d", TSL2540_INTENAB_ADDR, ret);
		return -EIO;
	}

	ret = i2c_reg_update_byte_dt(i2c_spec, TSL2540_CFG3_ADDR,
					TSL2540_CFG3_MASK, TSL2540_CFG3_CONF);
	if (ret) {
		LOG_ERR("%#x: I/O error: %d", TSL2540_CFG3_ADDR, ret);
		return -EIO;
	}

	k_sem_take(&data->sem, K_FOREVER);

	data->als_handler = handler;
	data->als_trigger = trig;

	if (handler != NULL) {
		tsl2540_setup_int(dev, true);

		/* Check whether already asserted */
		int pv = gpio_pin_get_dt(&config->int_gpio);

		if (pv > 0) {
			tsl2540_handle_int(dev);
		}
	}

	k_sem_give(&data->sem);

	return ret;
}

int tsl2540_trigger_init(const struct device *dev)
{
	const struct tsl2540_config *config = dev->config;
	struct tsl2540_data *data = dev->data;
	int rc;

	/* Check device is defined */
	if (config->int_gpio.port == NULL) {
		LOG_ERR("int-gpios is not defined in the device tree.");
		return -EINVAL;
	}

	/* Get the GPIO device */
	if (!gpio_is_ready_dt(&config->int_gpio)) {
		LOG_ERR("%s: gpio controller %s not ready", dev->name, config->int_gpio.port->name);
		return -ENODEV;
	}

	rc = gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);
	if (rc < 0) {
		return rc;
	}

	gpio_init_callback(&data->gpio_cb, tsl2540_gpio_callback, BIT(config->int_gpio.pin));

	if (gpio_add_callback(config->int_gpio.port, &data->gpio_cb) < 0) {
		LOG_ERR("Failed to set gpio callback!");
		return -EIO;
	}

	data->dev = dev;

#if defined(CONFIG_TSL2540_TRIGGER_OWN_THREAD)
	k_sem_init(&data->trig_sem, 0, K_SEM_MAX_LIMIT);
	k_thread_create(&data->thread, data->thread_stack, CONFIG_TSL2540_THREAD_STACK_SIZE,
			(k_thread_entry_t)tsl2540_thread_main, data, NULL, NULL,
			K_PRIO_COOP(CONFIG_TSL2540_THREAD_PRIORITY), 0, K_NO_WAIT);
	k_thread_name_set(&data->thread, "TSL2540 trigger");
#elif defined(CONFIG_TSL2540_TRIGGER_GLOBAL_THREAD)
	data->work.handler = tsl2540_work_handler;
#endif

	return 0;
}
