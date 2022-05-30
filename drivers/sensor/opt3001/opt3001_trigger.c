/*
 * Copyright (c) 2022 Intellinium <giuliano.franchetto@intellinium.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/byteorder.h>
#include "opt3001.h"

#define DT_DRV_COMPAT ti_opt3001

static void opt3001_gpio_callback(const struct device *dev,
				  struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pins);

	struct opt3001_data *data =
		CONTAINER_OF(cb, struct opt3001_data, gpio_cb);
	const struct opt3001_config *cfg = data->dev->config;

	gpio_pin_interrupt_configure_dt(&cfg->irq_spec, GPIO_INT_DISABLE);

#if defined(CONFIG_OPT3001_TRIGGER_OWN_THREAD)
	k_sem_give(&data->gpio_sem);
#elif defined(CONFIG_OPT3001_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif /* CONFIG_OPT3001_TRIGGER_OWN_THREAD */
}

static void opt3001_handle_interrupt(const struct device *dev)
{
	int err;
	uint16_t config;
	struct opt3001_data *data = dev->data;
	const struct opt3001_config *cfg = dev->config;
	struct sensor_trigger limit_trig = {
		.type = SENSOR_TRIG_THRESHOLD,
		.chan = SENSOR_CHAN_ALL,
	};

	/* Reading the configuration field to clear irq */
	err = i2c_burst_read_dt(&cfg->i2c, 0x01, (uint8_t *) &config, 2);
	if (err) {
		goto rearm;
	}

	if (!data->limit_handler) {
		goto rearm;
	}

	data->limit_handler(dev, &limit_trig);

rearm:
	gpio_pin_interrupt_configure_dt(&cfg->irq_spec,
					GPIO_INT_EDGE_TO_ACTIVE);
}

#if CONFIG_OPT3001_TRIGGER_OWN_THREAD
static void opt3001_thread(struct opt3001_data *data)
{
	while (1) {
		k_sem_take(&data->gpio_sem, K_FOREVER);
		opt3001_handle_interrupt(data->dev);
	}
}
#endif /* CONFIG_OPT3001_TRIGGER_OWN_THREAD */

#ifdef CONFIG_OPT3001_TRIGGER_GLOBAL_THREAD
static void opt3001_work_cb(struct k_work *work)
{
	struct opt3001_data *data =
		CONTAINER_OF(work, struct opt3001_data, work);

	opt3001_handle_interrupt(data->dev);
}
#endif /* CONFIG_OPT3001_TRIGGER_GLOBAL_THREAD */

int opt3001_set_th(const struct device *dev,
		   uint16_t value,
		   uint8_t addr)
{
	uint8_t le;
	uint16_t tl;
	uint32_t v;
	struct opt3001_data *data = dev->data;
	const struct opt3001_config *cfg = dev->config;

	/* The values used here are the full-scale range limits for each
	 * low-limit register exponent.
	 * More information can be found using Table 12 of the opt3001
	 * datasheet
	 */
	if (value <= 40.95) {
		le = 0;
	} else if (40.95 < value && value <= 81.90) {
		le = 1;
	} else if (81.90 < value && value <= 163.80) {
		le = 2;
	} else if (163.80 < value && value <= 327.60) {
		le = 3;
	} else if (327.60 < value && value <= 655.20) {
		le = 4;
	} else if (655.20 < value && value <= 1310.40) {
		le = 5;
	} else if (1310.40 < value && value <= 2620.80) {
		le = 6;
	} else if (2620.80 < value && value <= 5241.60) {
		le = 7;
	} else if (5241.60 < value && value <= 10483.20) {
		le = 8;
	} else if (10483.20 < value && value <= 20966.40) {
		le = 9;
	} else if (20966.40 < value && value <= 41932.80) {
		le = 10;
	} else {
		le = 11;
	}

	v = ((uint32_t) (value * 100.0)) >> le;
	tl = (v & 0x0FFF) | (le << 12);
	tl = sys_be16_to_cpu(tl);

	uint8_t buf[3] = {
		addr,
		tl & 0xff,
		tl >> 8
	};

	return i2c_write_dt(&cfg->i2c, buf, 3);
}

static int opt3001_set_higher_th(const struct device *dev, uint16_t value)
{
	return opt3001_set_th(dev, value, 0x03);
}

static int opt3001_set_lower_th(const struct device *dev, uint16_t value)
{
	return opt3001_set_th(dev, value, 0x02);
}

static int opt3001_enable_int(const struct device *dev, bool enabled)
{
	int err;
	const struct opt3001_config *config = dev->config;

	if (enabled) {
		gpio_pin_interrupt_configure_dt(&config->irq_spec,
						GPIO_INT_EDGE_TO_ACTIVE);
		return 0;
	}

	gpio_pin_interrupt_configure_dt(&config->irq_spec,
					GPIO_INT_DISABLE);

	err = opt3001_set_lower_th(dev, 0x0000);
	if (err) {
		return err;
	}

	return opt3001_set_higher_th(dev, 0xBFFF);
}

int opt3001_trigger_set(const struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
	struct opt3001_data *data = dev->data;
	bool enabled = handler != NULL;

	switch (trig->type) {
	case SENSOR_TRIG_THRESHOLD:
		data->limit_handler = handler;
		return opt3001_enable_int(dev, enabled);
	default:
		return -EINVAL;
	}
}

int opt3001_trigger_init(const struct device *dev)
{
	int err;
	struct opt3001_data *data = dev->data;
	const struct opt3001_config *cfg = dev->config;

	if (!device_is_ready(cfg->irq_spec.port)) {
		return -ENOSYS;
	}

	data->dev = dev;

	err = gpio_pin_configure_dt(&cfg->irq_spec, GPIO_INPUT);
	if (err) {
		return err;
	}

	gpio_init_callback(&data->gpio_cb,
			   opt3001_gpio_callback,
			   BIT(cfg->irq_spec.pin));

	err = gpio_add_callback(cfg->irq_spec.port, &data->gpio_cb);
	if (err) {
		return err;
	}

#if defined(CONFIG_OPT3001_TRIGGER_OWN_THREAD)
	k_sem_init(&data->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&data->thread, data->thread_stack,
			CONFIG_OPT3001_THREAD_STACK_SIZE,
			(k_thread_entry_t) opt3001_thread, data,
			NULL, NULL, K_PRIO_COOP(CONFIG_OPT3001_THREAD_PRIORITY),
			0, K_NO_WAIT);
#elif defined(CONFIG_OPT3001_TRIGGER_GLOBAL_THREAD)
	data->work.handler = opt3001_work_cb;
#endif /* CONFIG_OPT3001_TRIGGER_GLOBAL_THREAD */

	return 0;
}
