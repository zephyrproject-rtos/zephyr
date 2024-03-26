/*
 * Copyright (c) 2024 Gustavo Silva
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sciosense_ens160

#include <zephyr/kernel.h>

#include "ens160.h"

LOG_MODULE_DECLARE(ENS160, CONFIG_SENSOR_LOG_LEVEL);

static inline void ens160_setup_int(const struct device *dev, bool enable)
{
	const struct ens160_config *config = dev->config;
	gpio_flags_t flags = enable ? GPIO_INT_EDGE_TO_ACTIVE : GPIO_INT_DISABLE;

	(void)gpio_pin_interrupt_configure_dt(&config->int_gpio, flags);
}

static void ens160_gpio_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct ens160_data *data = CONTAINER_OF(cb, struct ens160_data, gpio_cb);

	ens160_setup_int(data->dev, false);

#if defined(CONFIG_ENS160_TRIGGER_OWN_THREAD)
	k_sem_give(&data->gpio_sem);
#elif defined(CONFIG_ENS160_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
}

static void ens160_thread_cb(const struct device *dev)
{
	struct ens160_data *data = dev->data;
	uint8_t status;
	int ret;

	ret = data->tf->read_reg(dev, ENS160_REG_DEVICE_STATUS, &status);
	if (ret < 0) {
		return;
	}

	if (FIELD_GET(ENS160_STATUS_NEWDAT, status) != 0x01) {
		LOG_ERR("Data is not ready");
		return;
	}

	if (data->data_ready_handler != NULL) {
		data->data_ready_handler(dev, data->data_ready_trigger);
	}

	ens160_setup_int(dev, true);
}

#ifdef CONFIG_ENS160_TRIGGER_OWN_THREAD
static void ens160_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct ens160_data *data = p1;

	while (1) {
		k_sem_take(&data->gpio_sem, K_FOREVER);
		ens160_thread_cb(data->dev);
	}
}
#endif /* CONFIG_ENS160_TRIGGER_OWN_THREAD */

#ifdef CONFIG_ENS160_TRIGGER_GLOBAL_THREAD
static void ens160_work_cb(struct k_work *work)
{
	struct ens160_data *data = CONTAINER_OF(work, struct ens160_data, work);

	ens160_thread_cb(data->dev);
}
#endif /* CONFIG_ENS160_TRIGGER_GLOBAL_THREAD */

int ens160_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler)
{
	const struct ens160_config *config = dev->config;
	struct ens160_data *data = dev->data;

	if (!config->int_gpio.port) {
		return -ENOTSUP;
	}

	ens160_setup_int(dev, false);

	if (trig->type == SENSOR_TRIG_DATA_READY) {
		data->data_ready_handler = handler;
		data->data_ready_trigger = trig;
	}

	ens160_setup_int(dev, true);

	return 0;
}

int ens160_init_interrupt(const struct device *dev)
{
	const struct ens160_config *config = dev->config;
	struct ens160_data *data = dev->data;
	int ret;

	uint8_t int_cfg = ENS160_CONFIG_INTEN | ENS160_CONFIG_INTDAT | ENS160_CONFIG_INT_CFG;

	ret = data->tf->write_reg(dev, ENS160_REG_CONFIG, int_cfg);
	if (ret < 0) {
		LOG_ERR("Failed to write to config register");
		return -EIO;
	}

	if (!gpio_is_ready_dt(&config->int_gpio)) {
		LOG_ERR("%s: device %s is not ready", dev->name, config->int_gpio.port->name);
		return -ENODEV;
	}

	gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);

	gpio_init_callback(&data->gpio_cb, ens160_gpio_callback, BIT(config->int_gpio.pin));

	ret = gpio_add_callback(config->int_gpio.port, &data->gpio_cb);
	if (ret < 0) {
		LOG_ERR("Failed to set gpio callback");
		return -EIO;
	}

	ret = gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		return ret;
	}

	data->dev = dev;

#if defined(CONFIG_ENS160_TRIGGER_OWN_THREAD)
	k_sem_init(&data->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&data->thread, data->thread_stack, CONFIG_ENS160_THREAD_STACK_SIZE,
			ens160_thread, data, NULL, NULL, K_PRIO_COOP(CONFIG_ENS160_THREAD_PRIORITY),
			0, K_NO_WAIT);
#elif defined(CONFIG_ENS160_TRIGGER_GLOBAL_THREAD)
	data->work.handler = ens160_work_cb;
#endif

	return 0;
}
