/*
 * Copyright (c) 2017 Christian Taedcke
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Silabs EFM32PG12B MCU pin definitions.
 *
 * This file contains pin configuration data required by different MCU
 * modules to correctly configure GPIO controller.
 */

#ifndef _SILABS_EFM32PG12B_SOC_PINMAP_H_
#define _SILABS_EFM32PG12B_SOC_PINMAP_H_

#include <soc.h>
#include <em_gpio.h>

#ifdef CONFIG_SOC_PART_NUMBER_EFM32PG12B500F1024GL125
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
#elif (CONFIG_USART_GECKO_0_GPIO_LOC == 18)
#define PIN_USART0_TXD {gpioPortD, 10, gpioModePushPull, 1}
#define PIN_USART0_RXD {gpioPortD, 11, gpioModeInput, 1}
#else
#error ("Serial Driver for Gecko MCUs not implemented for this location index")
#endif
#endif /* CONFIG_USART_GECKO_0 */

#ifdef CONFIG_USART_GECKO_1
#if (CONFIG_USART_GECKO_1_GPIO_LOC == 0)
#error ("Serial Driver for Gecko MCUs not implemented for this location index")
#elif (CONFIG_USART_GECKO_1_GPIO_LOC == 1)
#define PIN_USART1_TXD {gpioPortA, 1, gpioModePushPull, 1}
#define PIN_USART1_RXD {gpioPortA, 2, gpioModeInput, 1}
#elif (CONFIG_USART_GECKO_1_GPIO_LOC == 2)
#define PIN_USART1_TXD {gpioPortA, 2, gpioModePushPull, 1}
#define PIN_USART1_RXD {gpioPortA, 3, gpioModeInput, 1}
#elif (CONFIG_USART_GECKO_1_GPIO_LOC == 3)
#define PIN_USART1_TXD {gpioPortA, 3, gpioModePushPull, 1}
#define PIN_USART1_RXD {gpioPortA, 4, gpioModeInput, 1}
#else
#error ("Serial Driver for Gecko MCUs not implemented for this location index")
#endif
#endif /* CONFIG_USART_GECKO_1 */
#else
#error ("Pinmap not available for this for Pearl Gecko MCU")
#endif /* SOC_PART_NUMBER_EFM32PG12B500F1024GL125*/

#endif /* _SILABS_EFM32PG12B_SOC_PINMAP_H_ */
