/*
 * Copyright (c) 2019 Gerson Fernando Budke
 * Copyright (c) 2016-2017 Piotr Mienkowski
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Atmel SAM V71 MCU pin definitions.
 *
 * This file contains pin configuration data required by different MCU
 * modules to correctly configure GPIO controller.
 */

#ifndef _ATMEL_SAM_SOC_PINMAP_H_
#define _ATMEL_SAM_SOC_PINMAP_H_

#include <soc.h>

/* Synchronous Serial Controller (SSC) */

#define PIN_SSC0_RD {PIO_PA10C_SSC_RD, PIOA, ID_PIOA, SOC_GPIO_FUNC_C}
#define PIN_SSC0_RF {PIO_PD24B_SSC_RF, PIOD, ID_PIOD, SOC_GPIO_FUNC_B}
#define PIN_SSC0_RK {PIO_PA22A_SSC_RK, PIOA, ID_PIOA, SOC_GPIO_FUNC_A}
#ifdef CONFIG_I2S_SAM_SSC_0_PIN_TD_PB5
#define PIN_SSC0_TD {PIO_PB5D_SSC_TD,  PIOB, ID_PIOB, SOC_GPIO_FUNC_D}
#elif CONFIG_I2S_SAM_SSC_0_PIN_TD_PD10
#define PIN_SSC0_TD {PIO_PD10C_SSC_TD, PIOD, ID_PIOD, SOC_GPIO_FUNC_C}
#elif CONFIG_I2S_SAM_SSC_0_PIN_TD_PD26
#define PIN_SSC0_TD {PIO_PD26B_SSC_TD, PIOD, ID_PIOD, SOC_GPIO_FUNC_B}
#endif
#define PIN_SSC0_TF {PIO_PB0D_SSC_TF,  PIOB, ID_PIOB, SOC_GPIO_FUNC_D}
#define PIN_SSC0_TK {PIO_PB1D_SSC_TK,  PIOB, ID_PIOB, SOC_GPIO_FUNC_D}

#define PINS_SSC0 {PIN_SSC0_RD, PIN_SSC0_RF, PIN_SSC0_RK, PIN_SSC0_TD, \
	  PIN_SSC0_TF, PIN_SSC0_TK}

#endif /* _ATMEL_SAM_SOC_PINMAP_H_ */
