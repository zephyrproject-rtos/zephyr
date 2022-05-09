/*
 * Copyright (c) 2019 Brett Witherspoon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc13xx_cc26xx_gpio

#include <zephyr/types.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/device.h>
#include <errno.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/dt-bindings/gpio/ti-cc13xx-cc26xx-gpio.h>

#include <driverlib/gpio.h>
#include <driverlib/interrupt.h>
#include <driverlib/ioc.h>
#include <driverlib/prcm.h>

#include <inc/hw_aon_event.h>

#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26XX.h>

#include "gpio_utils.h"

/* bits 16-18 in iocfg registers correspond to interrupt settings */
#define IOCFG_INT_MASK    0x00070000

/* the rest are for general (non-interrupt) config */
#define IOCFG_GEN_MASK    (~IOCFG_INT_MASK)

struct gpio_cc13xx_cc26xx_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	sys_slist_t callbacks;
};

static struct gpio_cc13xx_cc26xx_data gpio_cc13xx_cc26xx_data_0;

static const struct gpio_driver_config gpio_cc13xx_cc26xx_cfg_0 = {
	.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(0),
};

static int gpio_cc13xx_cc26xx_port_set_bits_raw(const struct device *port,
						uint32_t mask);
static int gpio_cc13xx_cc26xx_port_clear_bits_raw(const struct device *port,
						  uint32_t mask);

static int gpio_cc13xx_cc26xx_config(const struct device *port,
				     gpio_pin_t pin,
				     gpio_flags_t flags)
{
	uint32_t config = 0;

	__ASSERT_NO_MSG(pin < NUM_IO_MAX);

	switch (flags & (GPIO_INPUT | GPIO_OUTPUT)) {
	case GPIO_INPUT:
		config = IOC_INPUT_ENABLE;
		break;
	case GPIO_OUTPUT:
		config = IOC_INPUT_DISABLE;
		break;
	case 0:  /* disconnected */
		IOCPortConfigureSet(pin, IOC_PORT_GPIO, IOC_NO_IOPULL);
		GPIO_setOutputEnableDio(pin, GPIO_OUTPUT_DISABLE);
		return 0;
	default:
		return -ENOTSUP;
	}

	config |= IOC_SLEW_DISABLE | IOC_NO_WAKE_UP;

	config |= (flags & CC13XX_CC26XX_GPIO_DEBOUNCE) ?
		IOC_HYST_ENABLE : IOC_HYST_DISABLE;

	switch (flags & CC13XX_CC26XX_GPIO_DS_MASK) {
	case CC13XX_CC26XX_GPIO_DS_DFLT:
		config |= IOC_CURRENT_2MA | IOC_STRENGTH_AUTO;
		break;
	case CC13XX_CC26XX_GPIO_DS_ALT:
		/*
		 * Not all GPIO support 8ma, but setting that bit will use the
		 * highest supported drive strength.
		 */
		config |= IOC_CURRENT_8MA | IOC_STRENGTH_MAX;
		break;
	default:
		return -ENOTSUP;
	}

	switch (flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) {
	case 0:
		config |= IOC_NO_IOPULL;
		break;
	case GPIO_PULL_UP:
		config |= IOC_IOPULL_UP;
		break;
	case GPIO_PULL_DOWN:
		config |= IOC_IOPULL_DOWN;
		break;
	default:
		return -EINVAL;
	}

	config |= IOCPortConfigureGet(pin) & IOCFG_INT_MASK;
	IOCPortConfigureSet(pin, IOC_PORT_GPIO, config);

	if ((flags & GPIO_OUTPUT) != 0) {
		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
			gpio_cc13xx_cc26xx_port_set_bits_raw(port, BIT(pin));
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
			gpio_cc13xx_cc26xx_port_clear_bits_raw(port, BIT(pin));
		}
		GPIO_setOutputEnableDio(pin, GPIO_OUTPUT_ENABLE);
	} else {
		GPIO_setOutputEnableDio(pin, GPIO_OUTPUT_DISABLE);
	}

	return 0;
}

static int gpio_cc13xx_cc26xx_port_get_raw(const struct device *port,
					   uint32_t *value)
{
	__ASSERT_NO_MSG(value != NULL);

	*value = GPIO_readMultiDio(GPIO_DIO_ALL_MASK);

	return 0;
}

static int gpio_cc13xx_cc26xx_port_set_masked_raw(const struct device *port,
						  uint32_t mask,
						  uint32_t value)
{
	GPIO_setMultiDio(mask & value);
	GPIO_clearMultiDio(mask & ~value);

	return 0;
}

static int gpio_cc13xx_cc26xx_port_set_bits_raw(const struct device *port,
						uint32_t mask)
{
	GPIO_setMultiDio(mask);

	return 0;
}

static int gpio_cc13xx_cc26xx_port_clear_bits_raw(const struct device *port,
						  uint32_t mask)
{
	GPIO_clearMultiDio(mask);

	return 0;
}

static int gpio_cc13xx_cc26xx_port_toggle_bits(const struct device *port,
					       uint32_t mask)
{
	GPIO_toggleMultiDio(mask);

	return 0;
}

static int gpio_cc13xx_cc26xx_pin_interrupt_configure(const struct device *port,
						      gpio_pin_t pin,
						      enum gpio_int_mode mode,
						      enum gpio_int_trig trig)
{
	uint32_t config = 0;

	if (mode != GPIO_INT_MODE_DISABLED) {
		if (mode == GPIO_INT_MODE_EDGE) {
			if (trig == GPIO_INT_TRIG_BOTH) {
				config |= IOC_BOTH_EDGES;
			} else if (trig == GPIO_INT_TRIG_HIGH) {
				config |= IOC_RISING_EDGE;
			} else { /* GPIO_INT_TRIG_LOW */
				config |= IOC_FALLING_EDGE;
			}
		} else {
			return -ENOTSUP;
		}

		config |= IOC_INT_ENABLE;
	} else {
		config |= IOC_INT_DISABLE | IOC_NO_EDGE;
	}

	config |= IOCPortConfigureGet(pin) & IOCFG_GEN_MASK;
	IOCPortConfigureSet(pin, IOC_PORT_GPIO, config);

	return 0;
}

static int gpio_cc13xx_cc26xx_manage_callback(const struct device *port,
					      struct gpio_callback *callback,
					      bool set)
{
	struct gpio_cc13xx_cc26xx_data *data = port->data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}

static uint32_t gpio_cc13xx_cc26xx_get_pending_int(const struct device *dev)
{
	return GPIO_getEventMultiDio(GPIO_DIO_ALL_MASK);
}

static void gpio_cc13xx_cc26xx_isr(const struct device *dev)
{
	struct gpio_cc13xx_cc26xx_data *data = dev->data;

	uint32_t status = GPIO_getEventMultiDio(GPIO_DIO_ALL_MASK);

	GPIO_clearEventMultiDio(status);

	gpio_fire_callbacks(&data->callbacks, dev, status);
}

static int gpio_cc13xx_cc26xx_init(const struct device *dev)
{
#ifdef CONFIG_PM
	/* Set dependency on gpio resource to turn on power domains */
	Power_setDependency(PowerCC26XX_PERIPH_GPIO);
#else
	/* Enable peripheral power domain */
	PRCMPowerDomainOn(PRCM_DOMAIN_PERIPH);

	/* Enable GPIO peripheral */
	PRCMPeripheralRunEnable(PRCM_PERIPH_GPIO);

	/* Load PRCM settings */
	PRCMLoadSet();
	while (!PRCMLoadGet()) {
		continue;
	}
#endif

	/* Enable edge detection on any pad as a wakeup source */
	HWREG(AON_EVENT_BASE + AON_EVENT_O_MCUWUSEL) =
		(HWREG(AON_EVENT_BASE + AON_EVENT_O_MCUWUSEL) &
		(~AON_EVENT_MCUWUSEL_WU1_EV_M)) |
		AON_EVENT_MCUWUSEL_WU1_EV_PAD;

	/* Enable IRQ */
	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    gpio_cc13xx_cc26xx_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));

	/* Peripheral should not be accessed until power domain is on. */
	while (PRCMPowerDomainStatus(PRCM_DOMAIN_PERIPH) !=
	       PRCM_DOMAIN_POWER_ON) {
		continue;
	}

	return 0;
}

static const struct gpio_driver_api gpio_cc13xx_cc26xx_driver_api = {
	.pin_configure = gpio_cc13xx_cc26xx_config,
	.port_get_raw = gpio_cc13xx_cc26xx_port_get_raw,
	.port_set_masked_raw = gpio_cc13xx_cc26xx_port_set_masked_raw,
	.port_set_bits_raw = gpio_cc13xx_cc26xx_port_set_bits_raw,
	.port_clear_bits_raw = gpio_cc13xx_cc26xx_port_clear_bits_raw,
	.port_toggle_bits = gpio_cc13xx_cc26xx_port_toggle_bits,
	.pin_interrupt_configure = gpio_cc13xx_cc26xx_pin_interrupt_configure,
	.manage_callback = gpio_cc13xx_cc26xx_manage_callback,
	.get_pending_int = gpio_cc13xx_cc26xx_get_pending_int
};

DEVICE_DT_INST_DEFINE(0, gpio_cc13xx_cc26xx_init,
		    NULL, &gpio_cc13xx_cc26xx_data_0,
		    &gpio_cc13xx_cc26xx_cfg_0,
		    PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY,
		    &gpio_cc13xx_cc26xx_driver_api);
