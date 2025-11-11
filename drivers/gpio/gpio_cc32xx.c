/*
 * Copyright (c) 2016, Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc32xx_gpio
#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>

/* Driverlib includes */
#include <inc/hw_types.h>
#include <inc/hw_memmap.h>
#include <inc/hw_ints.h>
#include <inc/hw_gpio.h>
#include <driverlib/rom.h>
#include <driverlib/pin.h>
#undef __GPIO_H__  /* Zephyr and CC32XX SDK gpio.h conflict */
#include <driverlib/gpio.h>
#include <driverlib/rom_map.h>
#include <driverlib/interrupt.h>
#include <zephyr/irq.h>

#include <zephyr/drivers/gpio/gpio_utils.h>

/* Reserved */
#define PIN_XX  0xFF

static const uint8_t pinTable[] = {
	/* 00     01      02      03      04      05      06      07  */
	PIN_50, PIN_55, PIN_57, PIN_58, PIN_59, PIN_60, PIN_61, PIN_62,
	/* 08     09      10      11      12      13      14      15  */
	PIN_63, PIN_64, PIN_01, PIN_02, PIN_03, PIN_04, PIN_05, PIN_06,
	/* 16     17      18      19      20      21      22      23  */
	PIN_07, PIN_08, PIN_XX, PIN_XX, PIN_XX, PIN_XX, PIN_15, PIN_16,
	/* 24     25      26      27      28      29      30      31  */
	PIN_17, PIN_21, PIN_29, PIN_30, PIN_18, PIN_20, PIN_53, PIN_45,
	/* 32 */
	PIN_52
};

struct gpio_cc32xx_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	/* base address of GPIO port */
	unsigned long port_base;
	/* GPIO port number */
	uint8_t port_num;
};

struct gpio_cc32xx_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	/* list of registered callbacks */
	sys_slist_t callbacks;
};

static int gpio_cc32xx_port_set_bits_raw(const struct device *port,
					 uint32_t mask);
static int gpio_cc32xx_port_clear_bits_raw(const struct device *port,
					   uint32_t mask);

static inline int gpio_cc32xx_config(const struct device *port,
				     gpio_pin_t pin,
				     gpio_flags_t flags)
{
	const struct gpio_cc32xx_config *gpio_config = port->config;
	unsigned long port_base = gpio_config->port_base;

	if (((flags & GPIO_INPUT) != 0) && ((flags & GPIO_OUTPUT) != 0)) {
		return -ENOTSUP;
	}

	if ((flags & (GPIO_INPUT | GPIO_OUTPUT)) == 0) {
		return -ENOTSUP;
	}

	if ((flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) != 0) {
		return -ENOTSUP;
	}

	MAP_PinTypeGPIO(pinTable[gpio_config->port_num * 8 + pin],
		PIN_MODE_0, false);
	if (flags & GPIO_OUTPUT) {
		MAP_GPIODirModeSet(port_base, (1 << pin), GPIO_DIR_MODE_OUT);
		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
			gpio_cc32xx_port_set_bits_raw(port, BIT(pin));
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
			gpio_cc32xx_port_clear_bits_raw(port, BIT(pin));
		}
	} else {
		MAP_GPIODirModeSet(port_base, (1 << pin), GPIO_DIR_MODE_IN);
	}

	return 0;
}

static int gpio_cc32xx_port_get_raw(const struct device *port,
				    uint32_t *value)
{
	const struct gpio_cc32xx_config *gpio_config = port->config;
	unsigned long port_base = gpio_config->port_base;
	unsigned char pin_packed = 0xFF;

	*value = MAP_GPIOPinRead(port_base, pin_packed);

	return 0;
}

static int gpio_cc32xx_port_set_masked_raw(const struct device *port,
					   uint32_t mask,
					   uint32_t value)
{
	const struct gpio_cc32xx_config *gpio_config = port->config;
	unsigned long port_base = gpio_config->port_base;

	MAP_GPIOPinWrite(port_base, (unsigned char)mask, (unsigned char)value);

	return 0;
}

static int gpio_cc32xx_port_set_bits_raw(const struct device *port,
					 uint32_t mask)
{
	const struct gpio_cc32xx_config *gpio_config = port->config;
	unsigned long port_base = gpio_config->port_base;

	MAP_GPIOPinWrite(port_base, (unsigned char)mask, (unsigned char)mask);

	return 0;
}

static int gpio_cc32xx_port_clear_bits_raw(const struct device *port,
					   uint32_t mask)
{
	const struct gpio_cc32xx_config *gpio_config = port->config;
	unsigned long port_base = gpio_config->port_base;

	MAP_GPIOPinWrite(port_base, (unsigned char)mask, (unsigned char)~mask);

	return 0;
}

static int gpio_cc32xx_port_toggle_bits(const struct device *port,
					uint32_t mask)
{
	const struct gpio_cc32xx_config *gpio_config = port->config;
	unsigned long port_base = gpio_config->port_base;
	long value;

	value = MAP_GPIOPinRead(port_base, mask);

	MAP_GPIOPinWrite(port_base, (unsigned char)mask,
		(unsigned char)~value);

	return 0;
}

static int gpio_cc32xx_pin_interrupt_configure(const struct device *port,
					       gpio_pin_t pin,
					       enum gpio_int_mode mode,
					       enum gpio_int_trig trig)
{
	const struct gpio_cc32xx_config *gpio_config = port->config;
	unsigned long port_base = gpio_config->port_base;
	unsigned long int_type;

	__ASSERT(pin < 8, "Invalid pin number - only 8 pins per port");

	/*
	 * disable interrupt prior to changing int type helps
	 * prevent spurious interrupts observed when switching
	 * to level-based
	 */
	MAP_GPIOIntDisable(port_base, (1 << pin));

	if (mode != GPIO_INT_MODE_DISABLED) {
		if (mode == GPIO_INT_MODE_EDGE) {
			if (trig == GPIO_INT_TRIG_BOTH) {
				int_type = GPIO_BOTH_EDGES;
			} else if (trig == GPIO_INT_TRIG_HIGH) {
				int_type = GPIO_RISING_EDGE;
			} else {
				int_type = GPIO_FALLING_EDGE;
			}
		} else { /* GPIO_INT_MODE_LEVEL */
			if (trig == GPIO_INT_TRIG_HIGH) {
				int_type = GPIO_HIGH_LEVEL;
			} else {
				int_type = GPIO_LOW_LEVEL;
			}
		}

		MAP_GPIOIntTypeSet(port_base, (1 << pin), int_type);
		MAP_GPIOIntClear(port_base, (1 << pin));
		MAP_GPIOIntEnable(port_base, (1 << pin));
	}

	return 0;
}

static int gpio_cc32xx_manage_callback(const struct device *dev,
				       struct gpio_callback *callback,
				       bool set)
{
	struct gpio_cc32xx_data *data = dev->data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}

static void gpio_cc32xx_port_isr(const struct device *dev)
{
	const struct gpio_cc32xx_config *config = dev->config;
	struct gpio_cc32xx_data *data = dev->data;
	uint32_t int_status;

	/* See which interrupts triggered: */
	int_status = (uint32_t)MAP_GPIOIntStatus(config->port_base, 1);

	/* Clear GPIO Interrupt */
	MAP_GPIOIntClear(config->port_base, int_status);

	/* Call the registered callbacks */
	gpio_fire_callbacks(&data->callbacks, dev, int_status);
}

static DEVICE_API(gpio, api_funcs) = {
	.pin_configure = gpio_cc32xx_config,
	.port_get_raw = gpio_cc32xx_port_get_raw,
	.port_set_masked_raw = gpio_cc32xx_port_set_masked_raw,
	.port_set_bits_raw = gpio_cc32xx_port_set_bits_raw,
	.port_clear_bits_raw = gpio_cc32xx_port_clear_bits_raw,
	.port_toggle_bits = gpio_cc32xx_port_toggle_bits,
	.pin_interrupt_configure = gpio_cc32xx_pin_interrupt_configure,
	.manage_callback = gpio_cc32xx_manage_callback,
};

#define GPIO_CC32XX_INIT_FUNC(n)					     \
	static int gpio_cc32xx_a##n##_init(const struct device *dev)		     \
	{								     \
		ARG_UNUSED(dev);					     \
									     \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),	     \
			gpio_cc32xx_port_isr, DEVICE_DT_INST_GET(n),         \
			0);						     \
									     \
		MAP_IntPendClear(DT_INST_IRQN(n) + 16);			     \
		irq_enable(DT_INST_IRQN(n));				     \
									     \
		return 0;						     \
	}

#define GPIO_CC32XX_DEVICE_INIT(n)					     \
	DEVICE_DT_INST_DEFINE(n, gpio_cc32xx_a##n##_init,		     \
			NULL, &gpio_cc32xx_a##n##_data,			     \
			&gpio_cc32xx_a##n##_config,			     \
			POST_KERNEL, CONFIG_GPIO_INIT_PRIORITY,		     \
			&api_funcs)

#define GPIO_CC32XX_INIT(n)						     \
	static const struct gpio_cc32xx_config gpio_cc32xx_a##n##_config = { \
		.common = {						     \
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n), \
		},							     \
		.port_base = DT_INST_REG_ADDR(n),			     \
		.port_num = n						     \
	};								     \
									     \
	static struct gpio_cc32xx_data gpio_cc32xx_a##n##_data;		     \
									     \
	GPIO_CC32XX_INIT_FUNC(n)					     \
									     \
	GPIO_CC32XX_DEVICE_INIT(n);

DT_INST_FOREACH_STATUS_OKAY(GPIO_CC32XX_INIT)
