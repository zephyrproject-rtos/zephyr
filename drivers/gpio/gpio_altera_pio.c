/*
 * Copyright (c) 2022, Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT altr_pio

/**
 * @file
 * @brief PIO driver for Intel FPGA PIO Core IP
 * Reference : Embedded Peripherals IP User Guide : 27. PIO Core
 */

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>
#include <inttypes.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <string.h>
#include <stdbool.h>

#define ALTERA_AVALON_PIO_DATA_OFFSET		0x00
#define ALTERA_AVALON_PIO_DIRECTION_OFFSET	0x04
#define ALTERA_AVALON_PIO_IRQ_OFFSET		0x08
#define ALTERA_AVALON_PIO_SET_BITS		0x10
#define ALTERA_AVALON_PIO_CLEAR_BITS		0x14

#define DEV_CFG(_dev) ((struct gpio_altera_config *const)(_dev)->config)

typedef void (*altera_cfg_func_t)(void);

struct gpio_altera_config {
	struct gpio_driver_config	common;
	uintptr_t			reg_base;
	uint32_t			irq_num;
	uint8_t				direction;
	altera_cfg_func_t		cfg_func;
};

struct gpio_altera_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	/* list of callbacks */
	sys_slist_t cb;
};

/**
 * @brief Determine the pin direction.
 *
 * @param dev		Pointer to device structure for the driver instance.
 * @param pin_mask	Value indicating which pins will be determined.
 *
 * @return 0 if the pin direction is input, 1 if the pin direction is output.
 */
static bool gpio_pin_direction(const struct device *dev, uint32_t pin_mask)
{
	struct gpio_altera_config *const cfg = DEV_CFG(dev);
	const int direction = cfg->direction;
	uintptr_t reg_base = cfg->reg_base;
	uint32_t addr;
	uint32_t pin_direction;

	if (pin_mask == 0) {
		return -EINVAL;
	}

	/* Check if the direction is Bidirectional */
	if (direction != 0) {
		return -EINVAL;
	}

	addr = reg_base + ALTERA_AVALON_PIO_DIRECTION_OFFSET;

	pin_direction = sys_read32(addr);

	if (!(pin_direction & pin_mask)) {
		return false;
	}

	return true;
}

/**
 * @brief Configure a single pin.
 *
 * Configure the port direction. The selected pin configured to 0
 * if set to input and 1 if set to output.
 *
 * @param dev	Pointer to device structure for the driver instance.
 * @param pin	Pin number to configure. (0 to 31)
 * @param flags Flags for pin configuration: 'GPIO_INPUT/GPIO_OUTPUT'.
 *
 * @return 0 if successful, failed otherwise.
 */
static int gpio_altera_configure(const struct device *dev,
				 gpio_pin_t pin, gpio_flags_t flags)
{
	struct gpio_altera_config *const cfg = DEV_CFG(dev);
	const int port_pin_mask = cfg->common.port_pin_mask;
	const int direction = cfg->direction;
	uintptr_t reg_base = cfg->reg_base;
	uint32_t addr;

	/* Check if pin number is within range */
	if ((port_pin_mask & BIT(pin)) == 0) {
		return -EINVAL;
	}

	/* Check if the direction is Bidirectional */
	if (direction != 0) {
		return -EINVAL;
	}

	addr = reg_base + ALTERA_AVALON_PIO_DIRECTION_OFFSET;

	if (flags == GPIO_INPUT) {
		sys_clear_bits(addr, BIT(pin));
	} else if (flags == GPIO_OUTPUT) {
		sys_set_bits(addr, BIT(pin));
	} else {
		return -EINVAL;
	}

	return 0;
}

/**
 * @brief Get physical level of all input pins in a port.
 *
 * Read the value from the port. A low physical level on the
 * pin will be interpreted as value 0. A high physical level
 * will be interpreted as value 1.
 *
 * @param dev	Pointer to the device tructure for the driver instance.
 * @param value Pointer to a variable where pin values will be stored.
 *
 * @return 0 if successful, failed otherwise.
 */
static int gpio_altera_port_get_raw(const struct device *dev, uint32_t *value)
{
	struct gpio_altera_config *const cfg = DEV_CFG(dev);
	uintptr_t reg_base = cfg->reg_base;
	uint32_t addr;

	addr = reg_base + ALTERA_AVALON_PIO_DATA_OFFSET;

	__ASSERT(value != NULL, "value is null pointer!");

	if	(value == NULL)	{
		return -EINVAL;
	}

	*value = sys_read32((addr));

	return 0;
}

/**
 * @brief Set physical level of selected output pins to high.
 *
 * Set the port to 1. It will only write if data direction is set to output.
 *
 * @param dev  Pointer to the device structure for the driver instance.
 * @param mask Value indicating which pins will be modified.
 *
 * @return 0 if successful, failed otherwise.
 */
static int gpio_altera_port_set_bits_raw(const struct device *dev, gpio_port_pins_t mask)
{
	struct gpio_altera_config *const cfg = DEV_CFG(dev);
	const int port_pin_mask = cfg->common.port_pin_mask;
	uintptr_t reg_base = cfg->reg_base;
	uint32_t addr;

	if ((port_pin_mask & mask) == 0) {
		return -EINVAL;
	}

	if (!gpio_pin_direction(dev, mask)) {
		return -EINVAL;
	}

#ifdef CONFIG_GPIO_ALTERA_PIO_OUTSET
	addr = reg_base + ALTERA_AVALON_PIO_SET_BITS;
	sys_write32(mask, addr);
#else
	addr = reg_base + ALTERA_AVALON_PIO_DATA_OFFSET;
	sys_set_bits(addr, mask);
#endif

	return 0;
}

/**
 * @brief Set physical level of selected output pins to low.
 *
 * Set the port to 0. It will only write if data direction is set to output.
 *
 * @param dev  Pointer to the device structure for the driver instance.
 * @param mask Value indicating which pins will be modified.
 *
 * @return 0 if successful, failed otherwise.
 */
static int gpio_altera_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t mask)
{
	struct gpio_altera_config *const cfg = DEV_CFG(dev);
	const int port_pin_mask = cfg->common.port_pin_mask;
	uintptr_t reg_base = cfg->reg_base;
	uint32_t addr;

	/* Check if mask range within 32 */
	if ((port_pin_mask & mask) == 0) {
		return -EINVAL;
	}

	if (!gpio_pin_direction(dev, mask)) {
		return -EINVAL;
	}

#ifdef CONFIG_GPIO_ALTERA_PIO_OUTCLEAR
	addr = reg_base + ALTERA_AVALON_PIO_CLEAR_BITS;
	sys_write32(mask, addr);
#else
	addr = reg_base + ALTERA_AVALON_PIO_DATA_OFFSET;
	sys_clear_bits(addr, mask);
#endif

	return 0;
}

/**
 * @brief Initialise an instance of the driver.
 *
 * This function initialise the interrupt configuration for the driver.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @return 0 to indicate success.
 */
static int gpio_init(const struct device *dev)
{
	struct gpio_altera_config *const cfg = DEV_CFG(dev);

	/* Configure GPIO device */
	cfg->cfg_func();

	return 0;
}

/**
 * @brief Configure pin interrupt.
 *
 * @param dev	Pointer to the device structure for the driver instance.
 * @param pin	Pin number to configure. (0 to 31)
 * @param mode	Public flags. (GPIO_INT_MODE_DISABLED, GPIO_INT_MODE_LEVEL and
 *				GPIO_INT_MODE_EDGE)
 * @param trig	Public flags. (Edge Failing, Edge Rising or any)
 *
 * @return 0 if successful, failed otherwise.
 */
static int gpio_altera_pin_interrupt_configure(const struct device *dev,
						gpio_pin_t pin,
						enum gpio_int_mode mode,
						enum gpio_int_trig trig)
{
	ARG_UNUSED(trig);

	struct gpio_altera_config *const cfg = DEV_CFG(dev);
	uintptr_t reg_base = cfg->reg_base;
	const int port_pin_mask = cfg->common.port_pin_mask;
	uint32_t addr;

	/* Check if pin number is within range */
	if ((port_pin_mask & BIT(pin)) == 0) {
		return -EINVAL;
	}

	addr = reg_base + ALTERA_AVALON_PIO_IRQ_OFFSET;

	switch (mode) {
	case GPIO_INT_MODE_DISABLED:
		/* Disable interrupt of pin */
		sys_clear_bits(addr, BIT(pin));
		break;
	case GPIO_INT_MODE_LEVEL:
	case GPIO_INT_MODE_EDGE:
		/* Enable interrupt of pin */
		sys_set_bits(addr, BIT(pin));
		irq_enable(cfg->irq_num);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

/**
 * @brief Generic function to insert or remove a callback from a callback list.
 *
 * @param dev		Pointer to the device structure for the driver instance.
 * @param callback	A valid Application’s callback structure pointer.
 * @param set		A boolean indicating insertion or removal of the callback.
 *
 * @return 0 if successful, failed otherwise.
 */
static int gpio_altera_manage_callback(const struct device *dev,
					struct gpio_callback *callback,
					bool set)
{

	struct gpio_altera_data * const data = dev->data;

	return gpio_manage_callback(&data->cb, callback, set);
}

/**
 * @brief Address of interrupt service routine.
 *
 * @param dev	Pointer to the device structure for the driver instance.
 *
 */
static void gpio_altera_irq_handler(const struct device *dev)
{
	struct gpio_altera_config * const cfg = DEV_CFG(dev);
	struct gpio_altera_data *data = dev->data;
	uintptr_t reg_base = cfg->reg_base;
	uint32_t port_value;
	uint32_t addr;

	addr = reg_base + ALTERA_AVALON_PIO_IRQ_OFFSET;

	port_value = sys_read32(addr);

	sys_clear_bits(addr, port_value);

	/* Call the corresponding callback registered for the pin */
	gpio_fire_callbacks(&data->cb, dev, port_value);
}

static const struct gpio_driver_api gpio_altera_driver_api = {
	.pin_configure		 = gpio_altera_configure,
	.port_get_raw		 = gpio_altera_port_get_raw,
	.port_set_masked_raw	 = NULL,
	.port_set_bits_raw	 = gpio_altera_port_set_bits_raw,
	.port_clear_bits_raw	 = gpio_altera_port_clear_bits_raw,
	.port_toggle_bits	 = NULL,
	.pin_interrupt_configure = gpio_altera_pin_interrupt_configure,
	.manage_callback	 = gpio_altera_manage_callback
};

#define CREATE_GPIO_DEVICE(n)						\
	static void gpio_altera_cfg_func_##n(void);			\
	static struct gpio_altera_data gpio_altera_data_##n;		\
	static struct gpio_altera_config gpio_config_##n = {		\
		.common		= {					\
		.port_pin_mask	=					\
				  GPIO_PORT_PIN_MASK_FROM_DT_INST(n),	\
		},						        \
		.reg_base	= DT_INST_REG_ADDR(n),			\
		.direction	= DT_INST_ENUM_IDX(n, direction),	\
		.irq_num	= DT_INST_IRQN(n),			\
		.cfg_func	= gpio_altera_cfg_func_##n,		\
	};								\
							\
	DEVICE_DT_INST_DEFINE(n,			\
			      gpio_init,		\
			      NULL,			\
			      &gpio_altera_data_##n,	\
			      &gpio_config_##n,		\
			      PRE_KERNEL_1,		\
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE,	\
			      &gpio_altera_driver_api);		        \
									\
	static void gpio_altera_cfg_func_##n(void)			\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n),			        \
			    DT_INST_IRQ(n, priority),			\
			    gpio_altera_irq_handler,			\
			    DEVICE_DT_INST_GET(n),			\
			    0);						\
	}

DT_INST_FOREACH_STATUS_OKAY(CREATE_GPIO_DEVICE)

