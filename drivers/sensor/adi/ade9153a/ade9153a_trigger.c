/*
 * Copyright (c) 2024 Plentify (Pty) Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_ade9153a

#include <zephyr/drivers/sensor.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/__assert.h>

#include "ade9153a.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(ade9153a, CONFIG_SENSOR_LOG_LEVEL);

BUILD_ASSERT(
	CONFIG_ADE9153A_TRIGGER_IRQ || CONFIG_ADE9153A_TRIGGER_CF,
	"At least one of them must be enabled if the trigger function is enabled for this driver.");

static inline void interrupt_set_enable(const struct ade9153a_config *cfg, bool enable)
{
#if defined(CONFIG_ADE9153A_TRIGGER_IRQ)
	gpio_pin_interrupt_configure_dt(&cfg->irq_gpio_dt_spec,
					enable ? GPIO_INT_EDGE_TO_ACTIVE : GPIO_INT_DISABLE);
#endif
#if defined(CONFIG_ADE9153A_TRIGGER_CF)
	gpio_pin_interrupt_configure_dt(&cfg->cf_gpio_dt_spec,
					enable ? GPIO_INT_EDGE_TO_ACTIVE : GPIO_INT_DISABLE);
#endif
}

static void gpio_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct ade9153a_data *data = CONTAINER_OF(cb, struct ade9153a_data, gpio_cb);
	const struct ade9153a_config *config = data->dev->config;

	interrupt_set_enable(config, false);

	k_msgq_put(&data->trigger_pins_msgq, &pins, K_NO_WAIT);
}

static void thread_cb(const struct device *dev, uint32_t pins)
{
	struct ade9153a_data *data = dev->data;
	const struct ade9153a_config *config = dev->config;

#if defined(CONFIG_ADE9153A_TRIGGER_IRQ)
	if ((pins & BIT(config->irq_gpio_dt_spec.pin)) && (data->irq_handler != NULL)) {
		LOG_DBG("IRQ trigger happened");
		data->irq_handler(dev, data->irq_trigger);
	}
#endif

#if defined(CONFIG_ADE9153A_TRIGGER_CF)
	if ((pins & BIT(config->cf_gpio_dt_spec.pin)) && (data->cf_handler != NULL)) {
		LOG_DBG("CF trigger happened");
		data->cf_handler(dev, data->cf_trigger);
	}

#endif
	interrupt_set_enable(config, true);
}

static void thread(void *dev_data, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	__ASSERT_NO_MSG(dev_data != NULL);

	struct ade9153a_data *data = dev_data;
	int32_t trigger_pins;

	while (1) {
		k_msgq_get(&data->trigger_pins_msgq, &trigger_pins, K_FOREVER);
		thread_cb(data->dev, trigger_pins);
	}
}

int ade9153a_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler)
{
	__ASSERT_NO_MSG(trig->chan == SENSOR_CHAN_ALL);

	struct ade9153a_data *data = dev->data;
	const struct ade9153a_config *config = dev->config;
	enum sensor_trigger_ade9153a ade_trig_type = (enum sensor_trigger_ade9153a)trig->type;

#if defined(CONFIG_ADE9153A_TRIGGER_IRQ)
	if (config->irq_gpio_dt_spec.port == NULL) {
		return -ENOTSUP;
	}
#endif

#if defined(CONFIG_ADE9153A_TRIGGER_CF)
	if (config->cf_gpio_dt_spec.port == NULL) {
		return -ENOTSUP;
	}
#endif

	interrupt_set_enable(config, false);

	switch (ade_trig_type) {
	case SENSOR_TRIG_ADE9153A_IRQ:
		data->irq_handler = handler;
		data->irq_trigger = trig;
		LOG_DBG("IRQ trigger set");
		break;
	case SENSOR_TRIG_ADE9153A_CF:
		data->cf_handler = handler;
		data->cf_trigger = trig;
		LOG_DBG("CF trigger set");
		break;
	default:
		LOG_ERR("Unsupported sensor trigger");
		return -EINVAL;
	}

	interrupt_set_enable(config, true);

	return 0;
}

int ade9153a_init_interrupt(const struct device *dev)
{
	struct ade9153a_data *data = dev->data;
	const struct ade9153a_config *config = dev->config;

#if defined(CONFIG_ADE9153A_TRIGGER_IRQ)
	if (!gpio_is_ready_dt(&config->irq_gpio_dt_spec)) {
		LOG_ERR("%s: device %s is not ready", dev->name,
			config->irq_gpio_dt_spec.port->name);
		return -ENODEV;
	}

	gpio_pin_configure_dt(&config->irq_gpio_dt_spec,
			      GPIO_INPUT | config->irq_gpio_dt_spec.dt_flags);

	gpio_init_callback(&data->gpio_cb, gpio_callback, BIT(config->irq_gpio_dt_spec.pin));

	if (gpio_add_callback(config->irq_gpio_dt_spec.port, &data->gpio_cb) < 0) {
		LOG_DBG("Failed to set IRQ gpio callback!");
		return -EIO;
	}
	LOG_DBG("IRQ trigger initialized");
#endif

#if defined(CONFIG_ADE9153A_TRIGGER_CF)
	if (!gpio_is_ready_dt(&config->cf_gpio_dt_spec)) {
		LOG_ERR("%s: device %s is not ready", dev->name,
			config->cf_gpio_dt_spec.port->name);
		return -ENODEV;
	}

	gpio_pin_configure_dt(&config->cf_gpio_dt_spec,
			      GPIO_INPUT | config->cf_gpio_dt_spec.dt_flags);

	gpio_init_callback(&data->gpio_cb, gpio_callback, BIT(config->cf_gpio_dt_spec.pin));

	if (gpio_add_callback(config->cf_gpio_dt_spec.port, &data->gpio_cb) < 0) {
		LOG_DBG("Failed to set CF gpio callback!");
		return -EIO;
	}
	LOG_DBG("CF trigger initialized");
#endif

	data->dev = dev;

	k_msgq_init(&data->trigger_pins_msgq, data->trigger_msgq_buffer, sizeof(uint32_t), 10);

	k_thread_create(&data->thread, data->thread_stack, CONFIG_ADE9153A_THREAD_STACK_SIZE,
			thread, data, NULL, NULL, K_PRIO_COOP(CONFIG_ADE9153A_THREAD_PRIORITY), 0,
			K_NO_WAIT);

	interrupt_set_enable(config, true);

	return 0;
}
