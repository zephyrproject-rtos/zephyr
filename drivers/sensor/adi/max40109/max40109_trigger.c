/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "max40109.h"

LOG_MODULE_DECLARE(max40109);

#if defined(CONFIG_MAX40109_TRIGGER_OWN_THREAD) || defined(CONFIG_MAX40109_TRIGGER_GLOBAL_THREAD)

/**
 * @brief MAX40109 interrupt handling callback
 *
 * @param dev Device Structure
 */
static void max40109_thread_cb(const struct device *dev)
{
	const struct max40109_config *config = dev->config;
	struct max40109_data *data = dev->data;
	int ret = 0;
	uint8_t status_msb = 0;
	uint8_t status_lsb = 0;
	uint8_t status[2] = {0, 0};
	uint8_t faults;

	ret = max40109_reg_read(dev, MAX40109_REG_STATUS_MSB, &status_msb, 1);
	if (ret < 0) {
		return;
	}
	ret = max40109_reg_read(dev, MAX40109_REG_STATUS_LSB, &status_lsb, 1);
	if (ret < 0) {
		return;
	}

	LOG_DBG("Interrupt Status: 0x%02X 0x%02X", status_msb, status_lsb);

	/*
	 * DRV Fault and INT Fault have no enable bit in Interrupt Enable, so the
	 * device re-asserts ALERT for as long as the condition is present. Log
	 * the transitions only, otherwise a stuck fault floods the console.
	 */
	faults = status_msb & (MAX40109_DRV_FAULT_MASK | MAX40109_INT_FAULT_MASK);
	if (faults != data->fault_state) {
		uint8_t changed = faults ^ data->fault_state;

		if (changed & MAX40109_DRV_FAULT_MASK) {
			LOG_WRN("Drive Fault %s",
				(faults & MAX40109_DRV_FAULT_MASK) ? "asserted" : "cleared");
		}
		if (changed & MAX40109_INT_FAULT_MASK) {
			LOG_WRN("Input Temp Fault %s",
				(faults & MAX40109_INT_FAULT_MASK) ? "asserted" : "cleared");
		}

		data->fault_state = faults;
	}

	if ((status_msb & MAX40109_TEMP_DATA_READY_MASK) && data->temp_data_rdy_handler) {
		data->temp_data_rdy_handler(dev, data->temp_data_rdy_trigger);
	}
	if ((status_msb & MAX40109_PRESSURE_DATA_READY_MASK) && data->pressure_data_rdy_handler) {
		data->pressure_data_rdy_handler(dev, data->pressure_data_rdy_trigger);
	}
	if ((status_lsb & MAX40109_OV_DRV_MASK) && data->ov_drv_handler) {
		data->ov_drv_handler(dev, data->ov_drv_trigger);
	}
	if ((status_lsb & MAX40109_UV_DRV_MASK) && data->uv_drv_handler) {
		data->uv_drv_handler(dev, data->uv_drv_trigger);
	}
	if ((status_lsb & MAX40109_OV_INT_MASK) && data->ov_int_handler) {
		data->ov_int_handler(dev, data->ov_int_trigger);
	}
	if ((status_lsb & MAX40109_UV_INT_MASK) && data->uv_int_handler) {
		data->uv_int_handler(dev, data->uv_int_trigger);
	}
	if ((status_lsb & MAX40109_OV_INP_PLUS_MASK) && data->ov_inp_positive_handler) {
		data->ov_inp_positive_handler(dev, data->ov_inp_positive_trigger);
	}
	if ((status_lsb & MAX40109_UV_INP_PLUS_MASK) && data->uv_inp_positive_handler) {
		data->uv_inp_positive_handler(dev, data->uv_inp_positive_trigger);
	}
	if ((status_lsb & MAX40109_OV_INP_MINUS_MASK) && data->ov_inp_negative_handler) {
		data->ov_inp_negative_handler(dev, data->ov_inp_negative_trigger);
	}
	if ((status_lsb & MAX40109_UV_INP_MINUS_MASK) && data->uv_inp_negative_handler) {
		data->uv_inp_negative_handler(dev, data->uv_inp_negative_trigger);
	}

	status[0] = status_msb;
	status[1] = status_lsb;

	/* Clear the interrupt status */
	ret = max40109_reg_write_multiple(dev, MAX40109_REG_STATUS_MSB, status, 2);
	if (ret < 0) {
		LOG_ERR("Failed to clear interrupt status");
	}

	ret = gpio_pin_interrupt_configure_dt(&config->interrupt_gpio, GPIO_INT_LEVEL_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to re-enable interrupt");
	}
}

#endif /* CONFIG_MAX40109_TRIGGER_* */

/**
 * @brief GPIO callback function for MAX40109 interrupt
 *
 * @param dev Device Structure
 * @param cb GPIO Callback Structure
 * @param pins Pin mask
 */
static void max40109_gpio_callback(const struct device *dev, struct gpio_callback *cb,
				   uint32_t pins)
{
	struct max40109_data *data = CONTAINER_OF(cb, struct max40109_data, gpio_cb);
	const struct max40109_config *config = data->dev->config;

	gpio_pin_interrupt_configure_dt(&config->interrupt_gpio, GPIO_INT_DISABLE);

#if defined(CONFIG_MAX40109_TRIGGER_OWN_THREAD)
	k_sem_give(&data->gpio_sem);
#elif defined(CONFIG_MAX40109_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
}

#if defined(CONFIG_MAX40109_TRIGGER_OWN_THREAD)
/**
 * @brief MAX40109 interrupt handling thread for own thread configuration
 *
 * @param p1 Parameter 1 Containing device data
 * @param p2 Unused
 * @param p3 Unused
 */
static void max40109_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct max40109_data *data = p1;
	const struct device *dev = data->dev;

	while (1) {
		k_sem_take(&data->gpio_sem, K_FOREVER);
		max40109_thread_cb(dev);
	}
}
#elif defined(CONFIG_MAX40109_TRIGGER_GLOBAL_THREAD)
/**
 * @brief MAX40109 interrupt handling work callback for global thread configuration
 *
 * @param work Work structure
 */
static void max40109_work_cb(struct k_work *work)
{
	struct max40109_data *data = CONTAINER_OF(work, struct max40109_data, work);
	const struct device *dev = data->dev;

	max40109_thread_cb(dev);
}
#endif

/**
 * @brief Set sensor trigger and its handler for MAX40109
 *
 * @param dev Device Structure
 * @param trig Sensor Trigger Structure
 * @param handler Trigger Handler Function Pointer
 * @return int 0 on success, negative error code on failure
 */
int max40109_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler)
{
	const struct max40109_config *config = dev->config;
	struct max40109_data *data = dev->data;

	int ret = 0;

	uint8_t int_mask = 0;
	uint8_t enable = 0;
	bool msb = false;

	if (!config->interrupt_gpio.port) {
		return -ENOTSUP;
	}

	ret = gpio_pin_interrupt_configure_dt(&config->interrupt_gpio, GPIO_INT_DISABLE);
	if (ret < 0) {
		return ret;
	}

	switch ((int)trig->type) {

	case SENSOR_TRIG_DATA_READY:
		data->pressure_data_rdy_handler = handler;
		data->pressure_data_rdy_trigger = trig;
		int_mask |= MAX40109_PRESSURE_DATA_READY_MASK;
		msb = true;
		break;

	case SENSOR_TRIG_MAX40109_PRESSURE_DATA_READY:
		data->pressure_data_rdy_handler = handler;
		data->pressure_data_rdy_trigger = trig;
		int_mask |= MAX40109_PRESSURE_DATA_READY_MASK;
		msb = true;
		break;

	case SENSOR_TRIG_MAX40109_TEMP_DATA_READY:
		data->temp_data_rdy_handler = handler;
		data->temp_data_rdy_trigger = trig;
		int_mask |= MAX40109_TEMP_DATA_READY_MASK;
		msb = true;
		break;

	case SENSOR_TRIG_MAX40109_DRIVE_VOLTAGE_OV:
		data->ov_drv_handler = handler;
		data->ov_drv_trigger = trig;
		int_mask |= MAX40109_OV_DRV_MASK;
		break;

	case SENSOR_TRIG_MAX40109_DRIVE_VOLTAGE_UV:
		data->uv_drv_handler = handler;
		data->uv_drv_trigger = trig;
		int_mask |= MAX40109_UV_DRV_MASK;
		break;

	case SENSOR_TRIG_MAX40109_INPUT_TEMPERATURE_VOLTAGE_OV:
		data->ov_int_handler = handler;
		data->ov_int_trigger = trig;
		int_mask |= MAX40109_OV_INT_MASK;
		break;

	case SENSOR_TRIG_MAX40109_INPUT_TEMPERATURE_VOLTAGE_UV:
		data->uv_int_handler = handler;
		data->uv_int_trigger = trig;
		int_mask |= MAX40109_UV_INT_MASK;
		break;

	case SENSOR_TRIG_MAX40109_INPUT_PRESSURE_POSITIVE_OV:
		data->ov_inp_positive_handler = handler;
		data->ov_inp_positive_trigger = trig;
		int_mask |= MAX40109_OV_INP_PLUS_MASK;
		break;

	case SENSOR_TRIG_MAX40109_INPUT_PRESSURE_POSITIVE_UV:
		data->uv_inp_positive_handler = handler;
		data->uv_inp_positive_trigger = trig;
		int_mask |= MAX40109_UV_INP_PLUS_MASK;
		break;

	case SENSOR_TRIG_MAX40109_INPUT_PRESSURE_NEGATIVE_OV:
		data->ov_inp_negative_handler = handler;
		data->ov_inp_negative_trigger = trig;
		int_mask |= MAX40109_OV_INP_MINUS_MASK;
		break;

	case SENSOR_TRIG_MAX40109_INPUT_PRESSURE_NEGATIVE_UV:
		data->uv_inp_negative_handler = handler;
		data->uv_inp_negative_trigger = trig;
		int_mask |= MAX40109_UV_INP_MINUS_MASK;
		break;

	default:
		/* Restore the interrupt that was disabled above before bailing out. */
		(void)gpio_pin_interrupt_configure_dt(&config->interrupt_gpio,
						      GPIO_INT_LEVEL_ACTIVE);
		return -ENOTSUP;
	}

	enable = (handler != NULL) ? 1 : 0;

	if (msb) {
		ret = max40109_reg_update(dev, MAX40109_INTERRUPT_ENABLE_MSB, int_mask, enable);
	} else {
		ret = max40109_reg_update(dev, MAX40109_INTERRUPT_ENABLE_LSB, int_mask, enable);
	}
	if (ret < 0) {
		return ret;
	}

	return gpio_pin_interrupt_configure_dt(&config->interrupt_gpio, GPIO_INT_LEVEL_ACTIVE);
}

/**
 * @brief Initialize MAX40109 interrupt
 *
 * @param dev Device Structure
 * @return int 0 on success, negative error code on failure
 */
int max40109_init_interrupt(const struct device *dev)
{
	const struct max40109_config *config = dev->config;
	struct max40109_data *data = dev->data;

	int ret;

	if (!gpio_is_ready_dt(&config->interrupt_gpio)) {
		return -ENODEV;
	}
	ret = gpio_pin_configure_dt(&config->interrupt_gpio, GPIO_INPUT);
	if (ret < 0) {
		return ret;
	}

	gpio_init_callback(&data->gpio_cb, max40109_gpio_callback, BIT(config->interrupt_gpio.pin));

	ret = gpio_add_callback(config->interrupt_gpio.port, &data->gpio_cb);
	if (ret < 0) {
		return ret;
	}

	data->dev = dev;

#if defined(CONFIG_MAX40109_TRIGGER_OWN_THREAD)
	k_sem_init(&data->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&data->thread, data->thread_stack, CONFIG_MAX40109_THREAD_STACK_SIZE,
			max40109_thread, data, NULL, NULL,
			K_PRIO_COOP(CONFIG_MAX40109_THREAD_PRIORITY), 0, K_NO_WAIT);

	k_thread_name_set(&data->thread, dev->name);
#elif defined(CONFIG_MAX40109_TRIGGER_GLOBAL_THREAD)
	data->work.handler = max40109_work_cb;
#endif

	return gpio_pin_interrupt_configure_dt(&config->interrupt_gpio, GPIO_INT_LEVEL_ACTIVE);
}
