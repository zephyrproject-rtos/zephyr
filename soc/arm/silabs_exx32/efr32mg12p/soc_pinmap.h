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
