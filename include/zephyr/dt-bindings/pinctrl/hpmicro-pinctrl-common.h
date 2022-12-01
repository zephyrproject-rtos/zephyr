/*
 * Copyright (c) 2022 HPMicro
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_HPMICRO_PINCTRL_COMMON_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_HPMICRO_PINCTRL_COMMON_H_

#include <zephyr/sys/util_macro.h>

/**
 * @brief Pin numbers of gpio
 *
 */
#define HPMICRO_PIN_NUM_SHIFT		0U
#define HPMICRO_PIN_NUM_MASK		BIT_MASK(10)

/**
 * @brief io peripheral function
 *
 */
#define HPMICRO_PIN_ANALOG_MASK		BIT_MASK(0)
#define HPMICRO_PIN_ANALOG_SHIFT	10U
#define HPMICRO_PIN_ALT_MASK		BIT_MASK(5)
#define HPMICRO_PIN_ALT_SHIFT		11U
#define HPMICRO_PIN_IOC_MASK		BIT_MASK(2)
#define HPMICRO_PIN_IOC_SHIFT		30U

/**
 * @brief numerical ioc_pad for IO ports
 */

#define	HPMICRO_PORTA 0		/* IO port A */
#define	HPMICRO_PORTB 32	/* .. */
#define	HPMICRO_PORTC 64
#define	HPMICRO_PORTD 96
#define	HPMICRO_PORTE 128
#define	HPMICRO_PORTF 160
#define	HPMICRO_PORTX 416
#define	HPMICRO_PORTY 448
#define	HPMICRO_PORTZ 480	/* IO port Z */

/**
 * @brief ioc controller mode
 *
 */
#define IOC_TYPE_IOC 0
#define IOC_TYPE_BIOC 1
#define IOC_TYPE_PIOC 2

/**
 * @brief helper macro to encode an IO port pin in a numerical format
 */

#define HPMICRO_PIN(_port, _pin) \
	(_port + _pin)

/**
 * @brief macro generation codes to select pin functions
 * pin: HPMICRO_PIN(port, pin)
 * is_analog: 1: enable analog, 0: disable analog
 * alt: 0 - 31 Function Selection alt0-alt31
 * ioc: IOC_TYPE_IOC IOC_TYPE_BIOC IOC_TYPE_PIOC
 */
#define HPMICRO_PINMUX_1(pin, ioc, is_analog, alt)						\
		(((pin & HPMICRO_PIN_NUM_MASK) << HPMICRO_PIN_NUM_SHIFT) |		\
		((is_analog & HPMICRO_PIN_ANALOG_MASK) << HPMICRO_PIN_ANALOG_SHIFT) |	\
		((alt & HPMICRO_PIN_ALT_MASK) << HPMICRO_PIN_ALT_SHIFT) | \
		((ioc & HPMICRO_PIN_IOC_MASK) << HPMICRO_PIN_IOC_SHIFT))

/**
 * @brief macro generation codes to select pin functions
 * port: HPMICRO_PORTA - HPMICRO_PORTZ
 * pin: 0-32
 * is_analog: 1: enable analog, 0: disable analog
 * alt: 0 - 31 Function Selection alt0-alt31
 * ioc: IOC_TYPE_IOC IOC_TYPE_BIOC IOC_TYPE_PIOC
 */
#define HPMICRO_PINMUX(port, pin, ioc, is_analog, alt) \
		HPMICRO_PINMUX_1(HPMICRO_PIN(port, pin), ioc, is_analog, alt)

#endif	/* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_HPMICRO_PINCTRL_COMMON_H_ */
