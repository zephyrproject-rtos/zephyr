/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file gpio_mchp_g2.c
 * @brief GPIO driver implementation for Microchip devices (gpio_g2).
 */

#define DT_DRV_COMPAT microchip_gpio_g2

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(gpio_mchp_g2, CONFIG_GPIO_LOG_LEVEL);

#define PINS_MASK 0xFU

/* Pins which supports Analog pins */
#define AIN_PIN_MASK_PORTA 0x4008
#define AIN_PIN_MASK_PORTB 0xCCFF
#define AIN_PIN_MASK_PORTC 0x0080
#define AIN_PIN_MASK_PORTD 0x00FC
#define AIN_PIN_MASK_PORTE 0x0060

/**
 * @brief Configuration structure for MCHP GPIO driver
 */
struct gpio_mchp_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	gpio_registers_t *regs;
	/* Function pointer for configuring GPIO interrupts. */
	void (*irq_config)(void);
};

/**
 * @brief Runtime data structure for MCHP GPIO driver
 */
struct gpio_mchp_data {
	/* Common GPIO driver data */
	struct gpio_driver_data common;
	/* gpio_driver_data needs to be first */

	/* provided by gpio_utils
	 * callbacks are stored here for each pins
	 */
	sys_slist_t callbacks;
};

/**
 * @brief Configure a GPIO pin
 *
 * @param dev Pointer to the device structure
 * @param pin Pin number to configure
 * @param flags Configuration flags
 * @retval 0 on success
 * @retval ENOTSUP If any of the configuration options is not supported
 */
static int gpio_mchp_config(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_mchp_config *config = dev->config;
	gpio_registers_t *regs = config->regs;
	int ret = 0;
	uint32_t mask, io_flags;

	io_flags = flags & (GPIO_INPUT | GPIO_OUTPUT);
	if ((io_flags == GPIO_DISCONNECTED) || (io_flags == (GPIO_INPUT | GPIO_OUTPUT))) {
		ret = -ENOTSUP;
	} else {
		mask = BIT(pin & PINS_MASK);

		/* If any JTAG pins are being repurposed, disable JTAG globally */
		CFG_REGS->CFG_CFGCON0CLR = CFG_CFGCON0_JTAGEN_Msk;

		/* Digital Mode Enable if pin can be analog */
		switch ((uintptr_t)regs) {
#ifdef AIN_PIN_MASK_PORTA
		case (uintptr_t)GPIOA_REGS:
			regs->GPIO_ANSELCLR = mask & AIN_PIN_MASK_PORTA;
			break;
#endif
#ifdef AIN_PIN_MASK_PORTB
		case (uintptr_t)GPIOB_REGS:
			regs->GPIO_ANSELCLR = mask & AIN_PIN_MASK_PORTB;
			break;
#endif
#ifdef AIN_PIN_MASK_PORTC
		case (uintptr_t)GPIOC_REGS:
			regs->GPIO_ANSELCLR = mask & AIN_PIN_MASK_PORTC;
			break;
#endif
#ifdef AIN_PIN_MASK_PORTD
		case (uintptr_t)GPIOD_REGS:
			regs->GPIO_ANSELCLR = mask & AIN_PIN_MASK_PORTD;
			break;
#endif
#ifdef AIN_PIN_MASK_PORTE
		case (uintptr_t)GPIOE_REGS:
			regs->GPIO_ANSELCLR = mask & AIN_PIN_MASK_PORTE;
			break;
#endif
		}

		if ((flags & GPIO_OUTPUT) != 0) {
			/* Output is Open drain */
			if ((flags & GPIO_LINE_OPEN_DRAIN) != 0) {
				regs->GPIO_ODCSET = mask;
			} else {
				regs->GPIO_ODCCLR = mask;
			}

			if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
				regs->GPIO_LATCLR = mask;
			} else if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
				regs->GPIO_LATSET = mask;
			}

			regs->GPIO_CNPUCLR = mask;
			regs->GPIO_CNPDCLR = mask;

			regs->GPIO_TRISCLR = mask;
		} else {

			/* Configure pull-up or pull-down if requested */
			if ((flags & GPIO_PULL_UP) != 0) {
				regs->GPIO_CNPUSET = mask;
			} else {
				regs->GPIO_CNPUCLR = mask;
			}

			if ((flags & GPIO_PULL_DOWN) != 0) {
				regs->GPIO_CNPDSET = mask;
			} else {
				regs->GPIO_CNPDCLR = mask;
			}

			regs->GPIO_LATCLR = mask;

			/* input configuration*/
			regs->GPIO_TRISSET = mask;
		}
	}

	return ret;
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

	*value = config->regs->GPIO_PORT;

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

	config->regs->GPIO_LAT = (config->regs->GPIO_LAT & ~mask) | (value & mask);

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

	config->regs->GPIO_LATSET = pins;

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

	config->regs->GPIO_LATCLR = pins;

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

	config->regs->GPIO_LATINV = pins;
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
	gpio_registers_t *regs = config->regs;
	struct gpio_mchp_data *data = dev->data;
	gpio_flags_t flags = 0;

	/* Determine pin configuration flags */
	bool is_input = (regs->GPIO_TRIS & BIT(pin)) != 0;
	bool is_pullup_enabled = (regs->GPIO_CNPU & BIT(pin)) != 0;
	bool is_pulldwn_enabled = (regs->GPIO_CNPD & BIT(pin)) != 0;
	bool is_output_high = (regs->GPIO_PORT & BIT(pin)) != 0;
	bool is_active_low = data->common.invert & (gpio_port_pins_t)BIT(pin);
	bool is_open_drain = (regs->GPIO_ODC & BIT(pin)) != 0;

	/* Set flags based on pin configuration */
	if (is_input == true) {
		flags |= GPIO_INPUT;
		if (is_pullup_enabled == true) {
			flags |= GPIO_PULL_UP;
		}
		if (is_pulldwn_enabled == true) {
			flags |= GPIO_PULL_DOWN;
		}
	} else {
		flags |= GPIO_OUTPUT;
		flags |= is_output_high ? GPIO_OUTPUT_INIT_HIGH : GPIO_OUTPUT_INIT_LOW;
		if (is_open_drain == true) {
			flags |= GPIO_LINE_OPEN_DRAIN;
		}
	}

	/* Set active low/high flag */
	flags |= is_active_low ? GPIO_ACTIVE_LOW : GPIO_ACTIVE_HIGH;

	/* Output the flags */
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
	/* Get the GPIO register base address*/
	gpio_registers_t *regs = config->regs;

	map &= config->common.port_pin_mask;

	if (inputs != NULL) {
		/* Get the input pins */
		*inputs = map & (regs->GPIO_TRIS);
	}

	if (outputs != NULL) {
		/* Get the output pins */
		*outputs = map & ~(regs->GPIO_TRIS);
	}

	return 0;
}

#endif /* CONFIG_GPIO_GET_DIRECTION */

/**
 * @brief GPIO interrupt service routine (ISR).
 *
 * This function is called when a GPIO interrupt occurs. It logs the interrupt event
 * and invokes registered GPIO callbacks for the triggered pins.
 *
 * @param dev Pointer to the GPIO device structure.
 */
static void gpio_mchp_isr(const struct device *dev)
{
	const struct gpio_mchp_config *config = dev->config;
	struct gpio_mchp_data *data = dev->data;
	gpio_registers_t *regs = config->regs;
	uint32_t pins;

	if (regs->GPIO_CNCON & GPIO_CNCON_EDGEDETECT_Msk) {
		pins = regs->GPIO_CNF;
		regs->GPIO_CNF = 0;
	} else {
		pins = regs->GPIO_CNSTAT;
		pins &= regs->GPIO_CNEN;

		/* read PORT to clear mismatch condition to detect next pin change */
		regs->GPIO_PORT;
	}

	gpio_fire_callbacks(&data->callbacks, dev, pins);
}
/**
 * @brief Configure interrupt for a specific GPIO pin.
 *
 * This function configures the interrupt mode and trigger type for a given GPIO pin
 * on a Microchip GPIO controller. It sets up the necessary parameters and enables or
 * disables the interrupt as requested.
 *
 * @param[in] dev   Pointer to the device structure for the GPIO controller.
 * @param[in] pin   Pin number to configure.
 * @param[in] mode  Interrupt mode (e.g., disabled, edge, level).
 * @param[in] trig  Interrupt trigger type (e.g., rising, falling, both).
 *
 * @retval 0        On success.
 * @retval -ENOTSUP If an invalid mode or trigger type is specified.
 *
 * @note This function relies on the device's configuration and data structures.
 */
static int gpio_mchp_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
					     enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	int ret_val = 0;
	const struct gpio_mchp_config *config = dev->config;
	gpio_registers_t *regs = config->regs;
	uint32_t mask;
	uint32_t key;

	LOG_DBG("mode = 0x%x trig = 0x%x\n", mode, trig);

	mask = BIT(pin);

	/* Lock interrupts to ensure atomic operation */
	key = irq_lock();

	/*
	 * Handle GPIO interrupt configuration based on the trigger mode.
	 */
	switch (mode) {
	case GPIO_INT_MODE_DISABLED:
		regs->GPIO_CNENCLR = mask;
		regs->GPIO_CNNECLR = mask;
		break;
	case GPIO_INT_MODE_LEVEL:
		if (trig == GPIO_INT_TRIG_BOTH) {
			regs->GPIO_CNENSET = mask;
		} else {
			ret_val = -ENOTSUP;
		}
		break;
	case GPIO_INT_MODE_EDGE:
		switch (trig) {
		case GPIO_INT_TRIG_HIGH:
			regs->GPIO_CNENSET = mask;
			regs->GPIO_CNNECLR = mask;
			break;
		case GPIO_INT_TRIG_LOW:
			regs->GPIO_CNENCLR = mask;
			regs->GPIO_CNNESET = mask;
			break;
		case GPIO_INT_TRIG_BOTH:
			regs->GPIO_CNENSET = mask;
			regs->GPIO_CNNESET = mask;
			break;
		default:
			ret_val = -ENOTSUP;
			break;
		}
		break;
	default:
		ret_val = -ENOTSUP;
		break;
	}

	if (ret_val != -ENOTSUP) {
		switch (mode) {
		case GPIO_INT_MODE_DISABLED:
			if (regs->GPIO_CNEN == 0 && regs->GPIO_CNNE == 0) {
				regs->GPIO_CNCONCLR = GPIO_CNCON_ON_Msk;
			}
			break;
		case GPIO_INT_MODE_LEVEL:
			regs->GPIO_CNCONCLR = GPIO_CNCON_EDGEDETECT_Msk;
			regs->GPIO_CNCONSET = GPIO_CNCON_ON_Msk;
			/* read PORT to clear mismatch condition to detect next pin change */
			regs->GPIO_PORT;
			break;
		case GPIO_INT_MODE_EDGE:
			regs->GPIO_CNCONSET = GPIO_CNCON_ON_Msk | GPIO_CNCON_EDGEDETECT_Msk;
			break;
		}
	}

	irq_unlock(key);

	LOG_DBG("retval = %d", ret_val);
	return ret_val;
}

/**
 * @brief Manage (add or remove) a GPIO callback for the MCHP GPIO driver.
 *
 * This function adds or removes a user-specified callback from the GPIO
 * callback syslist associated with the device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param callback Pointer to the GPIO callback structure to add or remove.
 * @param set Boolean flag indicating whether to add (true) or remove (false) the callback.
 *
 * @retval 0 On success.
 * @retval -EINVAL If the callback is invalid or not found (when removing).
 *
 * @note This function is typically called by higher-level GPIO API functions
 *       to manage interrupt callbacks.
 */
static int gpio_mchp_manage_callback(const struct device *dev, struct gpio_callback *callback,
				     bool set)
{
	struct gpio_mchp_data *const data = dev->data;
	/**Adds or remove user specified callback on to the syslist */
	return gpio_manage_callback(&data->callbacks, callback, set);
}

/**
 * @brief Get pending GPIO interrupt status for the MCHP GPIO driver.
 *
 * This function retrieves the pending interrupt status for the specified
 * GPIO device. It is typically used to check if any GPIO interrupts are
 * pending, for example after waking up from low power mode.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @return A bitmask indicating the pin and port numbers of pending interrupts.
 *         Each bit set corresponds to a pending interrupt on a specific pin.
 *
 */
static uint32_t gpio_mchp_get_pending_int(const struct device *dev)
{
	const struct gpio_mchp_config *config = dev->config;
	gpio_registers_t *regs = config->regs;
	uint32_t pins;

	pins = regs->GPIO_CNSTAT;
	pins &= regs->GPIO_CNEN;

	return pins;
}

/**
 * @brief Initialize the GPIO driver
 *
 * @param dev Pointer to the device structure
 * @retval 0 on success
 */
static int gpio_mchp_init(const struct device *dev)
{
	const struct gpio_mchp_config *config = dev->config;

	config->irq_config();

	return 0;
}

static DEVICE_API(gpio, gpio_mchp_api) = {
	.pin_configure = gpio_mchp_config,
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
	.pin_interrupt_configure = gpio_mchp_pin_interrupt_configure,
	.manage_callback = gpio_mchp_manage_callback,
	.get_pending_int = gpio_mchp_get_pending_int,
};

#define MCHP_GPIO_IRQ_CONNECT(n, m)                                                                \
	do {                                                                                       \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, m, irq), DT_INST_IRQ_BY_IDX(n, m, priority),     \
			    gpio_mchp_isr, DEVICE_DT_INST_GET(n), 0);                              \
		irq_enable(DT_INST_IRQ_BY_IDX(n, m, irq));                                         \
	} while (false)

#define GPIO_MCHP_IRQ_HANDLER_DECL(n) static void gpio_mchp_irq_config_##n(void)

#define GPIO_MCHP_IRQ_HANDLER(n)                                                                   \
	static void gpio_mchp_irq_config_##n(void)                                                 \
	{                                                                                          \
		MCHP_GPIO_IRQ_CONNECT(n, 0);                                                       \
	}

#define GPIO_MCHP_DEVICE_INIT(n)                                                                   \
	DEVICE_DT_INST_DEFINE(n, gpio_mchp_init, NULL, &gpio_mchp_data_##n, &gpio_mchp_config_##n, \
			      POST_KERNEL, CONFIG_GPIO_INIT_PRIORITY, &gpio_mchp_api)

#define GPIO_MCHP_INIT(n)                                                                          \
	GPIO_MCHP_IRQ_HANDLER_DECL(n);                                                             \
	static const struct gpio_mchp_config gpio_mchp_config_##n = {                              \
		.common =                                                                          \
			{                                                                          \
				.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n),               \
			},                                                                         \
		.regs = (gpio_registers_t *)DT_INST_REG_ADDR(n),                                   \
		.irq_config = gpio_mchp_irq_config_##n};                                           \
	static struct gpio_mchp_data gpio_mchp_data_##n;                                           \
	GPIO_MCHP_DEVICE_INIT(n)                                                                   \
	GPIO_MCHP_IRQ_HANDLER(n)

DT_INST_FOREACH_STATUS_OKAY(GPIO_MCHP_INIT)
