/*
 * Copyright (c) 2017 Christian Taedcke
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Silabs EXX32 MCU family General Purpose Input Output (GPIO)
 * module HAL driver.
 */

#ifndef _SILABS_EXX32_SOC_GPIO_H_
#define _SILABS_EXX32_SOC_GPIO_H_

#include <soc.h>
#include <em_gpio.h>

struct soc_gpio_pin {
	GPIO_Port_TypeDef port; /** GPIO port */
	unsigned int pin;       /** GPIO pin on the port */
	GPIO_Mode_TypeDef mode; /** mode of the pin, e.g. gpioModeInput */
	unsigned int out;       /** out register value */
};

/**
 * @brief Configure GPIO pin
 * @param[in] pin configuration data
 */
void soc_gpio_configure(const struct soc_gpio_pin *pin);

#endif /* _SILABS_EXX32_SOC_GPIO_H_ */
