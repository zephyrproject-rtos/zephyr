/*
 * Copyright (c) 2023 Kurtis Dinelle
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "tsl2591.h"

LOG_MODULE_DECLARE(TSL2591, CONFIG_SENSOR_LOG_LEVEL);

static inline void tsl2591_setup_int(const struct device *dev, bool enable)
{
	const struct tsl2591_config *config = dev->config;
	gpio_flags_t flags = enable ? GPIO_INT_EDGE_TO_ACTIVE : GPIO_INT_DISABLE;

	gpio_pin_interrupt_configure_dt(&config->int_gpio, flags);
}

static void tsl2591_gpio_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pins);

	struct tsl2591_data *data = CONTAINER_OF(cb, struct tsl2591_data, gpio_cb);

	tsl2591_setup_int(data->dev, false);

#if defined(CONFIG_TSL2591_TRIGGER_OWN_THREAD)
	k_sem_give(&data->trig_sem);
#elif defined(CONFIG_TSL2591_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
}

static void tsl2591_handle_int(const struct device *dev)
{
	struct tsl2591_data *data = dev->data;
	const struct tsl2591_config *config = dev->config;
	uint8_t clear_cmd;
	int ret;

	/* Interrupt must be cleared manually */
	clear_cmd = TSL2591_CLEAR_INT_CMD;
	ret = i2c_write_dt(&config->i2c, &clear_cmd, 1U);
	if (ret < 0) {
		LOG_ERR("Failed to clear interrupt");
		return;
	}

	if (data->th_handler != NULL) {
		data->th_handler(dev, data->th_trigger);
	}

	tsl2591_setup_int(dev, true);
}

#ifdef CONFIG_TSL2591_TRIGGER_OWN_THREAD
static void tsl2591_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct tsl2591_data *data = p1;

	while (1) {
		k_sem_take(&data->trig_sem, K_FOREVER);
		tsl2591_handle_int(data->dev);
	}
}
#endif

#ifdef CONFIG_TSL2591_TRIGGER_GLOBAL_THREAD
static void tsl2591_work_handler(struct k_work *work)
{
	struct tsl2591_data *data = CONTAINER_OF(work, struct tsl2591_data, work);

	tsl2591_handle_int(data->dev);
}
#endif

int tsl2591_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
	struct tsl2591_data *data = dev->data;
	const struct tsl2591_config *config = dev->config;
	int ret;

	if (!config->int_gpio.port) {
		return -ENOTSUP;
	}

	if (trig->chan != SENSOR_CHAN_LIGHT) {
		LOG_ERR("Unsupported sensor trigger channel");
		return -ENOTSUP;
	}

	if (trig->type != SENSOR_TRIG_THRESHOLD) {
		LOG_ERR("Unsupported sensor trigger type");
		return -ENOTSUP;
	}

	data->th_handler = handler;
	data->th_trigger = trig;
	tsl2591_setup_int(dev, true);

	ret = tsl2591_reg_update(dev, TSL2591_REG_ENABLE, TSL2591_AIEN_MASK, TSL2591_AIEN_ON);
	if (ret < 0) {
		LOG_ERR("Failed to enable interrupt on sensor");
	}

	return ret;
}

int tsl2591_initialize_int(const struct device *dev)
{
	struct tsl2591_data *data = dev->data;
	const struct tsl2591_config *config = dev->config;
	int ret;

	if (!gpio_is_ready_dt(&config->int_gpio)) {
		LOG_ERR("%s: gpio controller %s not ready", dev->name, config->int_gpio.port->name);
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT | config->int_gpio.dt_flags);
	if (ret < 0) {
		LOG_ERR("Failed to configure gpio pin for input");
		return ret;
	}

	gpio_init_callback(&data->gpio_cb, tsl2591_gpio_callback, BIT(config->int_gpio.pin));

	ret = gpio_add_callback(config->int_gpio.port, &data->gpio_cb);
	if (ret < 0) {
		LOG_DBG("Failed to set gpio callback");
		return ret;
	}

	data->dev = dev;

#if defined(CONFIG_TSL2591_TRIGGER_OWN_THREAD)
	ret = k_sem_init(&data->trig_sem, 0, K_SEM_MAX_LIMIT);
	if (ret < 0) {
		LOG_ERR("Failed to initialize trigger semaphore");
		return ret;
	}

	k_thread_create(&data->thread, data->thread_stack, CONFIG_TSL2591_THREAD_STACK_SIZE,
			tsl2591_thread, data, NULL, NULL,
			K_PRIO_COOP(CONFIG_TSL2591_THREAD_PRIORITY), 0, K_NO_WAIT);
#elif defined(CONFIG_TSL2591_TRIGGER_GLOBAL_THREAD)
	data->work.handler = tsl2591_work_handler;
#endif

	return 0;
}
