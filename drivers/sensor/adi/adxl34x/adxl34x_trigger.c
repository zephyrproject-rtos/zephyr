/*
 * Copyright (c) 2024 Chaim Zax
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/adxl34x.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/sys/util.h>

#include "adxl34x_private.h"
#include "adxl34x_reg.h"
#include "adxl34x_trigger.h"
#include "adxl34x_rtio.h"

LOG_MODULE_DECLARE(adxl34x, CONFIG_SENSOR_LOG_LEVEL);

#define GPIO_INT_TRIGGER GPIO_INT_EDGE_TO_ACTIVE

/**
 * Callback handler when an interrupt was detected
 *
 * @param[in] dev Pointer to the sensor device
 * @param[in,out] cb Original GPIO callback structure owning this handler
 * @param[in] pins Mask of pins that triggers the callback handler
 * @note Called from ISR
 */
static void adxl34x_gpio_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pins);
	struct adxl34x_dev_data *data = CONTAINER_OF(cb, struct adxl34x_dev_data, gpio_cb);

	if (data->work.handler != NULL) {
		k_work_submit(&data->work);
	}
}

/**
 * Handler used after an interrupt was detected when RTIO is not enabled/used
 *
 * @param[in,out] work Pointer to the work item
 * @note Called from worker thread
 */
static void adxl34x_work_handler(struct k_work *work)
{
	struct adxl34x_dev_data *data = CONTAINER_OF(work, struct adxl34x_dev_data, work);
	const struct device *dev = data->dev;
	const struct adxl34x_dev_config *cfg = dev->config;
	int rc;
	enum pm_device_state pm_state;

	rc = pm_device_state_get(dev, &pm_state);
	if (rc == 0 && pm_state != PM_DEVICE_STATE_ACTIVE) {
		return;
	}

	struct adxl34x_int_source int_source;

	rc = adxl34x_get_int_source(dev, &int_source);
	ARG_UNUSED(rc);
	__ASSERT_NO_MSG(rc == 0);

	if ((data->data_ready_handler != NULL) &&
	    (int_source.data_ready || int_source.watermark || int_source.overrun)) {
		/* Keep reading samples until the interrupt de-asserts */
		while (gpio_pin_get_dt(&cfg->gpio_int1)) {
			data->data_ready_handler(dev, data->data_ready_trigger);
		}
	}
	adxl34x_handle_motion_events(dev, int_source);
}

/**
 * Handler used after an interrupt was detected when RTIO is enabled/used
 *
 * @param[in,out] work Pointer to the work item
 * @note Called from worker thread
 */
static void adxl34x_rtio_work_handler(struct k_work *work)
{
	const struct adxl34x_dev_data *data = CONTAINER_OF(work, struct adxl34x_dev_data, work);
	const struct device *dev = data->dev;
	const struct adxl34x_dev_config *cfg = dev->config;
	int rc;
	enum pm_device_state pm_state;

	rc = pm_device_state_get(dev, &pm_state);
	if (rc == 0 && pm_state != PM_DEVICE_STATE_ACTIVE) {
		return;
	}

	/* Keep reading samples from the FIFO until the interrupt de-asserts. */
	while (gpio_pin_get_dt(&cfg->gpio_int1)) {
		rc = adxl34x_rtio_handle_motion_data(dev);
		if (rc != 0) {
			break;
		}
	}
}

/**
 * Handle any motion events detected
 *
 * @param[in] dev Pointer to the sensor device
 * @param[in] int_source The source of the event
 * @note Called from worker thread
 */
void adxl34x_handle_motion_events(const struct device *dev, struct adxl34x_int_source int_source)
{
	struct adxl34x_dev_data *data = dev->data;

	if (data->motion_event_handler != NULL && int_source.single_tap) {
		data->motion_event_handler(dev, data->tap_trigger);
	}
	if (data->motion_event_handler != NULL && int_source.double_tap) {
		data->motion_event_handler(dev, data->double_tap_trigger);
	}
	if (data->motion_event_handler != NULL && int_source.free_fall) {
		data->motion_event_handler(dev, data->freefall_trigger);
	}
	if (data->motion_event_handler != NULL && int_source.activity) {
		data->motion_event_handler(dev, data->motion_trigger);
	}
	if (data->motion_event_handler != NULL && int_source.inactivity) {
		data->motion_event_handler(dev, data->stationary_trigger);
	}
}

/**
 * Clear the FIFO of all data
 *
 * @param[in] dev Pointer to the sensor device
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_trigger_flush(const struct device *dev)
{
	const struct adxl34x_dev_config *config = dev->config;
	int rc = 0;
	uint8_t rx_buf[6];

	LOG_DBG("Flushing the FIFO");
	/* Read all data from the fifo and discard it */
	for (int i = 0; i < ADXL34X_FIFO_SIZE; i++) {
		rc |= config->bus_read_buf(dev, ADXL34X_REG_DATA, rx_buf, sizeof(rx_buf));
	}
	return rc;
}

/**
 * Setup the adxl34x to send interrupts when needed
 *
 * @param[in] dev Pointer to the sensor device
 * @return 0 if successful, negative errno code if failure.
 */
static int adxl34x_trigger_enable_interrupt(const struct device *dev)
{
	const struct adxl34x_dev_config *config = dev->config;
	struct adxl34x_dev_data *data = dev->data;
	int rc;
	struct adxl34x_fifo_ctl fifo_ctl;

	if (IS_ENABLED(CONFIG_ADXL34X_FIFO_MODE_BYPASS)) {
		fifo_ctl.fifo_mode = ADXL34X_FIFO_MODE_BYPASS;
	} else if (IS_ENABLED(CONFIG_ADXL34X_FIFO_MODE_FIFO)) {
		fifo_ctl.fifo_mode = ADXL34X_FIFO_MODE_FIFO;
	} else if (IS_ENABLED(CONFIG_ADXL34X_FIFO_MODE_STREAM)) {
		fifo_ctl.fifo_mode = ADXL34X_FIFO_MODE_STREAM;
	} else if (IS_ENABLED(CONFIG_ADXL34X_FIFO_MODE_TRIGGER)) {
		fifo_ctl.fifo_mode = ADXL34X_FIFO_MODE_TRIGGER;
	} else {
		LOG_ERR("Unsupported FIFO mode (see CONFIG_ADXL34X_FIFO_MODE...)");
	}

	const uint8_t int_pin = config->dt_int_pin - 1;

	fifo_ctl.trigger = int_pin;
	fifo_ctl.samples = config->dt_packet_size;
	rc = adxl34x_set_fifo_ctl(dev, &fifo_ctl);
	if (rc != 0) {
		LOG_ERR("Failed to enable fifo mode");
		return rc;
	}

	struct adxl34x_data_format data_format = data->cfg.data_format;

	data_format.int_invert = (uint8_t)(config->gpio_int1.dt_flags | GPIO_ACTIVE_LOW);
	rc = adxl34x_set_data_format(dev, &data_format);
	if (rc) {
		LOG_ERR("Failed to set interrupt level on device (%s)", dev->name);
		return rc;
	}

	/* Use INT1 or INT2 to trigger each interrupt */
	struct adxl34x_int_map int_map = {
		.data_ready = int_pin,
		.watermark = int_pin,
		.overrun = int_pin,
		.single_tap = int_pin,
		.double_tap = int_pin,
		.free_fall = int_pin,
		.activity = int_pin,
		.inactivity = int_pin,
	};
	rc = adxl34x_set_int_map(dev, &int_map);
	if (rc != 0) {
		LOG_ERR("Failed to enable trigger interrupt");
		return rc;
	}

	struct adxl34x_int_enable int_enable = {0};

	if (data->iodev_sqe != NULL) {
		/* Enable the FIFO interrupts when in streaming mode */
		int_enable.watermark = true;
		int_enable.overrun = true;
	} else if (data->data_ready_handler != NULL) {
		/* When NOT in streaming mode */
		int_enable.data_ready = true;
		int_enable.watermark = true;
		int_enable.overrun = true;
	}
	if (data->motion_event_handler != NULL) {
		if (data->tap_trigger) {
			int_enable.single_tap = true;
		}
		if (data->double_tap_trigger) {
			int_enable.double_tap = true;
		}
		if (data->freefall_trigger) {
			int_enable.free_fall = true;
		}
		if (data->motion_trigger) {
			int_enable.activity = true;
		}
		if (data->stationary_trigger) {
			int_enable.inactivity = true;
		}
	}
	rc = adxl34x_set_int_enable(dev, &int_enable);
	if (rc != 0) {
		LOG_ERR("Failed to enable trigger interrupt");
		return rc;
	}
	return 0;
}

/**
 * Suspend the adxl34x from collecting data and sending interrupts
 *
 * @param[in] dev Pointer to the sensor device
 * @return 0 if successful, negative errno code if failure.
 */
static int adxl34x_suspend(const struct device *dev)
{
	const struct adxl34x_dev_config *cfg = dev->config;
	struct adxl34x_dev_data *data = dev->data;
	int rc;

	rc = gpio_pin_interrupt_configure_dt(&cfg->gpio_int1, GPIO_INT_DISABLE);
	ARG_UNUSED(rc);
	__ASSERT_NO_MSG(rc == 0);

	/* Disable the interrupts on the adxl34x */
	struct adxl34x_int_enable int_enable = {0};

	rc = adxl34x_set_int_enable(dev, &int_enable);
	ARG_UNUSED(rc);
	__ASSERT_NO_MSG(rc == 0);

	/* Stop the adxl34x from sampling */
	struct adxl34x_power_ctl power_ctl = data->cfg.power_ctl;

	power_ctl.measure = 0;
	rc = adxl34x_set_power_ctl(dev, &power_ctl);
	ARG_UNUSED(rc);
	__ASSERT_NO_MSG(rc == 0);

	/* Clear the FIFO of the adxl34x */
	rc = adxl34x_trigger_flush(dev);
	ARG_UNUSED(rc);
	__ASSERT_NO_MSG(rc == 0);
	return rc;
}

/**
 * Resume normal operation of the adxl34x, continue data collection and send interrupts
 *
 * @param[in] dev Pointer to the sensor device
 * @return 0 if successful, negative errno code if failure.
 */
static int adxl34x_resume(const struct device *dev)
{
	const struct adxl34x_dev_config *cfg = dev->config;
	struct adxl34x_dev_data *data = dev->data;
	int rc;

	rc = gpio_pin_interrupt_configure_dt(&cfg->gpio_int1, GPIO_INT_TRIGGER);
	ARG_UNUSED(rc);
	__ASSERT_NO_MSG(rc == 0);

	/* Re-configure and enable the interrupts of the adxl34x */
	rc = adxl34x_trigger_enable_interrupt(dev);
	ARG_UNUSED(rc);
	__ASSERT_NO_MSG(rc == 0);

	/* Start the adxl34x, enable sampling data */
	struct adxl34x_power_ctl power_ctl = data->cfg.power_ctl;

	power_ctl.measure = 1;
	rc = adxl34x_set_power_ctl(dev, &power_ctl);
	ARG_UNUSED(rc);
	__ASSERT_NO_MSG(rc == 0);
	return rc;
}

/**
 * Reset the adxl34x data interrupt to make sure it's de-asserted
 *
 * @param[in] dev Pointer to the sensor device
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_trigger_reset(const struct device *dev)
{
	struct adxl34x_dev_data *data = dev->data;
	int rc;

	/* Clear the FIFO of the adxl34x */
	rc = adxl34x_trigger_flush(dev);
	ARG_UNUSED(rc);
	__ASSERT_NO_MSG(rc == 0);

	/* Re-configure and enable the interrupts of the adxl34x */
	rc = adxl34x_trigger_enable_interrupt(dev);
	ARG_UNUSED(rc);
	__ASSERT_NO_MSG(rc == 0);

	/* Start the adxl34x, enable sampling data */
	struct adxl34x_power_ctl power_ctl = data->cfg.power_ctl;

	power_ctl.measure = 1;
	rc = adxl34x_set_power_ctl(dev, &power_ctl);
	ARG_UNUSED(rc);
	__ASSERT_NO_MSG(rc == 0);
	return rc;
}

/**
 * Callback API for setting a sensor's trigger and handler
 *
 * Prepare the MCU and adxl34x for receiving sensor interrupts
 *
 * @param[in] dev Pointer to the sensor device
 * @param[in] trig The trigger to activate
 * @param[in] handler The function that should be called when the trigger
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
	struct adxl34x_dev_data *data = dev->data;
	enum pm_device_state pm_state;
	int rc;

	if (trig == NULL || handler == NULL) {
		return -EINVAL;
	}

	rc = pm_device_state_get(dev, &pm_state);
	if (rc == 0 && pm_state != PM_DEVICE_STATE_ACTIVE) {
		return -EIO;
	}

	adxl34x_suspend(dev);

	switch (trig->type) {
	case SENSOR_TRIG_DATA_READY:     /* New data is ready */
	case SENSOR_TRIG_FIFO_WATERMARK: /* The FIFO watermark has been reached */
	case SENSOR_TRIG_FIFO_FULL:      /* The FIFO becomes full */
		data->data_ready_handler = handler;
		data->data_ready_trigger = trig;
		break;
	case SENSOR_TRIG_TAP: /* A single tap is detected. */
		data->motion_event_handler = handler;
		data->tap_trigger = trig;
		break;
	case SENSOR_TRIG_DOUBLE_TAP: /* A double tap is detected. */
		data->motion_event_handler = handler;
		data->double_tap_trigger = trig;
		break;
	case SENSOR_TRIG_FREEFALL: /* A free fall is detected. */
		data->motion_event_handler = handler;
		data->freefall_trigger = trig;
		break;
	case SENSOR_TRIG_MOTION: /* Motion is detected. */
		data->motion_event_handler = handler;
		data->motion_trigger = trig;
		break;
	case SENSOR_TRIG_STATIONARY: /* No motion has been detected for a while. */
		data->motion_event_handler = handler;
		data->stationary_trigger = trig;
		break;
	case SENSOR_TRIG_TIMER:
	case SENSOR_TRIG_DELTA:
	case SENSOR_TRIG_NEAR_FAR:
	case SENSOR_TRIG_THRESHOLD:
	case SENSOR_TRIG_COMMON_COUNT:
	case SENSOR_TRIG_MAX:
	default:
		return -ENOTSUP;
	}

	adxl34x_resume(dev);
	return 0;
}

/**
 * Setup this driver so it can support triggers
 *
 * @param[in] dev Pointer to the sensor device
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_trigger_init(const struct device *dev)
{
	const struct adxl34x_dev_config *config = dev->config;
	struct adxl34x_dev_data *data = dev->data;
	int rc = 0;

	if (data->work.handler == NULL) {
		if (!config->gpio_int1.port) {
			LOG_ERR("trigger enabled but no interrupt gpio supplied");
			return -ENODEV;
		}

		if (!gpio_is_ready_dt(&config->gpio_int1)) {
			LOG_ERR("gpio_int1 not ready");
			return -ENODEV;
		}

		/* Prepare the pin to receive interrupts */
		rc = gpio_pin_configure_dt(&config->gpio_int1, GPIO_INPUT);
		ARG_UNUSED(rc);
		__ASSERT_NO_MSG(rc == 0);
		gpio_init_callback(&data->gpio_cb, adxl34x_gpio_callback,
				   BIT(config->gpio_int1.pin));
		rc = gpio_add_callback(config->gpio_int1.port, &data->gpio_cb);
		if (rc != 0) {
			LOG_ERR("Failed to set gpio callback");
			return rc;
		}
	}

	data->dev = dev;
	/* Prepare to handle interrupt callback(s). Register the streaming handler when and rtio-sqe
	 * instance is available, otherwize register the normal handler. When the trigger is
	 * initialized twice the streaming handler always takes preference.
	 */
	if (data->iodev_sqe != NULL) {
		data->work.handler = adxl34x_rtio_work_handler;
	} else if (data->work.handler == NULL) {
		data->work.handler = adxl34x_work_handler;
	}

	/* Finally enable the interrupt it self */
	rc = gpio_pin_interrupt_configure_dt(&config->gpio_int1, GPIO_INT_TRIGGER);
	ARG_UNUSED(rc);
	__ASSERT_NO_MSG(rc == 0);
	return 0;
}
