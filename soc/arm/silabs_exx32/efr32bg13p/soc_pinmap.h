/*
 * Copyright (c) 2020 Piotr Mienkowski
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Silabs EFR32BG13P MCU pin definitions.
 */

#ifndef SOC_PINMAP_H_
#define SOC_PINMAP_H_

#include <devicetree.h>
#include <em_gpio.h>

#define GPIO_NODE DT_INST(0, silabs_gecko_gpio)
#if DT_NODE_HAS_PROP(GPIO_NODE, location_swo)
#define SWO_LOCATION DT_PROP(GPIO_NODE, location_swo)
#endif

/* Serial Wire Output (SWO) */
#if (SWO_LOCATION == 0)
#define PIN_SWO {gpioPortF, 2, gpioModePushPull, 1}
#elif (SWO_LOCATION == 1)
#define PIN_SWO {gpioPortB, 13, gpioModePushPull, 1}
#elif (SWO_LOCATION == 2)
#define PIN_SWO {gpioPortD, 15, gpioModePushPull, 1}
#elif (SWO_LOCATION == 3)
#define PIN_SWO {gpioPortC, 11, gpioModePushPull, 1}
#elif (SWO_LOCATION >= 4)
#error ("Invalid SWO pin location")
#endif

#endif /* SOC_PINMAP_H_ */
