/*
 * Copyright (c) 2019 Interay Solutions B.V.
 * Copyright (c) 2019 Oane Kingma
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Silabs EFM32GG11B MCU pin definitions.
 *
 * This file contains pin configuration data required by different MCU
 * modules to correctly configure GPIO controller.
 */

#ifndef _SILABS_EFM32GG11B_SOC_PINMAP_H_
#define _SILABS_EFM32GG11B_SOC_PINMAP_H_

#include <soc.h>
#include <em_gpio.h>

#ifdef CONFIG_GPIO_GECKO
/* Serial Wire Output (SWO) */
#if (DT_GPIO_GECKO_SWO_LOCATION == 0)
#define PIN_SWO {gpioPortF, 2, gpioModePushPull, 1}
#elif (DT_GPIO_GECKO_SWO_LOCATION == 1)
#define PIN_SWO {gpioPortC, 15, gpioModePushPull, 1}
#elif (DT_GPIO_GECKO_SWO_LOCATION == 2)
#define PIN_SWO {gpioPortD, 1, gpioModePushPull, 1}
#elif (DT_GPIO_GECKO_SWO_LOCATION == 3)
#define PIN_SWO {gpioPortD, 2, gpioModePushPull, 1}
#elif (DT_GPIO_GECKO_SWO_LOCATION >= 4)
#error ("Invalid SWO pin location")
#endif
#endif /* CONFIG_GPIO_GECKO */

#endif /* _SILABS_EFM32GG11B_SOC_PINMAP_H_ */
