/*
 * Copyright (c) 2018 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_adxl345

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>

#include "adxl345.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(ADXL345, CONFIG_SENSOR_LOG_LEVEL);

static int adxl345_set_int_pad_state(const struct device *dev, uint8_t pad,
				     bool en)
{
	const struct adxl345_dev_config *cfg = dev->config;
	int state = en ? GPIO_INT_EDGE_TO_ACTIVE : GPIO_INT_DISABLE;

	/* in case of neither INT_1 nor INT_2 being defined */
	if (!cfg->gpio_int1.port && !cfg->gpio_int2.port) {
		return -ENOTSUP;
	}

	if (pad == 1) {
		return gpio_pin_interrupt_configure_dt(&cfg->gpio_int1, state);
	} else if (pad == 2) {
		return gpio_pin_interrupt_configure_dt(&cfg->gpio_int2, state);
	}

	/* pad may be -1, e.g. if no INT line defined in DT */
	return -EINVAL;
}

int adxl345_set_gpios_en(const struct device *dev, bool en)
{
	const struct adxl345_dev_config *cfg = dev->config;

	return adxl345_set_int_pad_state(dev, cfg->drdy_pad, en);
}

#if defined(CONFIG_ADXL345_TRIGGER_OWN_THREAD) || \
	defined(CONFIG_ADXL345_TRIGGER_GLOBAL_THREAD)

/** adxl345_handle_interrupt - Interrupt service routine for the sensor.
 * @dev: The device instance.
 * Handle and reset the sensor interrupt events.
 */
static void adxl345_handle_interrupt(const struct device *dev)
{
	struct adxl345_dev_data *data = dev->data;
	uint8_t status;
	int rc;

	rc = adxl345_get_status(dev, &status);
	__ASSERT(rc == 0, "Interrupt configuration failed");

	if (FIELD_GET(ADXL345_INT_DATA_RDY, status)) {
		if (data->drdy_handler) {
			/*
			 * A handler needs to flush FIFO, i.e. fetch and get
			 * samples to get new events
			 */
			data->drdy_handler(dev, data->drdy_trigger);
		}
	}
}
#endif

static void adxl345_int1_gpio_callback(const struct device *dev,
				       struct gpio_callback *cb,
				       uint32_t pins)
{
	struct adxl345_dev_data *data =
		CONTAINER_OF(cb, struct adxl345_dev_data, int1_cb);

	ARG_UNUSED(pins);

	adxl345_set_int_pad_state(dev, 1, false);

#if defined(CONFIG_ADXL345_TRIGGER_OWN_THREAD)
	k_sem_give(&data->gpio_sem);
#elif defined(CONFIG_ADXL345_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
}

static void adxl345_int2_gpio_callback(const struct device *dev,
				       struct gpio_callback *cb, uint32_t pins)
{
	struct adxl345_dev_data *data =
		CONTAINER_OF(cb, struct adxl345_dev_data, int2_cb);

	ARG_UNUSED(pins);

	adxl345_set_int_pad_state(dev, 2, false);

#if defined(CONFIG_ADXL345_TRIGGER_OWN_THREAD)
	k_sem_give(&data->gpio_sem);
#elif defined(CONFIG_ADXL345_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
}

#if defined(CONFIG_ADXL345_TRIGGER_OWN_THREAD)
static void adxl345_thread(struct adxl345_dev_data *data)
{
	while (true) {
		k_sem_take(&data->gpio_sem, K_FOREVER);
		adxl345_handle_interrupt(data->dev);
	}
}

#elif defined(CONFIG_ADXL345_TRIGGER_GLOBAL_THREAD)
static void adxl345_work_cb(struct k_work *work)
{
	struct adxl345_dev_data *data =
		CONTAINER_OF(work, struct adxl345_dev_data, work);

#if !defined CONFIG_ADXL345_STREAM
	/*
	 * Make sure, STREAM ISR w/ RTIO is handling the interrupt, and not
	 * cleaned up afterwards by the TRIGGER handler, if STREAM is enabled.
	 * So, disable TRIGGER ISR if STREAM is defined.
	 */
	adxl345_handle_interrupt(data->dev);
#endif /* !defined CONFIG_ADXL345_STREAM */

}
#endif

/**
 * adxl345_trigger_set - Register a handler for a sensor trigger from the app.
 * @dev: The device instance.
 * @trig: The interrupt event type of the sensor.
 * @handler: A handler for the sensor event to be registered.
 * Map sensor events to the interrupt lines.
 * return: 0 for success, or error.
 */
int adxl345_trigger_set(const struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
	const struct adxl345_dev_config *cfg = dev->config;
	struct adxl345_dev_data *data = dev->data;
	uint8_t status1, int_mask, int_en = 0;
	int rc;

	if (!cfg->gpio_int1.port && !cfg->gpio_int2.port) {
		/* might be in FIFO BYPASS mode */
		goto done;
	}

	/* generally turn off interrupts */
	rc = adxl345_set_gpios_en(dev, false);
	if (rc) {
		return rc;
	}

	switch (trig->type) {
	case SENSOR_TRIG_DATA_READY:
		data->drdy_handler = handler;
		data->drdy_trigger = trig;
		int_mask = ADXL345_INT_DATA_RDY;
		break;
	default:
		LOG_ERR("Unsupported sensor trigger");
		return -ENOTSUP;
	}

done:
	rc = adxl345_set_gpios_en(dev, true);
	if (rc) {
		return rc;
	}

	rc = adxl345_reg_update_bits(dev, ADXL345_REG_INT_MAP,
				      int_mask, int_en);
	if (rc < 0) {
		return rc;
	}
	/* Clear status */
	rc = adxl345_get_status(dev, &status1);
	if (rc < 0) {
		return rc;
	}

	return 0;
}

int adxl345_init_interrupt(const struct device *dev)
{
	const struct adxl345_dev_config *cfg = dev->config;
	struct adxl345_dev_data *data = dev->data;
	int rc;

	/* TRIGGER is set, but not INT line was defined in DT */
	if (!cfg->gpio_int1.port && !cfg->gpio_int2.port) {
		return -ENOTSUP;
	}

	if (cfg->gpio_int1.port) {
		if (!gpio_is_ready_dt(&cfg->gpio_int1)) {
			LOG_ERR("INT_1 line defined, but not ready");
			return -ENODEV;
		}
	}

	if (cfg->gpio_int2.port) {
		if (!gpio_is_ready_dt(&cfg->gpio_int2)) {
			LOG_ERR("INT_2 line defined, but not ready");
			return -ENODEV;
		}
	}

#if defined(CONFIG_ADXL345_TRIGGER_OWN_THREAD)
	k_sem_init(&data->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&data->thread, data->thread_stack,
			CONFIG_ADXL345_THREAD_STACK_SIZE,
			adxl345_thread, data,
			NULL, NULL, K_PRIO_COOP(CONFIG_ADXL345_THREAD_PRIORITY),
			0, K_NO_WAIT);
#elif defined(CONFIG_ADXL345_TRIGGER_GLOBAL_THREAD)
	data->work.handler = adxl345_work_cb;
#endif

	if (cfg->gpio_int1.port) {
		rc = gpio_pin_configure_dt(&cfg->gpio_int1, GPIO_INPUT);
		if (rc < 0) {
			return rc;
		}

		gpio_init_callback(&data->int1_cb,
				   adxl345_int1_gpio_callback,
				   BIT(cfg->gpio_int1.pin));

		rc = gpio_add_callback(cfg->gpio_int1.port,
					&data->int1_cb);
		if (rc < 0) {
			LOG_ERR("Failed to set INT_1 gpio callback!");
			return -EIO;
		}
	}

	if (cfg->gpio_int2.port) {
		rc = gpio_pin_configure_dt(&cfg->gpio_int2, GPIO_INPUT);
		if (rc < 0) {
			return rc;
		}

		gpio_init_callback(&data->int2_cb,
				   adxl345_int2_gpio_callback,
				   BIT(cfg->gpio_int2.pin));

		rc = gpio_add_callback(cfg->gpio_int2.port,
					&data->int2_cb);
		if (rc < 0) {
			LOG_ERR("Failed to set INT_2 gpio callback!");
			return -EIO;
		}
	}

	data->dev = dev;

	return 0;
}
