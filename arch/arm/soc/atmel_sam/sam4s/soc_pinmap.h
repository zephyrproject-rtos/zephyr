/*
 * Copyright (c) 2017 Justin Watson
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Atmel SAM4S MCU pin definitions.
 *
 * This file contains pin configuration data required by different MCU
 * modules to correctly configure GPIO controller.
 */

#ifndef _ATMEL_SAM4S_SOC_PINMAP_H_
#define _ATMEL_SAM4S_SOC_PINMAP_H_

#include <soc.h>

/* Universal Asynchronous Receiver Transmitter (UART) */

#define PIN_UART0_RXD {PIO_PA9A_URXD0, PIOA, ID_PIOA, SOC_GPIO_FUNC_A}
#define PIN_UART0_TXD {PIO_PA10A_UTXD0, PIOA, ID_PIOA, SOC_GPIO_FUNC_A}

#define PINS_UART0 {PIN_UART0_RXD, PIN_UART0_TXD}

#define PIN_UART1_RXD {PIO_PB2A_URXD1, PIOB, ID_PIOB, SOC_GPIO_FUNC_A}
#define PIN_UART1_TXD {PIO_PB3A_UTXD1, PIOB, ID_PIOB, SOC_GPIO_FUNC_A}

#define PINS_UART1 {PIN_UART1_RXD, PIN_UART1_TXD}

#endif /* _ATMEL_SAM4S_SOC_PINMAP_H_ */
