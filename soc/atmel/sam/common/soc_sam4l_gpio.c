/*
 * Copyright (c) 2020 Gerson Fernando Budke <nandojve@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Atmel SAM MCU family General-Purpose Input/Output Controller (GPIO)
 * module HAL driver.
 */

#include <zephyr/sys/__assert.h>
#include "soc_gpio.h"

static void configure_common_attr(volatile Gpio *gpio,
				  uint32_t mask, uint32_t flags)
{
	flags &= SOC_GPIO_FLAGS_MASK;

	/* Disable interrupts on the pin(s) */
	gpio->IERC = mask;

	/* Configure pull-up(s) */
	if (flags & SOC_GPIO_PULLUP) {
		gpio->PUERS = mask;
	} else {
		gpio->PUERC = mask;
	}

	/* Configure pull-down(s) */
	if (flags & SOC_GPIO_PULLDOWN) {
		gpio->PDERS = mask;
	} else {
		gpio->PDERC = mask;
	}

	/* Configure open drain (multi-drive) */
	if (flags & SOC_GPIO_OPENDRAIN) {
		gpio->ODMERS = mask;
	} else {
		gpio->ODMERC = mask;
	}
}

static void configure_input_attr(volatile Gpio *gpio,
				 uint32_t mask, uint32_t flags)
{
	/* Configure input filter */
	if ((flags & SOC_GPIO_IN_FILTER_MASK) != 0U) {
		if ((flags & SOC_GPIO_IN_FILTER_MASK) ==
		    SOC_GPIO_IN_FILTER_DEBOUNCE) {
			/* Enable de-bounce, disable de-glitch */
			gpio->GFERC = mask;
		} else {
			/* Disable de-bounce, enable de-glitch */
			gpio->GFERS = mask;
		}
	} else {
		gpio->GFERC = mask;
	}

	/* Configure interrupt */
	if (flags & SOC_GPIO_INT_ENABLE) {
		if ((flags & SOC_GPIO_INT_TRIG_MASK) ==
		    SOC_GPIO_INT_TRIG_DOUBLE_EDGE) {
			gpio->IMR0C = mask;
			gpio->IMR1C = mask;
		} else {
			if (flags & SOC_GPIO_INT_ACTIVE_HIGH) {
				/* Rising Edge*/
				gpio->IMR0S = mask;
				gpio->IMR1C = mask;
			} else {
				/* Falling Edge */
				gpio->IMR0C = mask;
				gpio->IMR1S = mask;
			}
		}
		/* Enable interrupts on the pin(s) */
		gpio->IERS = mask;
	} else {
		gpio->IERC = mask;
	}
}

void soc_gpio_configure(const struct soc_gpio_pin *pin)
{
	uint32_t mask = pin->mask;
	volatile Gpio *gpio = pin->regs;
	uint8_t periph_id = pin->periph_id;
	uint32_t flags = pin->flags;
	uint32_t type = pin->flags & SOC_GPIO_FUNC_MASK;

	/* Configure pin attributes common to all functions */
	configure_common_attr(gpio, mask, flags);

	switch (type) {
	case SOC_GPIO_FUNC_A:
		gpio->PMR0C = mask;
		gpio->PMR1C = mask;
		gpio->PMR2C = mask;
		gpio->GPERC = mask;
		break;

	case SOC_GPIO_FUNC_B:
		gpio->PMR0S = mask;
		gpio->PMR1C = mask;
		gpio->PMR2C = mask;
		gpio->GPERC = mask;
		break;

	case SOC_GPIO_FUNC_C:
		gpio->PMR0C = mask;
		gpio->PMR1S = mask;
		gpio->PMR2C = mask;
		gpio->GPERC = mask;
		break;

	case SOC_GPIO_FUNC_D:
		gpio->PMR0S = mask;
		gpio->PMR1S = mask;
		gpio->PMR2C = mask;
		gpio->GPERC = mask;
		break;

	case SOC_GPIO_FUNC_E:
		gpio->PMR0C = mask;
		gpio->PMR1C = mask;
		gpio->PMR2S = mask;
		gpio->GPERC = mask;
		break;

	case SOC_GPIO_FUNC_F:
		gpio->PMR0S = mask;
		gpio->PMR1C = mask;
		gpio->PMR2S = mask;
		gpio->GPERC = mask;
		break;

	case SOC_GPIO_FUNC_G:
		gpio->PMR0C = mask;
		gpio->PMR1S = mask;
		gpio->PMR2S = mask;
		gpio->GPERC = mask;
		break;

	case SOC_GPIO_FUNC_H:
		gpio->PMR0S = mask;
		gpio->PMR1S = mask;
		gpio->PMR2S = mask;
		gpio->GPERC = mask;
		break;

	case SOC_GPIO_FUNC_IN:
		soc_pmc_peripheral_enable(periph_id);
		configure_input_attr(gpio, mask, flags);

		gpio->ODERC = mask;
		gpio->STERS = mask;
		gpio->GPERS = mask;
		break;

	case SOC_GPIO_FUNC_OUT_1:
	case SOC_GPIO_FUNC_OUT_0:
		if (type == SOC_GPIO_FUNC_OUT_1) {
			gpio->OVRS = mask;
		} else {
			gpio->OVRC = mask;
		}

		gpio->ODCR0S = mask;
		gpio->ODCR1S = mask;
		gpio->ODERS = mask;
		gpio->STERC = mask;
		gpio->GPERS = mask;
		break;

	default:
		__ASSERT(0, "Unsupported pin function, check pin.flags value");
		return;
	}
}

void soc_gpio_list_configure(const struct soc_gpio_pin pins[],
			     unsigned int size)
{
	for (int i = 0; i < size; i++) {
		soc_gpio_configure(&pins[i]);
	}
}
