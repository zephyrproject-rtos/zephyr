/**
 * Copyright (c) 2022 Ithinx GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_pcf8574

#include <zephyr/drivers/gpio/gpio_utils.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pcf8574, CONFIG_GPIO_LOG_LEVEL);

struct pcf8574_pins_cfg {
	uint8_t configured_as_outputs; /* 0 for input, 1 for output */
	uint8_t outputs_state;
};

/** Runtime driver data of the pcf8574*/
struct pcf8574_drv_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	struct pcf8574_pins_cfg pins_cfg;
	sys_slist_t callbacks;
	struct k_sem lock;
	struct k_work work;
	const struct device *dev;
	struct gpio_callback int_gpio_cb;
	uint8_t input_port_last;
};

/** Configuration data*/
struct pcf8574_drv_cfg {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec gpio_int;
};

/**
 * @brief Reads the value of the pins from pcf8574 respectively from a connected device.
 *
 * @param dev Pointer to the device structure of the driver instance.
 * @param value Pointer to the input value. It contains the received Byte.
 *
 * @retval 0 If successful.
 * @retval Negative value for error code.
 */
static int pcf8574_process_input(const struct device *dev, gpio_port_value_t *value)
{
	const struct pcf8574_drv_cfg *drv_cfg = dev->config;
	struct pcf8574_drv_data *drv_data = dev->data;
	int rc = 0;
	uint8_t rx_buf;

	rc = i2c_read_dt(&drv_cfg->i2c, &rx_buf, sizeof(rx_buf));
	if (rc != 0) {
		LOG_ERR("%s: failed to read from device: %d", dev->name, rc);
		return -EIO;
	}

	if (value) {
		*value = rx_buf;
	}

	drv_data->input_port_last = rx_buf;

	return rc;
}

/** Register the read-task as work*/
static void pcf8574_work_handler(struct k_work *work)
{
	struct pcf8574_drv_data *drv_data = CONTAINER_OF(work, struct pcf8574_drv_data, work);

	k_sem_take(&drv_data->lock, K_FOREVER);

	uint32_t changed_pins;
	uint8_t input_port_last_temp = drv_data->input_port_last;
	int rc = pcf8574_process_input(drv_data->dev, &changed_pins);

	if (rc) {
		LOG_ERR("Failed to read interrupt sources: %d", rc);
	}
	k_sem_give(&drv_data->lock);
	if (input_port_last_temp != (uint8_t)changed_pins && !rc) {

		/** Find changed bits*/
		changed_pins ^= input_port_last_temp;
		gpio_fire_callbacks(&drv_data->callbacks, drv_data->dev, changed_pins);
	}
}

/** Callback for interrupt through some level changes on pcf8574 pins*/
static void pcf8574_int_gpio_handler(const struct device *dev, struct gpio_callback *gpio_cb,
				     uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pins);

	struct pcf8574_drv_data *drv_data =
		CONTAINER_OF(gpio_cb, struct pcf8574_drv_data, int_gpio_cb);

	k_work_submit(&drv_data->work);
}

/**
 * @brief This function reads a value from the connected device
 *
 * @param dev Pointer to the device structure of a port.
 * @param value Pointer to a variable where pin values will be stored.
 *
 * @retval 0 If successful.
 * @retval Negative value for error code.
 */
static int pcf8574_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	struct pcf8574_drv_data *drv_data = dev->data;
	int rc;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	if ((~drv_data->pins_cfg.configured_as_outputs & (uint8_t)*value) != (uint8_t)*value) {
		LOG_ERR("Pin(s) is/are configured as output which should be input.");
		return -EOPNOTSUPP;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	/**
	 * Reading of the input port also clears the generated interrupt,
	 * thus the configured callbacks must be fired also here if needed.
	 */
	rc = pcf8574_process_input(dev, value);

	k_sem_give(&drv_data->lock);

	return rc;
}

/**
 * @brief This function realizes the write connection to the i2c device.
 *
 * @param dev A pointer to the device structure
 * @param mask A mask of bits to set some bits to LOW or HIGH
 * @param value The value which is written via i2c to the pfc8574's output pins
 * @param toggle A way to toggle some bits with xor
 *
 * @retval 0 If successful.
 * @retval Negative value for error code.
 */
static int pcf8574_port_set_raw(const struct device *dev, uint8_t mask, uint8_t value,
				uint8_t toggle)
{
	const struct pcf8574_drv_cfg *drv_cfg = dev->config;
	struct pcf8574_drv_data *drv_data = dev->data;
	int rc = 0;
	uint8_t tx_buf;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	if ((drv_data->pins_cfg.configured_as_outputs & value) != value) {
		LOG_ERR("Pin(s) is/are configured as input which should be output.");
		return -EOPNOTSUPP;
	}

	tx_buf = (drv_data->pins_cfg.outputs_state & ~mask);
	tx_buf |= (value & mask);
	tx_buf ^= toggle;

	rc = i2c_write_dt(&drv_cfg->i2c, &tx_buf, sizeof(tx_buf));

	if (rc != 0) {
		LOG_ERR("%s: failed to write output port: %d", dev->name, rc);
		return -EIO;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);
	drv_data->pins_cfg.outputs_state = tx_buf;
	k_sem_give(&drv_data->lock);

	return 0;
}

/**
 * @brief This function fills a dummy because the pfc8574 has no pins to configure.
 * You can use it to set some pins permanent to HIGH or LOW until reset. It uses the port_set_raw
 * function to set the pins of pcf8574 directly.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pin The bit in the io register which is set to high
 * @param flags Flags like the GPIO direction or the state
 *
 * @retval 0 If successful.
 * @retval Negative value for error.
 */
static int pcf8574_pin_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	struct pcf8574_drv_data *drv_data = dev->data;
	int ret = 0;
	uint8_t temp_pins = drv_data->pins_cfg.outputs_state;
	uint8_t temp_outputs = drv_data->pins_cfg.configured_as_outputs;

	if (flags & (GPIO_PULL_UP | GPIO_PULL_DOWN | GPIO_DISCONNECTED | GPIO_SINGLE_ENDED)) {
		return -ENOTSUP;
	}
	if (flags & GPIO_INPUT) {
		temp_outputs &= ~BIT(pin);
		temp_pins &= ~(1 << pin);
	} else if (flags & GPIO_OUTPUT) {
		drv_data->pins_cfg.configured_as_outputs |= BIT(pin);
		temp_outputs = drv_data->pins_cfg.configured_as_outputs;
	}
	if (flags & GPIO_OUTPUT_INIT_HIGH) {
		temp_pins |= (1 << pin);
	}
	if (flags & GPIO_OUTPUT_INIT_LOW) {
		temp_pins &= ~(1 << pin);
	}

	ret = pcf8574_port_set_raw(dev, drv_data->pins_cfg.configured_as_outputs, temp_pins, 0);

	if (ret == 0) {
		k_sem_take(&drv_data->lock, K_FOREVER);
		drv_data->pins_cfg.outputs_state = temp_pins;
		drv_data->pins_cfg.configured_as_outputs = temp_outputs;
		k_sem_give(&drv_data->lock);
	}

	return ret;
}

/**
 * @brief Sets a value to the pins of pcf8574
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param mask The bit mask which bits should be set
 * @param value	The value which should be set
 *
 * @retval 0 If successful.
 * @retval Negative value for error.
 */
static int pcf8574_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
				       gpio_port_value_t value)
{
	return pcf8574_port_set_raw(dev, (uint8_t)mask, (uint8_t)value, 0);
}

/**
 * @brief Sets some output pins of the pcf8574
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pins The pin(s) which will be set in a range from 0 to 7
 *
 * @retval 0 If successful.
 * @retval Negative value for error.
 */
static int pcf8574_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	return pcf8574_port_set_raw(dev, (uint8_t)pins, (uint8_t)pins, 0);
}

/**
 * @brief clear some bits
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pins Pins which will be cleared
 *
 * @retval 0 If successful.
 * @retval Negative value for error.
 */
static int pcf8574_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	return pcf8574_port_set_raw(dev, (uint8_t)pins, 0, 0);
}

/**
 * @brief Toggle some bits
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pins Pins which will be toggled
 *
 * @retval 0 If successful.
 * @retval Negative value for error.
 */
static int pcf8574_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	return pcf8574_port_set_raw(dev, 0, 0, (uint8_t)pins);
}

/* Each pin gives an interrupt at pcf8574. In this function the configuration is checked. */
static int pcf8574_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
					   enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	const struct pcf8574_drv_cfg *drv_cfg = dev->config;

	if (!drv_cfg->gpio_int.port) {
		return -ENOTSUP;
	}

	/* This device supports only edge-triggered interrupts. */
	if (mode == GPIO_INT_MODE_LEVEL) {
		return -ENOTSUP;
	}

	return 0;
}

/** Register the callback in the callback list */
static int pcf8574_manage_callback(const struct device *dev, struct gpio_callback *callback,
				   bool set)
{
	struct pcf8574_drv_data *drv_data = dev->data;

	return gpio_manage_callback(&drv_data->callbacks, callback, set);
}

/** Initialize the pcf8574 */
static int pcf8574_init(const struct device *dev)
{
	const struct pcf8574_drv_cfg *drv_cfg = dev->config;
	struct pcf8574_drv_data *drv_data = dev->data;
	int rc;

	if (!device_is_ready(drv_cfg->i2c.bus)) {
		LOG_ERR("%s is not ready", drv_cfg->i2c.bus->name);
		return -ENODEV;
	}

	/* If the INT line is available, configure the callback for it. */
	if (drv_cfg->gpio_int.port) {
		if (!gpio_is_ready_dt(&drv_cfg->gpio_int)) {
			LOG_ERR("Port is not ready");
			return -ENODEV;
		}

		rc = gpio_pin_configure_dt(&drv_cfg->gpio_int, GPIO_INPUT);
		if (rc != 0) {
			LOG_ERR("%s: failed to configure INT line: %d", dev->name, rc);
			return -EIO;
		}

		rc = gpio_pin_interrupt_configure_dt(&drv_cfg->gpio_int, GPIO_INT_EDGE_TO_ACTIVE);
		if (rc != 0) {
			LOG_ERR("%s: failed to configure INT interrupt: %d", dev->name, rc);
			return -EIO;
		}

		gpio_init_callback(&drv_data->int_gpio_cb, pcf8574_int_gpio_handler,
				   BIT(drv_cfg->gpio_int.pin));
		rc = gpio_add_callback(drv_cfg->gpio_int.port, &drv_data->int_gpio_cb);
		if (rc != 0) {
			LOG_ERR("%s: failed to add INT callback: %d", dev->name, rc);
			return -EIO;
		}
	}

	return 0;
}

/** Realizes the functions of gpio.h for pcf8574*/
static const struct gpio_driver_api pcf8574_drv_api = {
	.pin_configure = pcf8574_pin_configure,
	.port_get_raw = pcf8574_port_get_raw,
	.port_set_masked_raw = pcf8574_port_set_masked_raw,
	.port_set_bits_raw = pcf8574_port_set_bits_raw,
	.port_clear_bits_raw = pcf8574_port_clear_bits_raw,
	.port_toggle_bits = pcf8574_port_toggle_bits,
	.pin_interrupt_configure = pcf8574_pin_interrupt_configure,
	.manage_callback = pcf8574_manage_callback,
};

#define GPIO_PCF8574_INST(idx)                                                                     \
	static const struct pcf8574_drv_cfg pcf8574_cfg##idx = {                                   \
		.common =                                                                          \
			{                                                                          \
				.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(idx),             \
			},                                                                         \
		.gpio_int = GPIO_DT_SPEC_INST_GET_OR(idx, int_gpios, {0}),                         \
		.i2c = I2C_DT_SPEC_INST_GET(idx),                                                  \
	};                                                                                         \
	static struct pcf8574_drv_data pcf8574_data##idx = {                                       \
		.lock = Z_SEM_INITIALIZER(pcf8574_data##idx.lock, 1, 1),                           \
		.work = Z_WORK_INITIALIZER(pcf8574_work_handler),                                  \
		.dev = DEVICE_DT_INST_GET(idx),                                                    \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(idx, pcf8574_init, NULL, &pcf8574_data##idx, &pcf8574_cfg##idx,      \
			      POST_KERNEL, CONFIG_GPIO_PCF8574_INIT_PRIORITY, &pcf8574_drv_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_PCF8574_INST);
