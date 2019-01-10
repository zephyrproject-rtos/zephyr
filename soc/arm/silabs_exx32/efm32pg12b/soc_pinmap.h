/*
 * Copyright (c) 2018 Christian Taedcke
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

#ifdef CONFIG_GPIO_GECKO
/* Serial Wire Output (SWO) */
#if (DT_GPIO_GECKO_SWO_LOCATION == 0)
#define PIN_SWO {gpioPortF, 2, gpioModePushPull, 1}
#elif (DT_GPIO_GECKO_SWO_LOCATION == 1)
#define PIN_SWO {gpioPortB, 13, gpioModePushPull, 1}
#elif (DT_GPIO_GECKO_SWO_LOCATION == 2)
#define PIN_SWO {gpioPortD, 15, gpioModePushPull, 1}
#elif (DT_GPIO_GECKO_SWO_LOCATION == 3)
#define PIN_SWO {gpioPortC, 11, gpioModePushPull, 1}
#elif (DT_GPIO_GECKO_SWO_LOCATION >= 4)
#error ("Invalid SWO pin location")
#endif
#endif /* CONFIG_GPIO_GECKO */

#ifdef CONFIG_USART_GECKO_0
#if (DT_USART_GECKO_0_GPIO_LOC == 0)
#define PIN_USART0_TXD {gpioPortA, 0, gpioModePushPull, 1}
#define PIN_USART0_RXD {gpioPortA, 1, gpioModeInput, 1}
#elif (DT_USART_GECKO_0_GPIO_LOC == 1)
#define PIN_USART0_TXD {gpioPortA, 1, gpioModePushPull, 1}
#define PIN_USART0_RXD {gpioPortA, 2, gpioModeInput, 1}
#elif (DT_USART_GECKO_0_GPIO_LOC == 2)
#define PIN_USART0_TXD {gpioPortA, 2, gpioModePushPull, 1}
#define PIN_USART0_RXD {gpioPortA, 3, gpioModeInput, 1}
#elif (DT_USART_GECKO_0_GPIO_LOC == 18)
#define PIN_USART0_TXD {gpioPortD, 10, gpioModePushPull, 1}
#define PIN_USART0_RXD {gpioPortD, 11, gpioModeInput, 1}
#else
#error ("Serial Driver for Gecko MCUs not implemented for this location index")
#endif
#endif /* CONFIG_USART_GECKO_0 */

#ifdef CONFIG_USART_GECKO_1
#if (CONFIG_USART_GECKO_1_GPIO_LOC == 0)
#error ("Serial Driver for Gecko MCUs not implemented for this location index")
#elif (DT_USART_GECKO_1_GPIO_LOC == 1)
#define PIN_USART1_TXD {gpioPortA, 1, gpioModePushPull, 1}
#define PIN_USART1_RXD {gpioPortA, 2, gpioModeInput, 1}
#elif (DT_USART_GECKO_1_GPIO_LOC == 2)
#define PIN_USART1_TXD {gpioPortA, 2, gpioModePushPull, 1}
#define PIN_USART1_RXD {gpioPortA, 3, gpioModeInput, 1}
#elif (DT_USART_GECKO_1_GPIO_LOC == 3)
#define PIN_USART1_TXD {gpioPortA, 3, gpioModePushPull, 1}
#define PIN_USART1_RXD {gpioPortA, 4, gpioModeInput, 1}
#else
#error ("Serial Driver for Gecko MCUs not implemented for this location index")
#endif
#endif /* CONFIG_USART_GECKO_1 */

#ifdef CONFIG_LEUART_GECKO
#ifdef CONFIG_LEUART_GECKO_0
#if (DT_LEUART_GECKO_0_LOCATION == 18)
#define PIN_LEUART0_TXD {gpioPortD, 10, gpioModePushPull, 1}
#define PIN_LEUART0_RXD {gpioPortD, 11, gpioModeInput, 1}
#else
#error ("Serial Driver for Gecko MCUs not implemented for this location index")
#endif
#endif /* CONFIG_LEUART_GECKO_0 */
#endif /* CONFIG_LEUART_GECKO */

#ifdef CONFIG_I2C_GECKO
#ifdef CONFIG_I2C_0
#if (DT_I2C_GECKO_0_LOCATION == 15)
#define PIN_I2C0_SDA {gpioPortC, 10, gpioModeWiredAnd, 1}
#define PIN_I2C0_SCL {gpioPortC, 11, gpioModeWiredAnd, 1}
#else
#error ("I2C Driver for Gecko MCUs not implemented for this location index")
#endif
#endif /* CONFIG_I2C_0 */

#ifdef CONFIG_I2C_1
#if (DT_I2C_GECKO_1_LOCATION == 6)
#define PIN_I2C1_SDA {gpioPortB, 6, gpioModeWiredAnd, 1}
#define PIN_I2C1_SCL {gpioPortB, 7, gpioModeWiredAnd, 1}
#else
#error ("I2C Driver for Gecko MCUs not implemented for this location index")
#endif
#endif /* CONFIG_I2C_1 */
#endif /* CONFIG_I2C_GECKO */

#endif /* _SILABS_EFM32PG12B_SOC_PINMAP_H_ */
