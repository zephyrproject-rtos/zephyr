/*
 * Copyright (c) 2024 Texas Instruments Incorporated
 * Copyright (c) 2024 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc23x0_gpio

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>

#include <driverlib/clkctl.h>
#include <driverlib/gpio.h>
#include <inc/hw_ioc.h>

#define IOC_ADDR(index)       (IOC_BASE + IOC_O_IOC0 + (sizeof(uint32_t) * (index)))

struct gpio_cc23x0_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
};

struct gpio_cc23x0_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	sys_slist_t callbacks;
};

static void set_pin_mask_non_atomic(uint8_t index, uint32_t registerBaseAddress)
{
	GPIOSetConfigDio(GPIO_BASE + registerBaseAddress, BIT(index));
}

static int gpio_cc23x0_config(const struct device *port, gpio_pin_t pin, gpio_flags_t flags)
{
	uint32_t config = 0;
	uint32_t iocfg_reg = IOC_ADDR(pin);
	gpio_flags_t direction = flags & GPIO_DIR_MASK;

	if (flags & GPIO_PULL_UP) {
		config |= IOC_IOC0_PULLCTL_PULL_UP;
	} else if (flags & GPIO_PULL_DOWN) {
		config |= IOC_IOC0_PULLCTL_PULL_DOWN;
	} else {
		config |= IOC_IOC0_PULLCTL_PULL_DIS;
	}

	if (!(flags & GPIO_SINGLE_ENDED)) {
		config |= IOC_IOC0_IOMODE_NORMAL;
	} else {
		if (flags & GPIO_LINE_OPEN_DRAIN) {
			config |= IOC_IOC0_IOMODE_OPEND;
		} else {
			config |= IOC_IOC0_IOMODE_OPENS;
		}
	}
	if (direction & GPIO_INPUT) {
		config |= IOC_IOC0_INPEN_EN | IOC_IOC0_HYSTEN_EN;
	}

	GPIOSetConfigDio(iocfg_reg, config);

	if (flags & GPIO_OUTPUT) {
		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			GPIOSetDio(pin);
		} else if (flags & GPIO_OUTPUT_INIT_LOW) {
			GPIOClearDio(pin);
		}
		GPIOSetOutputEnableDio(pin, GPIO_OUTPUT_ENABLE);
	} else {
		GPIOSetOutputEnableDio(pin, GPIO_OUTPUT_DISABLE);
	}
	return 0;
}

#ifdef CONFIG_GPIO_GET_CONFIG
static int gpio_cc23x0_get_config(const struct device *port, gpio_pin_t pin, gpio_flags_t *flags)
{
	uint32_t out_flag = 0;
	uint32_t iocfg_reg = IOC_ADDR(pin);
	uint32_t config = GPIOGetConfigDio(iocfg_reg);

	/* GPIO input/output configuration flags */
	if (config & IOC_IOC0_INPEN_EN) {
		out_flag |= GPIO_INPUT;
	}

	if (GPIOGetOutputEnableDio(pin)) {
		out_flag |= GPIO_OUTPUT;

		if (GPIOReadDio(pin)) {
			out_flag |= GPIO_OUTPUT_INIT_HIGH;
		} else {
			/* This is the default value. If not explicitly set,
			 * the returned config will not be symmetric
			 */
			out_flag |= GPIO_OUTPUT_INIT_LOW;
		}
	}

	/* GPIO interrupt configuration flags */
	if ((config & IOC_IOC0_EDGEDET_M) != IOC_IOC0_EDGEDET_EDGE_DIS) {
		if (config & IOC_IOC0_EDGEDET_EDGE_POS) {
			out_flag |= GPIO_INT_EDGE_RISING;
		}

		if (config & IOC_IOC0_EDGEDET_EDGE_NEG) {
			out_flag |= GPIO_INT_EDGE_FALLING;
		}
	} else {
		/* This is the default value. If not explicitly set,
		 * the returned config will not be symmetric
		 */
		out_flag |= GPIO_INT_DISABLE;
	}

	/* GPIO pin drive flags */
	if (config & IOC_IOC0_IOMODE_OPENS) {
		out_flag |= GPIO_OPEN_SOURCE;
	}

	if (config & IOC_IOC0_IOMODE_OPEND) {
		out_flag |= IOC_IOC0_IOMODE_OPEND;
	}

	if (config & IOC_IOC0_PULLCTL_PULL_UP) {
		out_flag |= GPIO_PULL_UP;
	}

	if (config & IOC_IOC0_PULLCTL_PULL_DOWN) {
		out_flag |= GPIO_PULL_DOWN;
	}

	*flags = out_flag;

	return 0;
}
#endif

static int gpio_cc23x0_port_get_raw(const struct device *port, uint32_t *value)
{
	*value = GPIOReadMultiDio(GPIO_DIO_ALL_MASK);

	return 0;
}

static int gpio_cc23x0_port_set_masked_raw(const struct device *port, uint32_t mask, uint32_t value)
{
	GPIOWriteMultiDio(mask, value);

	return 0;
}

static int gpio_cc23x0_port_set_bits_raw(const struct device *port, uint32_t mask)
{
	GPIOSetMultiDio(mask);

	return 0;
}

static int gpio_cc23x0_port_clear_bits_raw(const struct device *port, uint32_t mask)
{
	GPIOClearMultiDio(mask);

	return 0;
}

static int gpio_cc23x0_port_toggle_bits(const struct device *port, uint32_t mask)
{
	GPIOToggleMultiDio(mask);

	return 0;
}

static int gpio_cc23x0xx_pin_interrupt_configure(const struct device *port, gpio_pin_t pin,
						 enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	if (mode == GPIO_INT_MODE_LEVEL) {
		return ENOTSUP;
	}

	uint32_t config = GPIOGetConfigDio(IOC_ADDR(pin)) & ~IOC_IOC0_EDGEDET_M;

	if (mode == GPIO_INT_MODE_DISABLED) {
		config |= IOC_IOC1_EDGEDET_EDGE_DIS;

		GPIOSetConfigDio(IOC_ADDR(pin), config);

		/* Disable interrupt mask */
		set_pin_mask_non_atomic(pin, GPIO_O_IMCLR);

	} else if (mode == GPIO_INT_MODE_EDGE) {
		switch (trig) {
		case GPIO_INT_TRIG_LOW:
			config |= IOC_IOC1_EDGEDET_EDGE_NEG;
			break;
		case GPIO_INT_TRIG_HIGH:
			config |= IOC_IOC1_EDGEDET_EDGE_POS;
			break;
		case GPIO_INT_TRIG_BOTH:
			config |= IOC_IOC1_EDGEDET_EDGE_BOTH;
			break;
		default:
			return ENOTSUP;
		}

		GPIOSetConfigDio(IOC_ADDR(pin), config);

		/* Enable interrupt mask */
		set_pin_mask_non_atomic(pin, GPIO_O_ICLR);
		set_pin_mask_non_atomic(pin, GPIO_O_IMSET);
	}

	return 0;
}

static int gpio_cc23x0_manage_callback(const struct device *port, struct gpio_callback *callback,
				       bool set)
{
	struct gpio_cc23x0_data *data = port->data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}

static uint32_t gpio_cc23x0_get_pending_int(const struct device *dev)
{
	return GPIOGetEventMultiDio(GPIO_DIO_ALL_MASK);
}

static void gpio_cc23x0_isr(const struct device *dev)
{
	struct gpio_cc23x0_data *data = dev->data;

	uint32_t status = GPIOGetEventMultiDio(GPIO_DIO_ALL_MASK);

	GPIOClearEventMultiDio(status);

	gpio_fire_callbacks(&data->callbacks, dev, status);
}

static int gpio_cc23x0_init(const struct device *dev)
{
	/* Enable GPIO domain clock */
	CLKCTLEnable(CLKCTL_BASE, CLKCTL_GPIO);

	/* Enable IRQ */
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), gpio_cc23x0_isr,
		    DEVICE_DT_INST_GET(0), 0);

	irq_enable(DT_INST_IRQN(0));

	return 0;
}

static DEVICE_API(gpio, gpio_cc23x0_driver_api) = {
	.pin_configure = gpio_cc23x0_config,
#ifdef CONFIG_GPIO_GET_CONFIG
	.pin_get_config = gpio_cc23x0_get_config,
#endif
	.port_get_raw = gpio_cc23x0_port_get_raw,
	.port_set_masked_raw = gpio_cc23x0_port_set_masked_raw,
	.port_set_bits_raw = gpio_cc23x0_port_set_bits_raw,
	.port_clear_bits_raw = gpio_cc23x0_port_clear_bits_raw,
	.port_toggle_bits = gpio_cc23x0_port_toggle_bits,
	.pin_interrupt_configure = gpio_cc23x0xx_pin_interrupt_configure,
	.manage_callback = gpio_cc23x0_manage_callback,
	.get_pending_int = gpio_cc23x0_get_pending_int,
};

static const struct gpio_cc23x0_config gpio_cc23x0_config_0 = {
	.common = {/* Read ngpios from DT */
		   .port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(0)}};

static struct gpio_cc23x0_data gpio_cc23x0_data_0;

DEVICE_DT_INST_DEFINE(0, gpio_cc23x0_init, NULL, &gpio_cc23x0_data_0, &gpio_cc23x0_config_0,
		      PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY, &gpio_cc23x0_driver_api);
