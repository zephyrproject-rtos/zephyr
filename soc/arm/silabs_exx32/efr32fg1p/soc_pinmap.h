/*
 * Copyright (c) 2018 Christian Taedcke
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Silabs EFR32FG1P MCU pin definitions.
 *
 * This file contains pin configuration data required by different MCU
 * modules to correctly configure GPIO controller.
 */

#ifndef _SILABS_EFR32FG1P_SOC_PINMAP_H_
#define _SILABS_EFR32FG1P_SOC_PINMAP_H_

#include <soc.h>
#include <em_gpio.h>

#ifdef CONFIG_SOC_PART_NUMBER_EFR32FG1P133F256GM48
#ifdef CONFIG_USART_GECKO_0
#if (CONFIG_USART_GECKO_0_GPIO_LOC == 0)
#define PIN_USART0_TXD {gpioPortA, 0, gpioModePushPull, 1}
#define PIN_USART0_RXD {gpioPortA, 1, gpioModeInput, 1}
#elif (CONFIG_USART_GECKO_0_GPIO_LOC == 1)
#define PIN_USART0_TXD {gpioPortA, 1, gpioModePushPull, 1}
#define PIN_USART0_RXD {gpioPortA, 2, gpioModeInput, 1}
#elif (CONFIG_USART_GECKO_0_GPIO_LOC == 2)
#define PIN_USART0_TXD {gpioPortA, 2, gpioModePushPull, 1}
#define PIN_USART0_RXD {gpioPortA, 3, gpioModeInput, 1}
#else
#error ("Serial Driver for Gecko MCUs not implemented for this location index")
#endif
#endif /* CONFIG_USART_GECKO_0 */

#ifdef CONFIG_USART_GECKO_1
#if (CONFIG_USART_GECKO_1_GPIO_LOC == 0)
#error ("Serial Driver for Gecko MCUs not implemented for this location index")
#elif (CONFIG_USART_GECKO_1_GPIO_LOC == 1)
#define PIN_USART1_TXD {gpioPortF, 10, gpioModePushPull, 1}
#define PIN_USART1_RXD {gpioPortF, 11, gpioModeInput, 1}
#elif (CONFIG_USART_GECKO_1_GPIO_LOC == 2)
#define PIN_USART1_TXD {gpioPortB, 9, gpioModePushPull, 1}
#define PIN_USART1_RXD {gpioPortB, 10, gpioModeInput, 1}
#elif (CONFIG_USART_GECKO_1_GPIO_LOC == 3)
#define PIN_USART1_TXD {gpioPortE, 2, gpioModePushPull, 1}
#define PIN_USART1_RXD {gpioPortE, 3, gpioModeInput, 1}
#else
#error ("Serial Driver for Gecko MCUs not implemented for this location index")
#endif
#endif /* CONFIG_USART_GECKO_1 */
#else
#error ("Pinmap not available for this for Flex Gecko MCU")
#endif /* CONFIG_SOC_PART_NUMBER_EFR32FG1P133F256GM48 */

#endif /* _SILABS_EFR32FG1P_SOC_PINMAP_H_ */
