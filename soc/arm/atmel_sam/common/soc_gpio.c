/*
 * Copyright (c) 2016 Piotr Mienkowski
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Atmel SAM MCU family General Purpose Input Output (GPIO)
 * module HAL driver.
 */

#include <sys/__assert.h>
#include "soc_gpio.h"

/*
 * There exist minor differences between SAM MCU family members in naming
 * of some of the registers. Check that our expectations are met.
 */
#if (!defined(PIO_IFSCER_P0) && !defined(PIO_DIFSR_P0)) \
	|| (!defined(PIO_IFSCDR_P0) && !defined(PIO_SCIFSR_P0)) \
	|| (!defined(PIO_ABCDSR_P0) && !defined(PIO_ABSR_P0))
#error "Unsupported Atmel SAM MCU series"
#endif

static void configure_common_attr(Pio *pio, uint32_t mask, uint32_t flags)
{
	/* Disable interrupts on the pin(s) */
	pio->PIO_IDR = mask;

	/* Configure pull-up(s) */
	if (flags & SOC_GPIO_PULLUP) {
		pio->PIO_PUER = mask;
	} else {
		pio->PIO_PUDR = mask;
	}

/* Configure pull-down only for MCU series that support it */
#if defined PIO_PPDER_P0
	/* Configure pull-down(s) */
	if (flags & SOC_GPIO_PULLDOWN) {
		pio->PIO_PPDER = mask;
	} else {
		pio->PIO_PPDDR = mask;
	}
#endif

	/* Configure open drain (multi-drive) */
	if (flags & SOC_GPIO_OPENDRAIN) {
		pio->PIO_MDER = mask;
	} else {
		pio->PIO_MDDR = mask;
	}
}

static void configure_input_attr(Pio *pio, uint32_t mask, uint32_t flags)
{
	/* Configure input filter */
	if ((flags & SOC_GPIO_IN_FILTER_MASK) != 0U) {
		if ((flags & SOC_GPIO_IN_FILTER_MASK) == SOC_GPIO_IN_FILTER_DEBOUNCE) {
			/* Enable de-bounce, disable de-glitch */
#if defined PIO_IFSCER_P0
			pio->PIO_IFSCER = mask;
#elif defined PIO_DIFSR_P0
			pio->PIO_DIFSR = mask;
#endif
		} else {
			/* Disable de-bounce, enable de-glitch */
#if defined PIO_IFSCDR_P0
			pio->PIO_IFSCDR = mask;
#elif defined PIO_SCIFSR_P0
			pio->PIO_SCIFSR = mask;
#endif
		}
		pio->PIO_IFER = mask;
	} else {
		pio->PIO_IFDR = mask;
	}

	/* Configure interrupt */
	if (flags & SOC_GPIO_INT_ENABLE) {
		if ((flags & SOC_GPIO_INT_TRIG_MASK) == SOC_GPIO_INT_TRIG_DOUBLE_EDGE) {
			/* Disable additional interrupt modes, enable the default */
			pio->PIO_AIMDR = mask;
		} else {
			/* Configure additional interrupt mode */
			if ((flags & SOC_GPIO_INT_TRIG_MASK) == SOC_GPIO_INT_TRIG_EDGE) {
				/* Select edge detection event */
				pio->PIO_ESR = mask;
			} else {
				/* Select level detection event */
				pio->PIO_LSR = mask;
			}

			if (flags & SOC_GPIO_INT_ACTIVE_HIGH) {
				pio->PIO_REHLSR = mask;
			} else {
				pio->PIO_FELLSR = mask;
			}
			/* Enable additional interrupt mode */
			pio->PIO_AIMER = mask;
		}
		/* Enable interrupts on the pin(s) */
		pio->PIO_IER = mask;
	} else {
		/* Nothing to do. All interrupts were disabled in the
		 * beginning.
		 */
	}
}

static void configure_output_attr(Pio *pio, uint32_t mask, uint32_t flags)
{
	/* Enable control of the I/O line by the PIO_ODSR register */
	pio->PIO_OWER = mask;
}

void soc_gpio_configure(const struct soc_gpio_pin *pin)
{
	uint32_t mask = pin->mask;
	Pio *pio = pin->regs;
	uint8_t periph_id = pin->periph_id;
	uint32_t flags = pin->flags;
	uint32_t type = pin->flags & SOC_GPIO_FUNC_MASK;

	/* Configure pin attributes common to all functions */
	configure_common_attr(pio, mask, flags);

	switch (type) {
	case SOC_GPIO_FUNC_A:
#if defined PIO_ABCDSR_P0
		pio->PIO_ABCDSR[0] &= ~mask;
		pio->PIO_ABCDSR[1] &= ~mask;
#elif defined PIO_ABSR_P0
		pio->PIO_ABSR &= ~mask;
#endif
		/* Connect pin to the peripheral (disconnect PIO block) */
		pio->PIO_PDR = mask;
		break;

	case SOC_GPIO_FUNC_B:
#if defined PIO_ABCDSR_P0
		pio->PIO_ABCDSR[0] |= mask;
		pio->PIO_ABCDSR[1] &= ~mask;
#elif defined PIO_ABSR_P0
		pio->PIO_ABSR |= mask;
#endif
		/* Connect pin to the peripheral (disconnect PIO block) */
		pio->PIO_PDR = mask;
		break;

#if defined PIO_ABCDSR_P0
	case SOC_GPIO_FUNC_C:
		pio->PIO_ABCDSR[0] &= ~mask;
		pio->PIO_ABCDSR[1] |= mask;
		/* Connect pin to the peripheral (disconnect PIO block) */
		pio->PIO_PDR = mask;
		break;

	case SOC_GPIO_FUNC_D:
		pio->PIO_ABCDSR[0] |= mask;
		pio->PIO_ABCDSR[1] |= mask;
		/* Connect pin to the peripheral (disconnect PIO block) */
		pio->PIO_PDR = mask;
		break;
#endif

	case SOC_GPIO_FUNC_IN:
		/* Enable module's clock */
		soc_pmc_peripheral_enable(periph_id);
		/* Configure pin attributes related to input function */
		configure_input_attr(pio, mask, flags);
		/* Configure pin as input */
		pio->PIO_ODR = mask;
		pio->PIO_PER = mask;
		break;

	case SOC_GPIO_FUNC_OUT_1:
	case SOC_GPIO_FUNC_OUT_0:
		/* Set initial pin value */
		if (type == SOC_GPIO_FUNC_OUT_1) {
			pio->PIO_SODR = mask;
		} else {
			pio->PIO_CODR = mask;
		}

		/* Configure pin attributes related to output function */
		configure_output_attr(pio, mask, flags);
		/* Configure pin(s) as output(s) */
		pio->PIO_OER = mask;
		pio->PIO_PER = mask;
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
