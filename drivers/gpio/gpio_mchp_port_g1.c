/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file gpio_mchp_port_g1.c
 * @brief GPIO driver implementation for Microchip devices.
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <soc.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(gpio_mchp_port_g1, CONFIG_GPIO_LOG_LEVEL);

/******************************************************************************
 * @brief Devicetree definitions
 *****************************************************************************/
#define DT_DRV_COMPAT microchip_port_g1_gpio

/******************************************************************************
 * @brief Data type definitions
 *****************************************************************************/
/**
 * @brief Configuration structure for MCHP GPIO driver
 */
struct gpio_mchp_config {
	/* Common GPIO driver configuration */
	struct gpio_driver_config common;

	/* Pointer to port group registers */
	port_group_registers_t *gpio_regs;
};

/**
 * @brief Runtime data structure for MCHP GPIO driver
 */
struct gpio_mchp_data {
	/* Common GPIO driver data */
	struct gpio_driver_data common;

	/* Pointer to device structure */
	const struct device *dev;
};

/******************************************************************************
 * @brief Helper functions
 *****************************************************************************/
/**
 * Get the current value of the GPIO port.
 */
static inline void gpio_port_get_val(port_group_registers_t *regs, uint32_t *port_val)
{
	*port_val = regs->PORT_IN;
}

/**
 * Set the GPIO port output value with a mask.
 */
static inline void gpio_port_outset_masked(port_group_registers_t *regs, uint32_t mask,
					   uint32_t value)
{
	regs->PORT_OUT = (regs->PORT_OUT & ~(mask)) | (value & mask);
}

/**
 * Enable input on a specific GPIO pin.
 */
static inline void gpio_enable_input(port_group_registers_t *regs, const uint32_t pin)
{
	regs->PORT_PINCFG[pin] |= PORT_PINCFG_INEN(1);
}

/**
 * Set a specific GPIO pin to high.
 */
static inline void gpio_outset(port_group_registers_t *regs, const uint32_t pin)
{
	regs->PORT_OUTSET = BIT(pin);
}

/**
 * Set a specific GPIO pin to low.
 */
static inline void gpio_outclr(port_group_registers_t *regs, const uint32_t pin)
{
	regs->PORT_OUTCLR = BIT(pin);
}

/**
 * Set the direction of a specific GPIO pin to output.
 */
static inline void gpio_set_dir_output(port_group_registers_t *regs, const uint32_t pin)
{
	regs->PORT_PINCFG[pin] &= ~PORT_PINCFG_INEN(1);
	regs->PORT_DIRSET = BIT(pin);
}

/**
 * Set the direction of a specific GPIO pin to input.
 */
static inline void gpio_set_dir_input(port_group_registers_t *regs, const uint32_t pin)
{
	gpio_enable_input(regs, pin);
	regs->PORT_DIRCLR = BIT(pin);
}

/**
 * Set the direction of a specific GPIO pin to input output.
 */
static inline void gpio_set_dir_input_output(port_group_registers_t *regs, const uint32_t pin)
{
	gpio_enable_input(regs, pin);
	regs->PORT_DIRSET = BIT(pin);
}

/**
 * Enable pull-up/pull-down resistor on a specific GPIO pin.
 */
static inline void gpio_enable_pullup(port_group_registers_t *regs, const uint32_t pin)
{
	regs->PORT_PINCFG[pin] |= PORT_PINCFG_PULLEN(1);
}

/**
 * Check if pull-up/pull-down resistor is enabled on a specific GPIO pin.
 */
static inline bool gpio_is_pullup(port_group_registers_t *regs, const uint32_t pin)
{
	bool is_pull_enabled = false;

	if ((regs->PORT_PINCFG[pin] & PORT_PINCFG_PULLEN(1)) != 0) {
		is_pull_enabled = true;
	}

	return is_pull_enabled;
}

/**
 * Set multiple GPIO pins to high.
 */
static inline void gpio_port_set_pins_high(port_group_registers_t *regs, const uint32_t pins)
{
	regs->PORT_OUTSET = pins;
}

/**
 * Set multiple GPIO pins to low.
 */
static inline void gpio_port_set_pins_low(port_group_registers_t *regs, const uint32_t pins)
{
	regs->PORT_OUTCLR = pins;
}

/**
 * Toggle multiple GPIO pins.
 */
static inline void gpio_port_toggle_pins(port_group_registers_t *regs, const uint32_t pins)
{
	regs->PORT_OUTTGL = pins;
}

/**
 * Check if a specific GPIO pin is set to high.
 */
static inline bool gpio_is_pin_high(port_group_registers_t *regs, const uint32_t pin)
{
	bool is_output_high = false;

	if ((regs->PORT_OUT & BIT(pin)) != 0) {
		is_output_high = true;
	}

	return is_output_high;
}

/**
 * Get the direction configuration of the GPIO port.
 */
static inline uint32_t gpio_port_get_dir(port_group_registers_t *regs)
{
	return regs->PORT_DIR;
}

/**
 * Check if a specific GPIO pin is configured as output.
 */
static inline bool gpio_is_pin_output(port_group_registers_t *regs, const uint32_t pin)
{
	bool is_output = false;

	if ((gpio_port_get_dir(regs) & BIT(pin)) != 0) {
		is_output = true;
	}

	return is_output;
}

/**
 * Get the pins configured as input.
 */
static inline uint32_t gpio_port_get_input_pins(port_group_registers_t *regs)
{
	uint32_t pin_id;
	uint32_t port_input = 0;

	for (pin_id = 0; pin_id < 32; pin_id++) {
		if (((regs->PORT_PINCFG[pin_id] & PORT_PINCFG_INEN(1)) != 0)) {
			port_input |= BIT(pin_id);
		}
	}

	return port_input;
}

/**
 * Get the pins configured as output.
 */
static inline uint32_t gpio_port_get_output_pins(port_group_registers_t *regs)
{
	uint32_t port_output = 0;

	port_output = gpio_port_get_dir(regs);

	return port_output;
}

/**
 * Disconnect the GPIO pin.
 */
static inline void gpio_disconnect(port_group_registers_t *regs, const uint32_t pin)
{
	/* Disable pull-up/pull-down resistor on a specific GPIO pin. */
	regs->PORT_PINCFG[pin] &= ~PORT_PINCFG_PULLEN(1);

	/* Disable input on a specific GPIO pin. */
	regs->PORT_PINCFG[pin] &= ~PORT_PINCFG_INEN(1);
	regs->PORT_DIRCLR = BIT(pin);
}

/**
 * Configure GPIO pin as input
 */
static int gpio_configure_input(port_group_registers_t *gpio_reg, gpio_pin_t pin,
				gpio_flags_t flags)
{
	/* Configure the pin as input if requested */
	gpio_set_dir_input(gpio_reg, pin);

	/* Configure pull-up or pull-down if requested */
	if ((flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) != 0) {
		gpio_enable_pullup(gpio_reg, pin);

		if (flags & GPIO_PULL_UP) {
			gpio_outset(gpio_reg, pin);
		} else {
			gpio_outclr(gpio_reg, pin);
		}
	}

	return 0;
}

/**
 * Configure GPIO pin as output
 */
static int gpio_configure_output(port_group_registers_t *gpio_reg, gpio_pin_t pin,
				 gpio_flags_t flags)
{
	int retval = 0;

	/* Output is incompatible with pull-up or pull-down */
	if ((flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) != 0) {
		retval = -ENOTSUP;
	} else {
		/* Set initial output state if specified */
		if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
			gpio_outclr(gpio_reg, pin);
		} else if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
			gpio_outset(gpio_reg, pin);
		} else {
			/* No init value requested */
			retval = 0;
		}
	}

	/* Set the pin as output */
	gpio_set_dir_output(gpio_reg, pin);

	return retval;
}

/******************************************************************************
 * @brief API functions
 *****************************************************************************/
/**
 * @brief Configure a GPIO pin
 *
 * @param dev Pointer to the device structure
 * @param pin Pin number to configure
 * @param flags Configuration flags
 * @retval 0 on success
 * @retval ENOTSUP If any of the configuration options is not supported
 */
static int gpio_mchp_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_mchp_config *config = dev->config;
	port_group_registers_t *gpio_reg = config->gpio_regs;
	gpio_flags_t io_flags = flags & (GPIO_INPUT | GPIO_OUTPUT);
	int retval = 0;

	/* Disable the pinmux functionality as its gpio */
	gpio_reg->PORT_PINCFG[pin] &= (uint8_t)~PORT_PINCFG_PMUXEN_Msk;

	if (io_flags == GPIO_DISCONNECTED) {

		/* Disconnect the gpio */
		gpio_disconnect(gpio_reg, pin);
	}

	/* Check for single-ended mode configuration */
	else if (flags & GPIO_SINGLE_ENDED) {
		retval = -ENOTSUP;
	}

	/* Configure the pin as input and output if requested */
	else if (io_flags == (GPIO_INPUT | GPIO_OUTPUT)) {
		gpio_set_dir_input_output(gpio_reg, pin);

		/* Set initial output state if specified */
		if (flags & GPIO_OUTPUT_INIT_LOW) {
			gpio_outclr(gpio_reg, pin);
		} else if (flags & GPIO_OUTPUT_INIT_HIGH) {
			gpio_outset(gpio_reg, pin);
		} else {
			/* No init value requested */
			retval = 0;
		}
	} else if (flags & GPIO_INPUT) {
		retval = gpio_configure_input(gpio_reg, pin, flags);
	} else if (flags & GPIO_OUTPUT) {
		retval = gpio_configure_output(gpio_reg, pin, flags);
	} else {
		/* Catch-all for unexpected flag combinations */
		retval = -ENOTSUP;
	}

	return retval;
}

/**
 * @brief Get raw port value
 *
 * @param dev Pointer to the device structure
 * @param value Pointer to store the port value
 * @retval 0 on success
 */
static int gpio_mchp_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	const struct gpio_mchp_config *config = dev->config;
	port_group_registers_t *gpio_reg = config->gpio_regs;

	/* Read the input value of the port */
	gpio_port_get_val(gpio_reg, value);

	return 0;
}

/**
 * @brief Set masked raw port value
 *
 * @param dev Pointer to the device structure
 * @param mask Mask of pins to set
 * @param value Value to set
 * @retval 0 on success
 */
static int gpio_mchp_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
					 gpio_port_value_t value)
{
	const struct gpio_mchp_config *config = dev->config;
	port_group_registers_t *gpio_reg = config->gpio_regs;

	/* Set the output value of the port with the specified mask */
	gpio_port_outset_masked(gpio_reg, mask, value);

	return 0;
}

/**
 * @brief Set bits in raw port value
 *
 * @param dev Pointer to the device structure
 * @param pins Pins to set
 * @retval 0 on success
 */
static int gpio_mchp_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_mchp_config *config = dev->config;
	port_group_registers_t *gpio_reg = config->gpio_regs;

	/* Set the specified pins in the output register */
	gpio_port_set_pins_high(gpio_reg, pins);

	return 0;
}

/**
 * @brief Clear bits in raw port value
 *
 * @param dev Pointer to the device structure
 * @param pins Pins to clear
 * @retval 0 on success
 */
static int gpio_mchp_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_mchp_config *config = dev->config;
	port_group_registers_t *gpio_reg = config->gpio_regs;

	/* Clear the specified pins in the output register */
	gpio_port_set_pins_low(gpio_reg, pins);

	return 0;
}

/**
 * @brief Toggle bits in raw port value
 *
 * @param dev Pointer to the device structure
 * @param pins Pins to toggle
 * @retval 0 on success
 */
static int gpio_mchp_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_mchp_config *config = dev->config;
	port_group_registers_t *gpio_reg = config->gpio_regs;

	/* Toggle the specified pins in the output register */
	gpio_port_toggle_pins(gpio_reg, pins);

	return 0;
}

#ifdef CONFIG_GPIO_GET_CONFIG
/**
 * @brief Get the configuration of a specific GPIO pin
 *
 * This function retrieves the configuration flags of a specified GPIO pin.
 *
 * @param dev Pointer to the device structure
 * @param pin The pin number to get the configuration for
 * @param out_flags Pointer to store the retrieved configuration flags
 * @retval 0 on success
 */
static int gpio_mchp_pin_get_config(const struct device *dev, gpio_pin_t pin,
				    gpio_flags_t *out_flags)
{
	const struct gpio_mchp_config *config = dev->config;
	port_group_registers_t *gpio_reg = config->gpio_regs;
	struct gpio_mchp_data *data = dev->data;
	gpio_flags_t flags = 0;

	/* flag to check if the pin is configured as an output */
	bool is_output = gpio_is_pin_output(gpio_reg, pin);

	/* flag to check if pull-up or pull-down resistors are enabled */
	bool is_pull_enabled = gpio_is_pullup(gpio_reg, pin);

	/* flag to check if the output is set to high */
	bool is_output_high = gpio_is_pin_high(gpio_reg, pin);

	/* Check if the pin is configured as active low */
	bool is_active_low = data->common.invert & (gpio_port_pins_t)BIT(pin);

	/* Check if the pin is configured as an output */
	if (is_output) {
		flags |= GPIO_OUTPUT;
		flags |= is_output_high ? GPIO_OUTPUT_INIT_HIGH : GPIO_OUTPUT_INIT_LOW;
	} else {
		flags |= GPIO_INPUT;

		/* Check if pull-up or pull-down resistors are enabled */
		if (is_pull_enabled) {
			flags |= is_output_high ? GPIO_PULL_UP : GPIO_PULL_DOWN;
		}
	}

	/* Check if the pin is configured as active low */
	flags |= is_active_low ? GPIO_ACTIVE_LOW : GPIO_ACTIVE_HIGH;

	*out_flags = flags;

	return 0;
}
#endif /* CONFIG_GPIO_GET_CONFIG */

#ifdef CONFIG_GPIO_GET_DIRECTION
/**
 * @brief Get the direction of GPIO pins in a port.
 *
 * This function retrieves the direction (input or output) of the specified GPIO pins in a port.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param map Bitmask representing the pins to check.
 * @param inputs Pointer to store the bitmask of input pins (can be NULL).
 * @param outputs Pointer to store the bitmask of output pins (can be NULL).
 *
 * @retval 0 on success
 */
static int gpio_mchp_port_get_direction(const struct device *dev, gpio_port_pins_t map,
					gpio_port_pins_t *inputs, gpio_port_pins_t *outputs)
{
	/* Get the device configuration */
	const struct gpio_mchp_config *config = dev->config;

	/* Get the GPIO register base address */
	port_group_registers_t *gpio_reg = config->gpio_regs;

	map &= config->common.port_pin_mask;

	if (inputs != NULL) {
		/* Get the input pins */
		*inputs = map & (gpio_port_get_input_pins(gpio_reg));
	}

	if (outputs != NULL) {
		/* Get the output pins */
		*outputs = map & (gpio_port_get_output_pins(gpio_reg));
	}

	return 0;
}

#endif /* CONFIG_GPIO_GET_DIRECTION */

/******************************************************************************
 * @brief Zephyr driver instance creation
 *****************************************************************************/
/**
 * @brief GPIO driver API structure
 */
static DEVICE_API(gpio, gpio_mchp_api) = {
	.pin_configure = gpio_mchp_configure,
	.port_get_raw = gpio_mchp_port_get_raw,
	.port_set_masked_raw = gpio_mchp_port_set_masked_raw,
	.port_set_bits_raw = gpio_mchp_port_set_bits_raw,
	.port_clear_bits_raw = gpio_mchp_port_clear_bits_raw,
	.port_toggle_bits = gpio_mchp_port_toggle_bits,
#ifdef CONFIG_GPIO_GET_CONFIG
	.pin_get_config = gpio_mchp_pin_get_config,
#endif
#ifdef CONFIG_GPIO_GET_DIRECTION
	.port_get_direction = gpio_mchp_port_get_direction,
#endif
};

/**
 * @brief Initialize the GPIO driver
 *
 * @param dev Pointer to the device structure
 * @retval 0 on success
 */
static int gpio_mchp_init(const struct device *dev)
{
	return 0;
}

/* Define GPIO port configuration macro */
/* clang-format off */
#define GPIO_PORT_CONFIG(idx)                                                                      \
	static const struct gpio_mchp_config gpio_mchp_config_##idx = {                            \
		.common = {                                                                        \
				.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(idx),             \
		},                                                                                 \
		.gpio_regs = (port_group_registers_t *)DT_INST_REG_ADDR(idx),                      \
	};                                                                                         \
	static struct gpio_mchp_data gpio_mchp_data_##idx;                                         \
	DEVICE_DT_DEFINE(DT_INST(idx, DT_DRV_COMPAT), gpio_mchp_init, NULL, &gpio_mchp_data_##idx, \
			 &gpio_mchp_config_##idx, PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY,         \
			 &gpio_mchp_api);
/* clang-format on */

/* Use DT_INST_FOREACH_STATUS_OKAY to iterate over GPIO instances */
DT_INST_FOREACH_STATUS_OKAY(GPIO_PORT_CONFIG)
