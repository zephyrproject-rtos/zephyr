/*
 * Xilinx Processor System MIO / EMIO GPIO controller driver
 * GPIO bank module
 *
 * Copyright (c) 2022, Weidmueller Interface GmbH & Co. KG
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include "gpio_xlnx_ps_bank.h"

#define LOG_MODULE_NAME gpio_xlnx_ps_bank
#define LOG_LEVEL CONFIG_GPIO_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define DT_DRV_COMPAT xlnx_ps_gpio_bank

/**
 * @brief GPIO bank pin configuration function
 *
 * Configures an individual pin within a MIO / EMIO GPIO pin bank.
 * The following flags specified by the GPIO subsystem are NOT
 * supported by the PS GPIO controller:
 *
 * - Pull up
 * - Pull down
 * - Open drain
 * - Open source.
 *
 * @param dev Pointer to the GPIO bank's device.
 * @param pin Index of the pin within the bank to be configured
 *            (decimal index of the pin).
 * @param flags Flags specifying the pin's configuration.
 *
 * @retval 0 if the device initialization completed successfully,
 *         -EINVAL if the specified pin index is out of range,
 *         -ENOTSUP if the pin configuration data contains a flag
 *         that is not supported by the controller.
 */
static int gpio_xlnx_ps_pin_configure(const struct device *dev,
				      gpio_pin_t pin,
				      gpio_flags_t flags)
{
	const struct gpio_xlnx_ps_bank_dev_cfg *dev_conf = dev->config;
	uint32_t pin_mask = BIT(pin);
	uint32_t bank_data;
	uint32_t dirm_data;
	uint32_t oen_data;

	/* Validity of the specified pin index is checked in drivers/gpio.h */

	/* Check for config flags not supported by the controller */
	if (flags & (GPIO_PULL_UP | GPIO_PULL_DOWN | GPIO_SINGLE_ENDED)) {
		return -ENOTSUP;
	}

	/* Read the data direction & output enable registers */
	dirm_data = sys_read32(GPIO_XLNX_PS_BANK_DIRM_REG);
	oen_data = sys_read32(GPIO_XLNX_PS_BANK_OEN_REG);

	if (flags & GPIO_OUTPUT) {
		dirm_data |= pin_mask;
		oen_data |= pin_mask;

		/*
		 * Setting of an initial value (see below) requires the
		 * direction register to be written *BEFORE* the data
		 * register, otherwise, the value will not be applied!
		 * The output enable bit can be set after the initial
		 * value has been written.
		 */
		sys_write32(dirm_data, GPIO_XLNX_PS_BANK_DIRM_REG);

		/*
		 * If the current pin is to be configured as an output
		 * pin, it is up to the caller to specify whether the
		 * output's initial value shall be high or low.
		 * -> Write the initial output value into the data register.
		 */
		bank_data = sys_read32(GPIO_XLNX_PS_BANK_DATA_REG);
		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			bank_data |= pin_mask;
		} else if (flags & GPIO_OUTPUT_INIT_LOW) {
			bank_data &= ~pin_mask;
		}
		sys_write32(bank_data, GPIO_XLNX_PS_BANK_DATA_REG);

		/* Set the pin's output enable bit */
		sys_write32(oen_data, GPIO_XLNX_PS_BANK_OEN_REG);
	} else {
		dirm_data &= ~pin_mask;
		oen_data &= ~pin_mask;

		/*
		 * Disable the output first in case of an O -> I
		 * transition, then change the pin's direction.
		 */
		sys_write32(oen_data, GPIO_XLNX_PS_BANK_OEN_REG);
		sys_write32(dirm_data, GPIO_XLNX_PS_BANK_DIRM_REG);
	}

	return 0;
}

/**
 * @brief Reads the current bit mask of the entire GPIO pin bank.
 *
 * Reads the current bit mask of the entire bank from the
 * read-only data register. This includes the current values
 * of not just all input pins, but both the input and output
 * pins within the bank.
 *
 * @param dev   Pointer to the GPIO bank's device.
 * @param value Pointer to a variable of type gpio_port_value_t
 *              to which the current bit mask read from the bank's
 *              RO data register will be written to.
 *
 * @retval 0 if the read operation completed successfully,
 *         -EINVAL if the pointer to the output variable is NULL.
 */
static int gpio_xlnx_ps_bank_get(const struct device *dev,
				 gpio_port_value_t *value)
{
	const struct gpio_xlnx_ps_bank_dev_cfg *dev_conf = dev->config;

	*value = sys_read32(GPIO_XLNX_PS_BANK_DATA_REG);
	return 0;
}

/**
 * @brief Masked write of a bit mask for the entire GPIO pin bank.
 *
 * Performs a masked write operation on the data register of
 * the current GPIO pin bank. The mask is applied twice:
 * first, it is applied to the current contents of the bank's
 * RO data register, clearing any bits that are zeroes in the
 * mask (will not have any effect on input pins). Second, it
 * is applied to the data word to be written into the current
 * bank's data register. The masked data word read from the
 * RO data register and the masked data word provided by the
 * caller ar then OR'ed and written to the bank's data register.
 *
 * @param dev   Pointer to the GPIO bank's device.
 * @param mask  Mask to be applied to both the current contents
 *              of the data register and the data word provided
 *              by the caller.
 * @param value Value to be written to the current bank's data
 *              register.
 *
 * @retval Always 0.
 */
static int gpio_xlnx_ps_bank_set_masked(const struct device *dev,
					gpio_port_pins_t mask,
					gpio_port_value_t value)
{
	const struct gpio_xlnx_ps_bank_dev_cfg *dev_conf = dev->config;
	uint32_t bank_data;

	bank_data = sys_read32(GPIO_XLNX_PS_BANK_DATA_REG);
	bank_data = (bank_data & ~mask) | (value & mask);
	sys_write32(bank_data, GPIO_XLNX_PS_BANK_DATA_REG);

	return 0;
}

/**
 * @brief Sets bits in the data register of the GPIO pin bank.
 *
 * Sets bits in the data register of the current GPIO pin bank
 * as a read-modify-write operation. All bits set in the bit
 * mask provided by the caller are OR'ed into the current data
 * word of the bank. This operation has no effect on the values
 * associated with pins configured as inputs.
 *
 * @param dev   Pointer to the GPIO bank's device.
 * @param pins  Bit mask specifying which bits shall be set in
 *              the data word of the current GPIO pin bank.
 *
 * @retval Always 0.
 */
static int gpio_xlnx_ps_bank_set_bits(const struct device *dev,
				      gpio_port_pins_t pins)
{
	const struct gpio_xlnx_ps_bank_dev_cfg *dev_conf = dev->config;
	uint32_t bank_data;

	bank_data = sys_read32(GPIO_XLNX_PS_BANK_DATA_REG);
	bank_data |= pins;
	sys_write32(bank_data, GPIO_XLNX_PS_BANK_DATA_REG);

	return 0;
}

/**
 * @brief Clears bits in the data register of the GPIO pin bank.
 *
 * Clears bits in the data register of the current GPIO pin bank
 * as a read-modify-write operation. All bits set in the bit
 * mask provided by the caller are NAND'ed into the current data
 * word of the bank. This operation has no effect on the values
 * associated with pins configured as inputs.
 *
 * @param dev   Pointer to the GPIO bank's device.
 * @param pins  Bit mask specifying which bits shall be cleared
 *              in the data word of the current GPIO pin bank.
 *
 * @retval Always 0.
 */
static int gpio_xlnx_ps_bank_clear_bits(const struct device *dev,
					gpio_port_pins_t pins)
{
	const struct gpio_xlnx_ps_bank_dev_cfg *dev_conf = dev->config;
	uint32_t bank_data;

	bank_data = sys_read32(GPIO_XLNX_PS_BANK_DATA_REG);
	bank_data &= ~pins;
	sys_write32(bank_data, GPIO_XLNX_PS_BANK_DATA_REG);

	return 0;
}

/**
 * @brief Toggles bits in the data register of the GPIO pin bank.
 *
 * Toggles bits in the data register of the current GPIO pin bank
 * as a read-modify-write operation. All bits set in the bit
 * mask provided by the caller are XOR'ed into the current data
 * word of the bank. This operation has no effect on the values
 * associated with pins configured as inputs.
 *
 * @param dev   Pointer to the GPIO bank's device.
 * @param pins  Bit mask specifying which bits shall be toggled
 *              in the data word of the current GPIO pin bank.
 *
 * @retval Always 0.
 */
static int gpio_xlnx_ps_bank_toggle_bits(const struct device *dev,
					 gpio_port_pins_t pins)
{
	const struct gpio_xlnx_ps_bank_dev_cfg *dev_conf = dev->config;
	uint32_t bank_data;

	bank_data = sys_read32(GPIO_XLNX_PS_BANK_DATA_REG);
	bank_data ^= pins;
	sys_write32(bank_data, GPIO_XLNX_PS_BANK_DATA_REG);

	return 0;
}

/**
 * @brief Configures the interrupt behaviour of a pin within the
 *        current GPIO bank.
 *
 * Configures the interrupt behaviour of a pin within the current
 * GPIO bank. If a pin is to be configured to trigger an interrupt,
 * the following modes are supported:
 *
 * - edge or level triggered,
 * - rising edge / high level or falling edge / low level,
 * - in edge mode only: trigger on both rising and falling edge.
 *
 * @param dev  Pointer to the GPIO bank's device.
 * @param pin  Index of the pin within the bank to be configured
 *             (decimal index of the pin).
 * @param mode Mode configuration: edge, level or interrupt disabled.
 * @param trig Trigger condition configuration: high/low level or
 *             rising/falling/both edge(s).
 *
 * @retval 0 if the interrupt configuration completed successfully,
 *         -EINVAL if the specified pin index is out of range,
 *         -ENOTSUP if the interrupt configuration data contains an
 *         invalid combination of configuration flags.
 */
static int gpio_xlnx_ps_bank_pin_irq_configure(const struct device *dev,
					       gpio_pin_t pin,
					       enum gpio_int_mode mode,
					       enum gpio_int_trig trig)
{
	const struct gpio_xlnx_ps_bank_dev_cfg *dev_conf = dev->config;
	uint32_t pin_mask = BIT(pin);
	uint32_t int_type_data;
	uint32_t int_polarity_data;
	uint32_t int_any_data;

	/* Validity of the specified pin index is checked in drivers/gpio.h */

	/* Disable the specified pin's interrupt before (re-)configuring it */
	sys_write32(pin_mask, GPIO_XLNX_PS_BANK_INT_DIS_REG);

	int_type_data = sys_read32(GPIO_XLNX_PS_BANK_INT_TYPE_REG);
	int_polarity_data = sys_read32(GPIO_XLNX_PS_BANK_INT_POLARITY_REG);
	int_any_data = sys_read32(GPIO_XLNX_PS_BANK_INT_ANY_REG);

	if (mode != GPIO_INT_MODE_DISABLED) {

		if (mode == GPIO_INT_MODE_LEVEL) {
			int_type_data &= ~pin_mask;
		} else if (mode == GPIO_INT_MODE_EDGE) {
			int_type_data |= pin_mask;
		} else {
			return -EINVAL;
		}

		if (trig == GPIO_INT_TRIG_LOW) {
			int_any_data &= ~pin_mask;
			int_polarity_data &= ~pin_mask;
		} else if (trig == GPIO_INT_TRIG_HIGH) {
			int_any_data &= ~pin_mask;
			int_polarity_data |= pin_mask;
		} else if (trig == GPIO_INT_TRIG_BOTH) {
			if (mode == GPIO_INT_MODE_LEVEL) {
				return -EINVAL;
			}
			int_any_data |= pin_mask;
		}

	} else { /* mode == GPIO_INT_MODE_DISABLED */
		int_any_data &= ~pin_mask;
		int_polarity_data &= ~pin_mask;
		int_type_data &= ~pin_mask;
	}

	sys_write32(int_any_data, GPIO_XLNX_PS_BANK_INT_ANY_REG);
	sys_write32(int_polarity_data, GPIO_XLNX_PS_BANK_INT_POLARITY_REG);
	sys_write32(int_type_data, GPIO_XLNX_PS_BANK_INT_TYPE_REG);

	if (mode != GPIO_INT_MODE_DISABLED) {
		/* Clear potential stale pending bit before enabling interrupt */
		sys_write32(pin_mask, GPIO_XLNX_PS_BANK_INT_STAT_REG);
		sys_write32(pin_mask, GPIO_XLNX_PS_BANK_INT_EN_REG);
	}

	return 0;
}

/**
 * @brief Returns the interrupt status of the current GPIO bank.
 *
 * Returns the interrupt status of the current GPIO bank, in the
 * form of a bit mask where each pin with a pending interrupt is
 * indicated. This information can either be used by the PM sub-
 * system or the parent controller device driver, which manages
 * the interrupt line of the entire PS GPIO controller, regardless
 * of how many bank sub-devices exist. As the current status is
 * read, it is automatically cleared. Callback triggering is handled
 * by the parent controller device.
 *
 * @param dev  Pointer to the GPIO bank's device.
 *
 * @retval A bit mask indicating for which pins within the bank
 *         an interrupt is pending.
 */
static uint32_t gpio_xlnx_ps_bank_get_int_status(const struct device *dev)
{
	const struct gpio_xlnx_ps_bank_dev_cfg *dev_conf = dev->config;
	uint32_t int_status;

	int_status = sys_read32(GPIO_XLNX_PS_BANK_INT_STAT_REG);
	if (int_status != 0) {
		sys_write32(int_status, GPIO_XLNX_PS_BANK_INT_STAT_REG);
	}

	return int_status;
}

/**
 * @brief Callback management re-direction function.
 *
 * Re-directs any callback management calls relating to the current
 * GPIO bank to the GPIO sub-system. Comp. documentation of the
 * underlying sub-system's #gpio_manage_callback function.
 *
 * @param dev      Pointer to the GPIO bank's device.
 * @param callback Pointer to a GPIO callback struct.
 * @param set      Callback set flag.
 *
 * @retval A bit mask indicating for which pins within the bank
 *         an interrupt is pending.
 */
static int gpio_xlnx_ps_bank_manage_callback(const struct device *dev,
					     struct gpio_callback *callback,
					     bool set)
{
	struct gpio_xlnx_ps_bank_dev_data *dev_data = dev->data;

	return gpio_manage_callback(&dev_data->callbacks, callback, set);
}

/* GPIO bank device driver API */
static const struct gpio_driver_api gpio_xlnx_ps_bank_apis = {
	.pin_configure = gpio_xlnx_ps_pin_configure,
	.port_get_raw = gpio_xlnx_ps_bank_get,
	.port_set_masked_raw = gpio_xlnx_ps_bank_set_masked,
	.port_set_bits_raw = gpio_xlnx_ps_bank_set_bits,
	.port_clear_bits_raw = gpio_xlnx_ps_bank_clear_bits,
	.port_toggle_bits = gpio_xlnx_ps_bank_toggle_bits,
	.pin_interrupt_configure = gpio_xlnx_ps_bank_pin_irq_configure,
	.manage_callback = gpio_xlnx_ps_bank_manage_callback,
	.get_pending_int = gpio_xlnx_ps_bank_get_int_status
};

/**
 * @brief Initialize a MIO / EMIO GPIO bank sub-device
 *
 * Initialize a MIO / EMIO GPIO bank sub-device, which is a child
 * of the parent Xilinx PS GPIO controller device driver. This ini-
 * tialization function sets up a defined initial state for each
 * GPIO bank.
 *
 * @param dev Pointer to the GPIO bank's device.
 *
 * @retval Always 0.
 */
static int gpio_xlnx_ps_bank_init(const struct device *dev)
{
	const struct gpio_xlnx_ps_bank_dev_cfg *dev_conf = dev->config;

	sys_write32(~0x0, GPIO_XLNX_PS_BANK_INT_DIS_REG);  /* Disable all interrupts */
	sys_write32(~0x0, GPIO_XLNX_PS_BANK_INT_STAT_REG); /* Clear all interrupts */
	sys_write32(0x0, GPIO_XLNX_PS_BANK_OEN_REG);       /* All outputs disabled */
	sys_write32(0x0, GPIO_XLNX_PS_BANK_DIRM_REG);      /* All pins input */
	sys_write32(0x0, GPIO_XLNX_PS_BANK_DATA_REG);      /* Zero data register */

	return 0;
}

/* MIO / EMIO bank device definition macros */
#define GPIO_XLNX_PS_BANK_INIT(idx)\
static const struct gpio_xlnx_ps_bank_dev_cfg gpio_xlnx_ps_bank##idx##_cfg = {\
	.common = {\
		.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(idx),\
	},\
	.base_addr = DT_REG_ADDR(DT_PARENT(DT_INST(idx, DT_DRV_COMPAT))),\
	.bank_index = idx,\
};\
static struct gpio_xlnx_ps_bank_dev_data gpio_xlnx_ps_bank##idx##_data;\
DEVICE_DT_INST_DEFINE(idx, gpio_xlnx_ps_bank_init, NULL,\
	&gpio_xlnx_ps_bank##idx##_data, &gpio_xlnx_ps_bank##idx##_cfg,\
	PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY, &gpio_xlnx_ps_bank_apis);

/* Register & initialize all MIO / EMIO GPIO banks specified in the device tree. */
DT_INST_FOREACH_STATUS_OKAY(GPIO_XLNX_PS_BANK_INIT);
