/*
 * Copyright (c) 2016-2017 Piotr Mienkowski
 * Copyright (c) 2020-2023 Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Atmel SAM MCU family General Purpose Input Output (GPIO)
 * module HAL driver.
 */

#ifndef _ATMEL_SAM_SOC_GPIO_H_
#define _ATMEL_SAM_SOC_GPIO_H_

#include <zephyr/types.h>
#include <soc.h>

/*
 * Pin flags/attributes
 */

/* TODO: replace hard coded pin attribute values with defines provided
 * in gpio.h, once the official API is clean.
 */

#define SOC_GPIO_DEFAULT                (0)

#define SOC_GPIO_FLAGS_POS              (0)
#define SOC_GPIO_FLAGS_SIZE             (4)
#define SOC_GPIO_FLAGS_MASK             (0x0FU << SOC_GPIO_FLAGS_POS)
#define SOC_GPIO_PULLUP_POS             (0)
#define SOC_GPIO_PULLUP                 (1 << SOC_GPIO_PULLUP_POS)
#define SOC_GPIO_PULLDOWN_POS           (1)
#define SOC_GPIO_PULLDOWN               (1 << SOC_GPIO_PULLDOWN_POS)
#define SOC_GPIO_OPENDRAIN_POS          (2)
#define SOC_GPIO_OPENDRAIN              (1 << SOC_GPIO_OPENDRAIN_POS)
#define SOC_GPIO_WAKEUP_POS             (3)
#define SOC_GPIO_WAKEUP                 (1 << SOC_GPIO_WAKEUP_POS)

/* Bit field: SOC_GPIO_IN_FILTER */
#define SOC_GPIO_IN_FILTER_POS          (SOC_GPIO_FLAGS_POS + SOC_GPIO_FLAGS_SIZE)
#define SOC_GPIO_IN_FILTER_SIZE         (2)
#define SOC_GPIO_IN_FILTER_MASK         (0x03U << SOC_GPIO_IN_FILTER_POS)
#define SOC_GPIO_IN_FILTER_NONE         (0 << SOC_GPIO_IN_FILTER_POS)
#define SOC_GPIO_IN_FILTER_DEBOUNCE     (1 << SOC_GPIO_IN_FILTER_POS)
#define SOC_GPIO_IN_FILTER_DEGLITCH     (2 << SOC_GPIO_IN_FILTER_POS)

#define SOC_GPIO_INT_POS                (SOC_GPIO_IN_FILTER_POS + SOC_GPIO_IN_FILTER_SIZE)
#define SOC_GPIO_INT_SIZE               (1)
#define SOC_GPIO_INT_ENABLE             (1 << SOC_GPIO_INT_POS)

/* Bit field: SOC_GPIO_INT_TRIG */
#define SOC_GPIO_INT_TRIG_POS           (SOC_GPIO_INT_POS + SOC_GPIO_INT_SIZE)
#define SOC_GPIO_INT_TRIG_SIZE          (2)
#define SOC_GPIO_INT_TRIG_MASK          (0x03U << SOC_GPIO_INT_TRIG_POS)
/** Interrupt is triggered by a level detection event. */
#define SOC_GPIO_INT_TRIG_LEVEL         (0 << SOC_GPIO_INT_TRIG_POS)
/** Interrupt is triggered by an edge detection event. */
#define SOC_GPIO_INT_TRIG_EDGE          (1 << SOC_GPIO_INT_TRIG_POS)
/** Interrupt is triggered by any edge detection event. */
#define SOC_GPIO_INT_TRIG_DOUBLE_EDGE   (2 << SOC_GPIO_INT_TRIG_POS)

/** Interrupt is triggered by a high level / rising edge detection event */
#define SOC_GPIO_INT_ACTIVE_POS         (SOC_GPIO_INT_TRIG_POS + SOC_GPIO_INT_TRIG_SIZE)
#define SOC_GPIO_INT_ACTIVE_SIZE        (1)
#define SOC_GPIO_INT_ACTIVE_HIGH        (1 << SOC_GPIO_INT_ACTIVE_POS)

/* Bit field: SOC_GPIO_FUNC */
#define SOC_GPIO_FUNC_POS               (16)
#define SOC_GPIO_FUNC_MASK              (0x0F << SOC_GPIO_FUNC_POS)
/** Connect pin to peripheral A. */
#define SOC_GPIO_FUNC_A                 (0 << SOC_GPIO_FUNC_POS)
/** Connect pin to peripheral B. */
#define SOC_GPIO_FUNC_B                 (1 << SOC_GPIO_FUNC_POS)
/** Connect pin to peripheral C. */
#define SOC_GPIO_FUNC_C                 (2 << SOC_GPIO_FUNC_POS)
/** Connect pin to peripheral D. */
#define SOC_GPIO_FUNC_D                 (3 << SOC_GPIO_FUNC_POS)
/** Connect pin to peripheral E. */
#define SOC_GPIO_FUNC_E                 (4 << SOC_GPIO_FUNC_POS)
/** Connect pin to peripheral F. */
#define SOC_GPIO_FUNC_F                 (5 << SOC_GPIO_FUNC_POS)
/** Connect pin to peripheral G. */
#define SOC_GPIO_FUNC_G                 (6 << SOC_GPIO_FUNC_POS)
/** Connect pin to peripheral H. */
#define SOC_GPIO_FUNC_H                 (7 << SOC_GPIO_FUNC_POS)
/** Configure pin as wakeup input source. */
#define SOC_GPIO_FUNC_WAKEUP            (8 << SOC_GPIO_FUNC_POS)
/** Configure pin as input. */
#define SOC_GPIO_FUNC_IN                (9 << SOC_GPIO_FUNC_POS)
/** Configure pin as output and set it initial value to 0. */
#define SOC_GPIO_FUNC_OUT_0             (10 << SOC_GPIO_FUNC_POS)
/** Configure pin as output and set it initial value to 1. */
#define SOC_GPIO_FUNC_OUT_1             (11 << SOC_GPIO_FUNC_POS)

struct soc_gpio_pin {
	uint32_t mask;     /** pin(s) bit mask */
#ifdef ID_GPIO
	Gpio *regs;        /** pointer to registers of the GPIO controller */
#else
	Pio *regs;         /** pointer to registers of the PIO controller */
#endif
	uint8_t periph_id; /** peripheral ID of the PIO controller */
	uint32_t flags;    /** pin flags/attributes */
};

/**
 * @brief Configure GPIO pin(s).
 *
 * Configure one or several pins belonging to the same GPIO port.
 * Example scenarios:
 * - configure pin(s) as input with debounce filter enabled.
 * - connect pin(s) to a peripheral B and enable pull-up.
 * - configure pin(s) as open drain output.
 * All pins are configured in the same way.
 *
 * @remark The function will enable the GPIO module's clock only if
 * any of its pins is configured as an input. This is typically what
 * a user wants. A pin will function correctly without clock enabled
 * when configured as an output or connected to a peripheral.
 * In some cases, e.g. when a pin is configured as an output with
 * a pull-up and user wants to read pin's input value it is necessary
 * to enable GPIO module's clock separately.
 *
 * @param pin  pin's configuration data such as pin mask, pin attributes, etc.
 */
void soc_gpio_configure(const struct soc_gpio_pin *pin);

/**
 * @brief Configure a list of GPIO pin(s).
 *
 * Configure an arbitrary amount of pins in an arbitrary way. Each
 * configuration entry is a single item in an array passed as an
 * argument to the function.
 *
 * @param pins an array where each item contains pin's configuration data.
 * @param size size of the pin list.
 */
void soc_gpio_list_configure(const struct soc_gpio_pin pins[],
			     unsigned int size);

/**
 * @brief Set pin(s) high.
 *
 * Set pin(s) defined in the mask parameter to high. The pin(s) have to be
 * configured as output by the configure function. The flags field which
 * is part of pin struct is ignored.
 *
 * @param pin  pointer to a pin instance describing one or more pins.
 */
static inline void soc_gpio_set(const struct soc_gpio_pin *pin)
{
#ifdef ID_GPIO
	pin->regs->OVRS = pin->mask;
#else
	pin->regs->PIO_SODR = pin->mask;
#endif
}

/**
 * @brief Set pin(s) low.
 *
 * Set pin(s) defined in the mask field to low. The pin(s) have to be
 * configured as output by the configure function. The flags field which
 * is part of pin struct is ignored.
 *
 * @param pin  pointer to a pin instance describing one or more pins.
 */
static inline void soc_gpio_clear(const struct soc_gpio_pin *pin)
{
#ifdef ID_GPIO
	pin->regs->OVRC = pin->mask;
#else
	pin->regs->PIO_CODR = pin->mask;
#endif
}

/**
 * @brief Get pin(s) value.
 *
 * Get value of the pin(s) defined in the mask field.
 *
 * @param pin  pointer to a pin instance describing one or more pins.
 * @return pin(s) value. To assess value of a specific pin the pin's bit
 * field has to be read.
 */
static inline uint32_t soc_gpio_get(const struct soc_gpio_pin *pin)
{
#ifdef ID_GPIO
	return pin->regs->PVR & pin->mask;
#else
	return pin->regs->PIO_PDSR & pin->mask;
#endif
}

/**
 * @brief Set the length of the debounce window.
 *
 * The debouncing filter automatically rejects a pulse with a duration of less
 * than 1/2 programmable divided slow clock period tdiv_slck, while a pulse with
 * a duration of one or more tdiv_slck cycles is accepted. For pulse durations
 * between 1/2 selected clock cycle and one tdiv_slck clock cycle, the pulse may
 * or may not be taken into account, depending on the precise timing of its
 * occurrence.
 *
 * tdiv_slck = ((div + 1) x 2) x tslck
 * where tslck is the slow clock, typically 32.768 kHz.
 *
 * Setting the length of the debounce window is only meaningful if the pin is
 * configured as input and the debounce pin option is enabled.
 *
 * @param pin  pointer to a pin instance describing one or more pins.
 * @param div  slow clock divider, valid values: from 0 to 2^14 - 1
 */
static inline void soc_gpio_debounce_length_set(const struct soc_gpio_pin *pin,
						uint32_t div)
{
#ifdef ID_GPIO
	if (div) {
		pin->regs->STERS = pin->mask;
	} else {
		pin->regs->STERC = pin->mask;
	}
#else
	pin->regs->PIO_SCDR = PIO_SCDR_DIV(div);
#endif
}

#endif /* _ATMEL_SAM_SOC_GPIO_H_ */
