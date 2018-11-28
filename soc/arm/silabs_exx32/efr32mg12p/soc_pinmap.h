/*
 * Copyright (c) 2018 Christian Taedcke, Diego Sueiro
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Silabs EFR32MG MCU pin definitions.
 *
 * This file contains pin configuration data required by different MCU
 * modules to correctly configure GPIO controller.
 */

#ifndef _SOC_PINMAP_H_
#define _SOC_PINMAP_H_

#include <em_gpio.h>

#ifdef CONFIG_UART_GECKO
#ifdef CONFIG_USART_GECKO_0
#if (DT_SILABS_GECKO_USART_USART_0_LOCATION == 0)
#define PIN_USART0_TXD {gpioPortA, 0, gpioModePushPull, 1}
#define PIN_USART0_RXD {gpioPortA, 1, gpioModeInput, 0}
#else
#error ("Serial Driver for Gecko MCUs not implemented for this location index")
#endif
#endif /* CONFIG_USART_GECKO_0 */
#endif /* CONFIG_UART_GECKO */

#ifdef CONFIG_LEUART_GECKO
#ifdef CONFIG_LEUART_GECKO_0
#if (DT_SILABS_GECKO_LEUART_LEUART_0_LOCATION == 27)
#define PIN_LEUART0_TXD {gpioPortF, 3, gpioModePushPull, 1}
#define PIN_LEUART0_RXD {gpioPortF, 4, gpioModeInput, 1}
#else
#error ("Serial Driver for Gecko MCUs not implemented for this location index")
#endif
#endif /* CONFIG_LEUART_GECKO_0 */
#endif /* CONFIG_LEUART_GECKO */

#ifdef CONFIG_I2C_GECKO
#ifdef CONFIG_I2C_0
#if (DT_SILABS_GECKO_I2C_I2C_0_LOCATION == 15)
#define PIN_I2C0_SDA {gpioPortC, 10, gpioModeWiredAnd, 1}
#define PIN_I2C0_SCL {gpioPortC, 11, gpioModeWiredAnd, 1}
#else
#error ("I2C Driver for Gecko MCUs not implemented for this location index")
#endif
#endif /* CONFIG_I2C_0 */

#ifdef CONFIG_I2C_1
#if (DT_SILABS_GECKO_I2C_I2C_1_LOCATION == 17)
#define PIN_I2C1_SDA {gpioPortC, 4, gpioModeWiredAnd, 1}
#define PIN_I2C1_SCL {gpioPortC, 5, gpioModeWiredAnd, 1}
#else
#error ("I2C Driver for Gecko MCUs not implemented for this location index")
#endif
#endif /* CONFIG_I2C_1 */
#endif /* CONFIG_I2C_GECKO */

#endif /* _SOC_PINMAP_H_ */
