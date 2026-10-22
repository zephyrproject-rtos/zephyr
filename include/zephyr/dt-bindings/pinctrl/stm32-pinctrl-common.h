/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_STM32_PINCTRL_COMMON_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_STM32_PINCTRL_COMMON_H_

/**
 * @brief numerical IDs for IO ports
 */

#define	STM32_PORTA 0	/* IO port A */
#define	STM32_PORTB 1	/* .. */
#define	STM32_PORTC 2
#define	STM32_PORTD 3
#define	STM32_PORTE 4
#define	STM32_PORTF 5
#define	STM32_PORTG 6
#define	STM32_PORTH 7
#define	STM32_PORTI 8
#define	STM32_PORTJ 9
#define	STM32_PORTK 10	/* IO port K */
#define	STM32_PORTM 12	/* IO port M (0xC) */
#define	STM32_PORTN 13
#define	STM32_PORTO 14
#define	STM32_PORTP 15	/* IO port P (0xF) */
#define	STM32_PORTQ 16	/* IO port Q (0x10) */
#define	STM32_PORTZ 25  /* IO port Z (0x19) */

#ifndef STM32_PORTS_MAX
#define STM32_PORTS_MAX (STM32_PORTZ + 1)
#endif

/**
 * @brief helper macro to encode an IO port pin in a numerical format
 */
#define STM32PIN(_port, _pin) \
	(_port << 4 | _pin)

#endif	/* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_STM32_PINCTRL_COMMON_H_ */
