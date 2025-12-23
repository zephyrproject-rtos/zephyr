/*
 * Copyright (c) 2025 Würth Elektronik eiSos GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT we_wsen_isds_2536030320001

#include <zephyr/logging/log.h>

#include "wsen_isds_2536030320001.h"

LOG_MODULE_DECLARE(WSEN_ISDS_2536030320001, CONFIG_SENSOR_LOG_LEVEL);

/* Enable/disable interrupt handling for data-ready, tap, free-fall, delta/wake-up */
static inline int isds_2536030320001_setup_interrupt(const struct device *dev,
						     const struct gpio_dt_spec *pin, bool enable)
{
	unsigned int flags = enable ? GPIO_INT_EDGE_TO_ACTIVE : GPIO_INT_DISABLE;

	return gpio_pin_interrupt_configure_dt(pin, flags);
}

static inline void isds_2536030320001_handle_interrupt_1(const struct device *dev)
{
	struct isds_2536030320001_data *data = dev->data;
	const struct isds_2536030320001_config *cfg = dev->config;

	/* Disable interrupt handling until the interrupt has been processed */
	isds_2536030320001_setup_interrupt(data->dev, &cfg->drdy_interrupt_gpio, false);

#if defined(CONFIG_WSEN_ISDS_2536030320001_TRIGGER_OWN_THREAD)
	k_sem_give(&data->drdy_sem);
#elif defined(CONFIG_WSEN_ISDS_2536030320001_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->drdy_work);
#endif
}

static void isds_2536030320001_process_interrupt_1(const struct device *dev)
{
	struct isds_2536030320001_data *data = dev->data;
	const struct isds_2536030320001_config *cfg = dev->config;
	ISDS_status_t status_reg;

	if (ISDS_getStatusRegister(&data->sensor_interface, &status_reg) != WE_SUCCESS) {
		LOG_ERR("Failed to read status register");
		return;
	}

	if (data->accel_data_ready_handler && status_reg.accDataReady) {
		data->accel_data_ready_handler(dev, data->accel_data_ready_trigger);
	}

	if (data->gyro_data_ready_handler && status_reg.gyroDataReady) {
		data->gyro_data_ready_handler(dev, data->gyro_data_ready_trigger);
	}

	if (data->temp_data_ready_handler && status_reg.tempDataReady) {
		data->temp_data_ready_handler(dev, data->temp_data_ready_trigger);
	}

	/* Re-enable interrupt handling */
	isds_2536030320001_setup_interrupt(dev, &cfg->drdy_interrupt_gpio, true);
}

/*
 * Is called when interrupt on INT1.
 * Triggers asynchronous handling of interrupt in isds_2536030320001_process_interrupt().
 */
static void isds_2536030320001_interrupt_1_gpio_callback(const struct device *dev,
							 struct gpio_callback *cb, uint32_t pins)
{
	struct isds_2536030320001_data *data =
		CONTAINER_OF(cb, struct isds_2536030320001_data, drdy_interrupt_cb);

	ARG_UNUSED(pins);

	isds_2536030320001_handle_interrupt_1(data->dev);
}

#ifdef CONFIG_WSEN_ISDS_2536030320001_EVENTS
static inline void isds_2536030320001_handle_interrupt_0(const struct device *dev)
{
	struct isds_2536030320001_data *data = dev->data;
	const struct isds_2536030320001_config *cfg = dev->config;

	/* Disable interrupt handling until the interrupt has been processed */
	isds_2536030320001_setup_interrupt(data->dev, &cfg->events_interrupt_gpio, false);

#if defined(CONFIG_WSEN_ISDS_2536030320001_TRIGGER_OWN_THREAD)
	k_sem_give(&data->events_sem);
#elif defined(CONFIG_WSEN_ISDS_2536030320001_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->events_work);
#endif
}

/* Asynchronous handling of interrupt triggered in isds_2536030320001_gpio_callback() */
static void isds_2536030320001_process_interrupt_0(const struct device *dev)
{
	struct isds_2536030320001_data *data = dev->data;
	const struct isds_2536030320001_config *cfg = dev->config;

#ifdef CONFIG_WSEN_ISDS_2536030320001_TAP
	ISDS_tapEvent_t tap_event;
	/* Read tap event register to find out if a tap event interrupt occurred. */
	if (ISDS_getTapEventRegister(&data->sensor_interface, &tap_event) != WE_SUCCESS) {
		LOG_ERR("Failed to read tap event register");
		return;
	}

	if (data->single_tap_handler && tap_event.singleState) {
		data->single_tap_handler(dev, data->single_tap_trigger);
	}

	if (data->double_tap_handler && tap_event.doubleState) {
		data->double_tap_handler(dev, data->double_tap_trigger);
	}
#endif /* CONFIG_WSEN_ISDS_2536030320001_TAP */

#if defined(CONFIG_WSEN_ISDS_2536030320001_FREEFALL) ||                                            \
	defined(CONFIG_WSEN_ISDS_2536030320001_DELTA)

	ISDS_wakeUpEvent_t wake_up_event;
	/* Read wake-up event register to find out if freefall or wake-up interrupt occurred. */
	if (ISDS_getWakeUpEventRegister(&data->sensor_interface, &wake_up_event) != WE_SUCCESS) {
		LOG_ERR("Failed to read wake-up event register");
		return;
	}
#endif /* CONFIG_WSEN_ISDS_2536030320001_FREEFALL || CONFIG_WSEN_ISDS_2536030320001_DELTA */

#ifdef CONFIG_WSEN_ISDS_2536030320001_FREEFALL
	if (data->freefall_handler && wake_up_event.freeFallState) {
		data->freefall_handler(dev, data->freefall_trigger);
	}
#endif /* CONFIG_WSEN_ISDS_2536030320001_FREEFALL */

#ifdef CONFIG_WSEN_ISDS_2536030320001_DELTA
	if (data->delta_handler && wake_up_event.wakeUpState) {
		data->delta_handler(dev, data->delta_trigger);
	}
#endif /* CONFIG_WSEN_ISDS_2536030320001_DELTA */

	/* Re-enable interrupt handling */
	isds_2536030320001_setup_interrupt(dev, &cfg->events_interrupt_gpio, true);
}

/*
 * Is called when interrupt on INT0.
 * Triggers asynchronous handling of interrupt in isds_2536030320001_process_interrupt().
 */
static void isds_2536030320001_interrupt_0_gpio_callback(const struct device *dev,
							 struct gpio_callback *cb, uint32_t pins)
{
	struct isds_2536030320001_data *data =
		CONTAINER_OF(cb, struct isds_2536030320001_data, events_interrupt_cb);

	ARG_UNUSED(pins);

	isds_2536030320001_handle_interrupt_0(data->dev);
}

#endif /* CONFIG_WSEN_ISDS_2536030320001_EVENTS */

#ifdef CONFIG_WSEN_ISDS_2536030320001_TRIGGER_OWN_THREAD
static void isds_2536030320001_drdy_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct isds_2536030320001_data *data = p1;

	while (true) {
		k_sem_take(&data->drdy_sem, K_FOREVER);
		isds_2536030320001_process_interrupt_1(data->dev);
	}
}
#ifdef CONFIG_WSEN_ISDS_2536030320001_EVENTS
static void isds_2536030320001_events_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct isds_2536030320001_data *data = p1;

	while (true) {
		k_sem_take(&data->events_sem, K_FOREVER);
		isds_2536030320001_process_interrupt_0(data->dev);
	}
}
#endif /* CONFIG_WSEN_ISDS_2536030320001_EVENTS */
#endif /* CONFIG_WSEN_ISDS_2536030320001_TRIGGER_OWN_THREAD */

#ifdef CONFIG_WSEN_ISDS_2536030320001_TRIGGER_GLOBAL_THREAD
static void isds_2536030320001_drdy_work_cb(struct k_work *work)
{
	struct isds_2536030320001_data *isds_2536030320001 =
		CONTAINER_OF(work, struct isds_2536030320001_data, drdy_work);

	isds_2536030320001_process_interrupt_1(isds_2536030320001->dev);
}

#ifdef CONFIG_WSEN_ISDS_2536030320001_EVENTS
static void isds_2536030320001_events_work_cb(struct k_work *work)
{
	struct isds_2536030320001_data *isds_2536030320001 =
		CONTAINER_OF(work, struct isds_2536030320001_data, events_work);

	isds_2536030320001_process_interrupt_0(isds_2536030320001->dev);
}
#endif /* CONFIG_WSEN_ISDS_2536030320001_EVENTS */
#endif /* CONFIG_WSEN_ISDS_2536030320001_TRIGGER_GLOBAL_THREAD */

/* (Un)register trigger handler and enable/disable the corresponding sensor interrupt */
int isds_2536030320001_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
				   sensor_trigger_handler_t handler)
{
	struct isds_2536030320001_data *data = dev->data;
	const struct isds_2536030320001_config *cfg = dev->config;

	ISDS_state_t state = (handler != NULL) ? ISDS_enable : ISDS_disable;

	switch (trig->type) {
	case SENSOR_TRIG_DATA_READY: {
		int16_t x, y, z;

		switch (trig->chan) {
		case SENSOR_CHAN_ACCEL_X:
		case SENSOR_CHAN_ACCEL_Y:
		case SENSOR_CHAN_ACCEL_Z:
		case SENSOR_CHAN_ACCEL_XYZ: {
			data->accel_data_ready_handler = handler;
			data->accel_data_ready_trigger = trig;
			isds_2536030320001_setup_interrupt(dev, &cfg->drdy_interrupt_gpio,
							   data->accel_data_ready_handler ||
								   data->gyro_data_ready_handler ||
								   data->temp_data_ready_handler);
			if (state) {
				/* Dummy read: re-trigger interrupt */
				ISDS_getRawAccelerations(&data->sensor_interface, &x, &y, &z);
			}
			if (ISDS_enableAccDataReadyINT1(&data->sensor_interface, state) !=
			    WE_SUCCESS) {
				return -EIO;
			}
			break;
		}
		case SENSOR_CHAN_GYRO_X:
		case SENSOR_CHAN_GYRO_Y:
		case SENSOR_CHAN_GYRO_Z:
		case SENSOR_CHAN_GYRO_XYZ: {
			data->gyro_data_ready_handler = handler;
			data->gyro_data_ready_trigger = trig;
			isds_2536030320001_setup_interrupt(dev, &cfg->drdy_interrupt_gpio,
							   data->accel_data_ready_handler ||
								   data->gyro_data_ready_handler ||
								   data->temp_data_ready_handler);
			if (state) {
				/* Dummy read: re-trigger interrupt */
				ISDS_getRawAngularRates(&data->sensor_interface, &x, &y, &z);
			}
			if (ISDS_enableGyroDataReadyINT1(&data->sensor_interface, state) !=
			    WE_SUCCESS) {
				return -EIO;
			}
			break;
		}
		case SENSOR_CHAN_AMBIENT_TEMP: {
			data->temp_data_ready_handler = handler;
			data->temp_data_ready_trigger = trig;
			isds_2536030320001_setup_interrupt(dev, &cfg->drdy_interrupt_gpio,
							   data->accel_data_ready_handler ||
								   data->gyro_data_ready_handler ||
								   data->temp_data_ready_handler);
			if (state) {
				/* Dummy read: re-trigger interrupt */
				ISDS_getRawTemperature(&data->sensor_interface, &x);
			}
			if (ISDS_enableTemperatureDataReadyINT1(&data->sensor_interface, state) !=
			    WE_SUCCESS) {
				return -EIO;
			}
			break;
		}
		default:
			goto error;
		}
		return 0;
	}
#ifdef CONFIG_WSEN_ISDS_2536030320001_TAP
	case SENSOR_TRIG_TAP:
		if (trig->chan != SENSOR_CHAN_ALL) {
			break;
		}
		data->single_tap_handler = handler;
		data->single_tap_trigger = trig;
		isds_2536030320001_setup_interrupt(
			dev, &cfg->events_interrupt_gpio,
			data->single_tap_handler || data->double_tap_handler ||
				data->freefall_handler || data->delta_handler);
		if (ISDS_enableSingleTapINT0(&data->sensor_interface, state) != WE_SUCCESS) {
			return -EIO;
		}
		return 0;
	case SENSOR_TRIG_DOUBLE_TAP:
		if (trig->chan != SENSOR_CHAN_ALL) {
			break;
		}
		data->double_tap_handler = handler;
		data->double_tap_trigger = trig;
		isds_2536030320001_setup_interrupt(
			dev, &cfg->events_interrupt_gpio,
			data->single_tap_handler || data->double_tap_handler ||
				data->freefall_handler || data->delta_handler);
		if (ISDS_enableDoubleTapINT0(&data->sensor_interface, state) != WE_SUCCESS) {
			return -EIO;
		}
		return 0;
#endif /* CONFIG_WSEN_ISDS_2536030320001_TAP */

#ifdef CONFIG_WSEN_ISDS_2536030320001_FREEFALL
	case SENSOR_TRIG_FREEFALL:
		if (trig->chan != SENSOR_CHAN_ALL) {
			break;
		}
		data->freefall_handler = handler;
		data->freefall_trigger = trig;
		isds_2536030320001_setup_interrupt(
			dev, &cfg->events_interrupt_gpio,
			data->single_tap_handler || data->double_tap_handler ||
				data->freefall_handler || data->delta_handler);
		if (ISDS_enableFreeFallINT0(&data->sensor_interface, state) != WE_SUCCESS) {
			return -EIO;
		}
		return 0;
#endif /* CONFIG_WSEN_ISDS_2536030320001_FREEFALL */

#ifdef CONFIG_WSEN_ISDS_2536030320001_DELTA
	case SENSOR_TRIG_DELTA:
		if (trig->chan != SENSOR_CHAN_ALL) {
			break;
		}
		data->delta_handler = handler;
		data->delta_trigger = trig;
		isds_2536030320001_setup_interrupt(
			dev, &cfg->events_interrupt_gpio,
			data->single_tap_handler || data->double_tap_handler ||
				data->freefall_handler || data->delta_handler);
		if (ISDS_enableWakeUpINT0(&data->sensor_interface, state) != WE_SUCCESS) {
			return -EIO;
		}
		return 0;
#endif /* CONFIG_WSEN_ISDS_2536030320001_DELTA */
	default:
		break;
	}

error:
	LOG_ERR("Unsupported sensor trigger");
	return -ENOTSUP;
}

int isds_2536030320001_init_interrupt(const struct device *dev)
{
	struct isds_2536030320001_data *data = dev->data;
	const struct isds_2536030320001_config *cfg = dev->config;

	data->dev = dev;

	if (cfg->drdy_interrupt_gpio.port == NULL) {
		LOG_ERR("drdy-interrupt-gpios is not defined in the device tree.");
		return -EINVAL;
	}

	if (!gpio_is_ready_dt(&cfg->drdy_interrupt_gpio)) {
		LOG_ERR("Device %s is not ready", cfg->drdy_interrupt_gpio.port->name);
		return -ENODEV;
	}

	/* Setup interrupt gpio */
	if (gpio_pin_configure_dt(&cfg->drdy_interrupt_gpio, GPIO_INPUT) < 0) {
		LOG_ERR("Failed to configure %s.%02u", cfg->drdy_interrupt_gpio.port->name,
			cfg->drdy_interrupt_gpio.pin);
		return -EIO;
	}

	gpio_init_callback(&data->drdy_interrupt_cb, isds_2536030320001_interrupt_1_gpio_callback,
			   BIT(cfg->drdy_interrupt_gpio.pin));

	if (gpio_add_callback(cfg->drdy_interrupt_gpio.port, &data->drdy_interrupt_cb) < 0) {
		LOG_ERR("Failed to set gpio callback");
		return -EIO;
	}

#if defined(CONFIG_WSEN_ISDS_2536030320001_EVENTS)

	if (cfg->events_interrupt_gpio.port == NULL) {
		LOG_DBG("events-interrupt-gpios is not defined in the device tree.");
		return -EINVAL;
	}

	if (!gpio_is_ready_dt(&cfg->events_interrupt_gpio)) {
		LOG_ERR("Device %s is not ready", cfg->events_interrupt_gpio.port->name);
		return -ENODEV;
	}

	/* Setup interrupt gpio */
	if (gpio_pin_configure_dt(&cfg->events_interrupt_gpio, GPIO_INPUT) < 0) {
		LOG_ERR("Failed to configure %s.%02u", cfg->events_interrupt_gpio.port->name,
			cfg->events_interrupt_gpio.pin);
		return -EIO;
	}

	gpio_init_callback(&data->events_interrupt_cb, isds_2536030320001_interrupt_0_gpio_callback,
			   BIT(cfg->events_interrupt_gpio.pin));

	if (gpio_add_callback(cfg->events_interrupt_gpio.port, &data->events_interrupt_cb) < 0) {
		LOG_ERR("Failed to set gpio callback");
		return -EIO;
	}

#endif

#if defined(CONFIG_WSEN_ISDS_2536030320001_TRIGGER_OWN_THREAD)
	k_sem_init(&data->drdy_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&data->drdy_thread, data->drdy_thread_stack,
			CONFIG_WSEN_ISDS_2536030320001_THREAD_STACK_SIZE,
			isds_2536030320001_drdy_thread, data, NULL, NULL,
			K_PRIO_COOP(CONFIG_WSEN_ISDS_2536030320001_THREAD_PRIORITY), 0, K_NO_WAIT);
#if defined(CONFIG_WSEN_ISDS_2536030320001_EVENTS)
	k_sem_init(&data->events_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&data->events_thread, data->events_thread_stack,
			CONFIG_WSEN_ISDS_2536030320001_THREAD_STACK_SIZE,
			isds_2536030320001_events_thread, data, NULL, NULL,
			K_PRIO_COOP(CONFIG_WSEN_ISDS_2536030320001_THREAD_PRIORITY), 0, K_NO_WAIT);
#endif
#elif defined(CONFIG_WSEN_ISDS_2536030320001_TRIGGER_GLOBAL_THREAD)
	data->drdy_work.handler = isds_2536030320001_drdy_work_cb;
#if defined(CONFIG_WSEN_ISDS_2536030320001_EVENTS)
	data->events_work.handler = isds_2536030320001_events_work_cb;
#endif
#endif

	/* Enable interrupt on INT_0/INT_1 in pulsed mode */
	if (ISDS_enableLatchedInterrupt(&data->sensor_interface, ISDS_disable) != WE_SUCCESS) {
		LOG_ERR("Failed to disable latched mode");
		return -EIO;
	}

	/* Enable data-ready interrupt on INT_0/INT_1 in pulsed mode */
	if (ISDS_enableDataReadyPulsed(&data->sensor_interface, ISDS_enable) != WE_SUCCESS) {
		LOG_ERR("Failed to enable data-ready pulsed mode");
		return -EIO;
	}

	if (ISDS_enableInterrupts(&data->sensor_interface, ISDS_enable) != WE_SUCCESS) {
		LOG_ERR("Failed to enable interrupts");
		return -EIO;
	}

#ifdef CONFIG_WSEN_ISDS_2536030320001_TAP
	if (cfg->accel_odr < ISDS_accOdr416Hz || cfg->accel_odr >= ISDS_accOdr1Hz6) {
		LOG_WRN("The tap recognition feature requires a minimum output data rate "
			"of 416 Hz");
	}

	if (ISDS_enableDoubleTapEvent(&data->sensor_interface,
				      (cfg->tap_mode == 1) ? ISDS_enable : ISDS_disable) !=
	    WE_SUCCESS) {
		LOG_ERR("Failed to enable/disable double tap event");
		return -EIO;
	}

	if (ISDS_setTapThreshold(&data->sensor_interface, cfg->tap_threshold) != WE_SUCCESS) {
		LOG_ERR("Failed to set tap threshold");
		return -EIO;
	}

	if (cfg->tap_axis_enable[0] != 0) {
		if (ISDS_enableTapX(&data->sensor_interface, ISDS_enable) != WE_SUCCESS) {
			LOG_ERR("Failed to enable tap recognition in X direction");
			return -EIO;
		}
	}

	if (cfg->tap_axis_enable[1] != 0) {
		if (ISDS_enableTapY(&data->sensor_interface, ISDS_enable) != WE_SUCCESS) {
			LOG_ERR("Failed to enable tap recognition in Y direction");
			return -EIO;
		}
	}

	if (cfg->tap_axis_enable[2] != 0) {
		if (ISDS_enableTapZ(&data->sensor_interface, ISDS_enable) != WE_SUCCESS) {
			LOG_ERR("Failed to enable tap recognition in Z direction");
			return -EIO;
		}
	}

	if (ISDS_setTapShockTime(&data->sensor_interface, cfg->tap_shock) != WE_SUCCESS) {
		LOG_ERR("Failed to set tap shock duration");
		return -EIO;
	}

	if (ISDS_setTapLatencyTime(&data->sensor_interface, cfg->tap_latency) != WE_SUCCESS) {
		LOG_ERR("Failed to set tap latency");
		return -EIO;
	}

	if (ISDS_setTapQuietTime(&data->sensor_interface, cfg->tap_quiet) != WE_SUCCESS) {
		LOG_ERR("Failed to set tap quiet time");
		return -EIO;
	}
#endif /* CONFIG_WSEN_ISDS_2536030320001_TAP */

#ifdef CONFIG_WSEN_ISDS_2536030320001_FREEFALL
	if (ISDS_setFreeFallDuration(&data->sensor_interface, cfg->freefall_duration) !=
	    WE_SUCCESS) {
		LOG_ERR("Failed to set free-fall duration");
		return -EIO;
	}

	if (ISDS_setFreeFallThreshold(&data->sensor_interface, cfg->freefall_threshold) !=
	    WE_SUCCESS) {
		LOG_ERR("Failed to set free-fall threshold");
		return -EIO;
	}
#endif /* CONFIG_WSEN_ISDS_2536030320001_FREEFALL */

#ifdef CONFIG_WSEN_ISDS_2536030320001_DELTA
	if (ISDS_setWakeUpDuration(&data->sensor_interface, cfg->delta_duration) != WE_SUCCESS) {
		LOG_ERR("Failed to set wake-up duration");
		return -EIO;
	}

	if (ISDS_setWakeUpThreshold(&data->sensor_interface, cfg->delta_threshold) != WE_SUCCESS) {
		LOG_ERR("Failed to set wake-up threshold");
		return -EIO;
	}
#endif /* CONFIG_WSEN_ISDS_2536030320001_DELTA */

	return 0;
}
