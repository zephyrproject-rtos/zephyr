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
#if (DT_SILABS_GECKO_USART_USART_0_LOCATION_RX == 0)
#define PIN_USART0_RXD {gpioPortA, 1, gpioModeInput, 1}
#elif (DT_SILABS_GECKO_USART_USART_0_LOCATION_RX == 1)
#define PIN_USART0_RXD {gpioPortA, 2, gpioModeInput, 1}
#elif (DT_SILABS_GECKO_USART_USART_0_LOCATION_RX == 2)
#define PIN_USART0_RXD {gpioPortA, 3, gpioModeInput, 1}
#else
#error ("Serial Driver for Gecko MCUs not implemented for this location index")
#endif

#if (DT_SILABS_GECKO_USART_USART_0_LOCATION_TX == 0)
#define PIN_USART0_TXD {gpioPortA, 0, gpioModePushPull, 1}
#elif (DT_SILABS_GECKO_USART_USART_0_LOCATION_TX == 1)
#define PIN_USART0_TXD {gpioPortA, 1, gpioModePushPull, 1}
#elif (DT_SILABS_GECKO_USART_USART_0_LOCATION_TX == 2)
#define PIN_USART0_TXD {gpioPortA, 2, gpioModePushPull, 1}
#else
#error ("Serial Driver for Gecko MCUs not implemented for this location index")
#endif
#endif /* CONFIG_USART_GECKO_0 */

#ifdef CONFIG_USART_GECKO_1
#if (DT_SILABS_GECKO_USART_USART_1_LOCATION_RX == 0)
#define PIN_USART1_RXD {gpioPortA, 1, gpioModeInput, 1}
#elif (DT_SILABS_GECKO_USART_USART_1_LOCATION_RX == 1)
#define PIN_USART1_RXD {gpioPortA, 2, gpioModeInput, 1}
#elif (DT_SILABS_GECKO_USART_USART_1_LOCATION_RX == 2)
#define PIN_USART1_RXD {gpioPortA, 3, gpioModeInput, 1}
#elif (DT_SILABS_GECKO_USART_USART_1_LOCATION_RX == 3)
#define PIN_USART1_RXD {gpioPortA, 4, gpioModeInput, 1}
#elif (DT_SILABS_GECKO_USART_USART_1_LOCATION_RX == 13)
#define PIN_USART1_RXD {gpioPortC, 9, gpioModeInput, 1}
#else
#error ("Serial Driver for Gecko MCUs not implemented for this location index")
#endif

#if (DT_SILABS_GECKO_USART_USART_1_LOCATION_TX == 0)
#define PIN_USART1_TXD {gpioPortA, 0, gpioModePushPull, 1}
#elif (DT_SILABS_GECKO_USART_USART_1_LOCATION_TX == 1)
#define PIN_USART1_TXD {gpioPortA, 1, gpioModePushPull, 1}
#elif (DT_SILABS_GECKO_USART_USART_1_LOCATION_TX == 2)
#define PIN_USART1_TXD {gpioPortA, 2, gpioModePushPull, 1}
#elif (DT_SILABS_GECKO_USART_USART_1_LOCATION_TX == 3)
#define PIN_USART1_TXD {gpioPortA, 3, gpioModePushPull, 1}
#elif (DT_SILABS_GECKO_USART_USART_1_LOCATION_TX == 4)
#define PIN_USART1_TXD {gpioPortA, 4, gpioModePushPull, 1}
#else
#error ("Serial Driver for Gecko MCUs not implemented for this location index")
#endif
#endif /* CONFIG_USART_GECKO_1 */

#endif /* _SILABS_EFR32FG1P_SOC_PINMAP_H_ */
