/*
 * Copyright (c) 2017 Piotr Mienkowski
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Atmel SAM3X MCU pin definitions.
 *
 * This file contains pin configuration data required by different MCU
 * modules to correctly configure GPIO controller.
 */

#ifndef _ATMEL_SAM3X_SOC_PINMAP_H_
#define _ATMEL_SAM3X_SOC_PINMAP_H_

#include <soc.h>

/* Two-wire Interface (TWI) */

#define PIN_TWI0_TWCK {PIO_PA18A_TWCK0, PIOA, ID_PIOA, SOC_GPIO_FUNC_A}
#define PIN_TWI0_TWD  {PIO_PA17A_TWD0,  PIOA, ID_PIOA, SOC_GPIO_FUNC_A}

#define PINS_TWI0 {PIN_TWI0_TWCK, PIN_TWI0_TWD}

#define PIN_TWI1_TWCK {PIO_PB13A_TWCK1, PIOB, ID_PIOB, SOC_GPIO_FUNC_A}
#define PIN_TWI1_TWD  {PIO_PB12A_TWD1,  PIOB, ID_PIOB, SOC_GPIO_FUNC_A}

#define PINS_TWI1 {PIN_TWI1_TWCK, PIN_TWI1_TWD}

#endif /* _ATMEL_SAM3X_SOC_PINMAP_H_ */
