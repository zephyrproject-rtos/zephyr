/*
 * Copyright (c) 2023 Fraunhofer AICOS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/drivers/fuel_gauge/max17048.h>
#include "max17048.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(MAX17048);

/* Forward declaration of internal functions */
static int max17048_read_register(const struct device *dev, uint8_t registerId, uint16_t *response);
static int max17048_update_register(const struct device *dev, uint8_t reg, uint16_t mask,
				    uint16_t val);

static int max17048_clear_alert(const struct device *dev);
static void max17048_int_callback(const struct device *port, struct gpio_callback *cb,
				  uint32_t pin);
static void max17048_int_work(struct k_work *work);

static int max17048_undervoltage_threshold_set(const struct device *dev, const uint16_t voltage);
static int max17048_overvoltage_threshold_set(const struct device *dev, const uint16_t voltage);
static int max17048_low_soc_threshold_set(const struct device *dev, const uint8_t soc);

/* Public interface */
int max17048_trigger_set(const struct device *dev, const enum max17048_trigger_type trigger_type,
			 max17048_trigger_handler_t handler)
{
	struct max17048_data *drv_data = dev->data;
	const struct max17048_config *drv_config = dev->config;
	int rc = 0;

	if (dev == NULL || handler == NULL) {
		return -EINVAL;
	}
	switch (trigger_type) {
	case MAX17048_TRIGGER_UNDERVOLTAGE:
		rc = max17048_undervoltage_threshold_set(dev, drv_config->undervoltage_threshold);
		if (rc < 0) {
			LOG_ERR("Failed to set under-voltage value.");
		};
		drv_data->trigger_undervoltage_handler = handler;
		break;
	case MAX17048_TRIGGER_OVERVOLTAGE:
		rc = max17048_overvoltage_threshold_set(dev, drv_config->overvoltage_threshold);
		if (rc < 0) {
			LOG_ERR("Failed to set under-voltage value.");
		};
		drv_data->trigger_overvoltage_handler = handler;
		break;
	case MAX17048_TRIGGER_LOW_SOC:
		rc = max17048_low_soc_threshold_set(dev, drv_config->low_soc_threshold);
		if (rc < 0) {
			LOG_ERR("Failed to set under-voltage value.");
		};
		drv_data->trigger_low_soc_handler = handler;
		break;
	default:
		break;
	}
	return 0;
}

int max17048_trigger_init(const struct device *dev)
{
	const struct max17048_config *drv_cfg = dev->config;
	struct max17048_data *drv_data = dev->data;
	int rc;

	if (dev == NULL) {
		return -EINVAL;
	}

	drv_data->dev = dev;
	drv_data->work.handler = max17048_int_work;

	if (!gpio_is_ready_dt(&drv_cfg->int_gpio)) {
		LOG_ERR("Interrupt pin is not ready.");
		return -EIO;
	}

	if (gpio_pin_configure_dt(&drv_cfg->int_gpio, GPIO_INPUT) < 0) {
		LOG_ERR("Failed to configure %s pin %d", drv_cfg->int_gpio.port->name,
			drv_cfg->int_gpio.pin);
		return -EIO;
	}

	gpio_init_callback((struct gpio_callback *)&drv_data->gpio_cb, max17048_int_callback,
			   BIT(drv_cfg->int_gpio.pin));

	rc = gpio_add_callback_dt(&drv_cfg->int_gpio, (struct gpio_callback *)&drv_data->gpio_cb);

	if (rc < 0) {
		LOG_ERR("Failed to initialize interrupt on %s pin %d", drv_cfg->int_gpio.port->name,
			drv_cfg->int_gpio.pin);
		return -EIO;
	}

	if (gpio_pin_interrupt_configure_dt(&drv_cfg->int_gpio, GPIO_INT_EDGE_TO_ACTIVE) < 0) {
		LOG_ERR("Failed to configure interrupt on %s pin %d", drv_cfg->int_gpio.port->name,
			drv_cfg->int_gpio.pin);
		return -EIO;
	}

	return 0;
}

/* Internal functions */
static void max17048_int_callback(const struct device *port, struct gpio_callback *cb, uint32_t pin)
{
	const struct max17048_data *drv_data = CONTAINER_OF(cb, struct max17048_data, gpio_cb);

	/* Temporarily disables interrupt until CONFIG.ALRT is cleared */
	gpio_pin_interrupt_configure(port, pin, GPIO_INT_DISABLE);
	k_work_submit((struct k_work *)&drv_data->work);
}

static void max17048_process_interrupt(const struct device *dev)
{
	const struct max17048_data *drv_data = dev->data;
	uint16_t status;

	max17048_read_register(dev, REGISTER_STATUS, &status);

	if (status & MAX17048_STATUS_VH) {
		/* Disables over-voltage alarm by setting maximum value. */
		max17048_overvoltage_threshold_set(dev, MAX17048_OVERVOLTAGE_THRESHOLD_MAX);
		if (drv_data->trigger_overvoltage_handler) {
			drv_data->trigger_overvoltage_handler(dev, MAX17048_TRIGGER_OVERVOLTAGE);
		} else {
			LOG_WRN("Over-voltage was detected. But, no handler found.");
		}
	}
	if (status & MAX17048_STATUS_VL) {
		/* Disables under-voltage alarm by setting minimum value. */
		max17048_undervoltage_threshold_set(dev, 0);
		if (drv_data->trigger_undervoltage_handler) {
			drv_data->trigger_undervoltage_handler(dev, MAX17048_TRIGGER_UNDERVOLTAGE);
		} else {
			LOG_WRN("Under-voltage was detected. But, no handler found.");
		}
	}
	if (status & MAX17048_STATUS_HD) {
		if (drv_data->trigger_low_soc_handler) {
			drv_data->trigger_low_soc_handler(dev, MAX17048_TRIGGER_LOW_SOC);
		} else {
			LOG_WRN("Low SoC was detected. But, no handler found.");
		}
	}
}

static void max17048_int_work(struct k_work *work)
{
	const struct max17048_data *drv_data =
		(struct max17048_data *)CONTAINER_OF(work, struct max17048_data, work);
	const struct device *dev = drv_data->dev;
	const struct max17048_config *drv_cfg = dev->config;

	max17048_process_interrupt(dev);
	max17048_clear_alert(dev);
	/* Sets all bits in STATUS register to 0 */
	i2c_reg_write_byte_dt(&drv_cfg->i2c, REGISTER_STATUS, 0x00);
	gpio_pin_interrupt_configure_dt(&drv_cfg->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
}

static int max17048_clear_alert(const struct device *dev)
{
	return max17048_update_register(dev, REGISTER_CONFIG, MAX17048_CONFIG_ALRT, 0x0000);
}

static int max17048_read_register(const struct device *dev, uint8_t registerId, uint16_t *response)
{
	uint8_t max17048_buffer[2];
	const struct max17048_config *cfg = dev->config;
	int rc = i2c_write_read_dt(&cfg->i2c, &registerId, sizeof(registerId), max17048_buffer,
				   sizeof(max17048_buffer));

	if (rc != 0) {
		LOG_ERR("Unable to read register, error %d", rc);
		return rc;
	}

	*response = sys_get_be16(max17048_buffer);
	return 0;
}

static int max17048_update_register(const struct device *dev, uint8_t reg, uint16_t mask,
				    uint16_t val)
{
	int ret;
	const struct max17048_config *drv_config = dev->config;
	uint16_t old_val, verification;
	uint8_t buf[2];

	max17048_read_register(dev, reg, &old_val);
	val = (old_val & ~mask) | (val & mask);
	buf[0] = (uint8_t)(val >> 8);
	buf[1] = (uint8_t)(val);
	ret = i2c_burst_write_dt(&drv_config->i2c, reg, buf, 2);
	max17048_read_register(dev, reg, &verification);
	if (verification != val) {
		return -1;
	}
	return ret;
}

static int max17048_undervoltage_threshold_set(const struct device *dev, const uint16_t voltage)
{
	const struct max17048_config *drv_config = dev->config;
	uint16_t converted_val;
	uint8_t new_reg_val;

	if (dev == NULL || voltage > MAX17048_OVERVOLTAGE_THRESHOLD_MAX) {
		return -EINVAL;
	}

	/* 1LSB = 20mV */
	converted_val = voltage / 20;
	new_reg_val = converted_val;

	return i2c_reg_write_byte_dt(&drv_config->i2c, REGISTER_VALRT, converted_val);
}

static int max17048_overvoltage_threshold_set(const struct device *dev, const uint16_t voltage)
{
	const struct max17048_config *drv_config = dev->config;
	uint16_t converted_val;
	uint8_t new_reg_val;

	if (dev == NULL || voltage > MAX17048_OVERVOLTAGE_THRESHOLD_MAX) {
		return -EINVAL;
	}

	/* 1LSB = 20mV */
	converted_val = voltage / 20;
	new_reg_val = converted_val;

	return i2c_reg_write_byte_dt(&drv_config->i2c, REGISTER_VALRT + 1, converted_val);
}

static int max17048_low_soc_threshold_set(const struct device *dev, const uint8_t soc)
{
	uint8_t reg_val;

	if (dev == NULL || soc > MAX17048_SOC_THRESHOLD_MAX || soc < 1) {
		return -EINVAL;
	}

	reg_val = MAX17048_SOC_THRESHOLD_MAX - soc;

	return max17048_update_register(dev, REGISTER_CONFIG, 0x001F, reg_val);
}
