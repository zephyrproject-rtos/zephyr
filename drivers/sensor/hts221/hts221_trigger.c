/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_hts221

#include <device.h>
#include <drivers/i2c.h>
#include <sys/__assert.h>
#include <sys/util.h>
#include <kernel.h>
#include <drivers/sensor.h>
#include <logging/log.h>
#include "hts221.h"

#if HTS221_TRIGGER_ENABLED
LOG_MODULE_DECLARE(HTS221, CONFIG_SENSOR_LOG_LEVEL);

static inline void setup_drdy(const struct device *dev,
			      bool enable)
{
	struct hts221_data *data = dev->data;
	const struct hts221_config *cfg = dev->config;
	unsigned int flags = enable
		? GPIO_INT_EDGE_TO_ACTIVE
		: GPIO_INT_DISABLE;

	gpio_pin_interrupt_configure(data->drdy_dev, cfg->drdy_pin, flags);
}

static inline void handle_drdy(const struct device *dev)
{
	struct hts221_data *data = dev->data;

	setup_drdy(dev, false);

#if defined(CONFIG_HTS221_TRIGGER_OWN_THREAD)
	k_sem_give(&data->drdy_sem);
#elif defined(CONFIG_HTS221_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
}

static void process_drdy(const struct device *dev)
{
	struct hts221_data *data = dev->data;

	if (data->data_ready_handler != NULL) {
		data->data_ready_handler(dev, &data->data_ready_trigger);
	}

	if (data->data_ready_handler != NULL) {
		setup_drdy(dev, true);
	}
}

int hts221_trigger_set(const struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler)
{
	struct hts221_data *data = dev->data;
	const struct hts221_config *cfg = dev->config;

	__ASSERT_NO_MSG(trig->type == SENSOR_TRIG_DATA_READY);

	setup_drdy(dev, false);

	data->data_ready_handler = handler;
	if (handler == NULL) {
		return 0;
	}

	data->data_ready_trigger = *trig;

	setup_drdy(dev, true);

	/* If DRDY is active we probably won't get the rising edge, so
	 * invoke the callback manually.
	 */
	if (gpio_pin_get(data->drdy_dev, cfg->drdy_pin) > 0) {
		handle_drdy(dev);
	}

	return 0;
}

static void hts221_drdy_callback(const struct device *dev,
				 struct gpio_callback *cb, uint32_t pins)
{
	struct hts221_data *data =
		CONTAINER_OF(cb, struct hts221_data, drdy_cb);

	ARG_UNUSED(pins);

	handle_drdy(data->dev);
}

#ifdef CONFIG_HTS221_TRIGGER_OWN_THREAD
static void hts221_thread(struct hts221_data *data)
{
	while (1) {
		k_sem_take(&data->drdy_sem, K_FOREVER);
		process_drdy(data->dev);
	}
}
#endif

#ifdef CONFIG_HTS221_TRIGGER_GLOBAL_THREAD
static void hts221_work_cb(struct k_work *work)
{
	struct hts221_data *data =
		CONTAINER_OF(work, struct hts221_data, work);

	process_drdy(data->dev);
}
#endif

int hts221_init_interrupt(const struct device *dev)
{
	struct hts221_data *data = dev->data;
	const struct hts221_config *cfg = dev->config;

	data->dev = dev;

	/* setup data ready gpio interrupt */
	data->drdy_dev = device_get_binding(cfg->drdy_controller);
	if (data->drdy_dev == NULL) {
		LOG_ERR("Cannot get pointer to %s device.",
			cfg->drdy_controller);
		return -EINVAL;
	}

	gpio_pin_configure(data->drdy_dev, cfg->drdy_pin,
			   GPIO_INPUT | cfg->drdy_flags);

	gpio_init_callback(&data->drdy_cb, hts221_drdy_callback,
			   BIT(cfg->drdy_pin));

	if (gpio_add_callback(data->drdy_dev, &data->drdy_cb) < 0) {
		LOG_ERR("Could not set gpio callback.");
		return -EIO;
	}

	/* enable data-ready interrupt */
	if (i2c_reg_write_byte(data->i2c, cfg->i2c_addr,
			       HTS221_REG_CTRL3, HTS221_DRDY_EN) < 0) {
		LOG_ERR("Could not enable data-ready interrupt.");
		return -EIO;
	}

#if defined(CONFIG_HTS221_TRIGGER_OWN_THREAD)
	k_sem_init(&data->drdy_sem, 0, UINT_MAX);

	k_thread_create(&data->thread, data->thread_stack,
			CONFIG_HTS221_THREAD_STACK_SIZE,
			(k_thread_entry_t)hts221_thread, data,
			NULL, NULL, K_PRIO_COOP(CONFIG_HTS221_THREAD_PRIORITY),
			0, K_NO_WAIT);
#elif defined(CONFIG_HTS221_TRIGGER_GLOBAL_THREAD)
	data->work.handler = hts221_work_cb;
#endif

	setup_drdy(dev, true);

	return 0;
}
#endif /* HTS221_TRIGGER_ENABLED */
