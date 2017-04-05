/*
 * Copyright (c) 2017 Christian Taedcke
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Silabs EXX32 MCU family General Purpose Input Output (GPIO)
 * module HAL driver.
 */

#include "soc_gpio.h"

void soc_gpio_configure(const struct soc_gpio_pin *pin)
{
	GPIO_PinModeSet(pin->port, pin->pin, pin->mode, pin->out);
}
