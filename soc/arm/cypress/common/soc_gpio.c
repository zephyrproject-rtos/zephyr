/*
 * Copyright (c) 2016 Piotr Mienkowski
 * Copyright (c) 2021 ATL Electronics
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Cypress PSoC-6 MCU family General Purpose Input Output (GPIO)
 * module HAL driver.
 */

#include "soc_gpio.h"
#include "cy_gpio.h"

static uint32_t soc_gpio_get_drv_mode(uint32_t flags)
{
	uint32_t drv_mode = CY_GPIO_DM_ANALOG;

	flags = ((flags & SOC_GPIO_FLAGS_MASK) >> SOC_GPIO_FLAGS_POS);

	if (flags & SOC_GPIO_OPENDRAIN) {
		drv_mode = CY_GPIO_DM_OD_DRIVESLOW_IN_OFF;
	} else if (flags & SOC_GPIO_OPENSOURCE) {
		drv_mode = CY_GPIO_DM_OD_DRIVESHIGH_IN_OFF;
	} else if (flags & SOC_GPIO_PUSHPULL) {
		drv_mode = CY_GPIO_DM_STRONG_IN_OFF;
	} else if ((flags & SOC_GPIO_PULLUP) && (flags & SOC_GPIO_PULLDOWN)) {
		drv_mode = CY_GPIO_DM_PULLUP_DOWN_IN_OFF;
	} else if (flags & SOC_GPIO_PULLUP) {
		drv_mode = CY_GPIO_DM_PULLUP_IN_OFF;
	} else if (flags & SOC_GPIO_PULLDOWN) {
		drv_mode = CY_GPIO_DM_PULLDOWN_IN_OFF;
	} else {
		;
	}

	if (flags & SOC_GPIO_INPUTENABLE) {
		drv_mode |= CY_GPIO_DM_HIGHZ;
	}

	return drv_mode;
}

void soc_gpio_configure(const struct soc_gpio_pin *pin)
{
	uint32_t drv_mode = soc_gpio_get_drv_mode(pin->flags);
	uint32_t function = ((pin->flags & SOC_GPIO_FUNC_MASK) >>
			     SOC_GPIO_FUNC_POS);

	Cy_GPIO_SetHSIOM(pin->regs, pin->pinum, function);
	Cy_GPIO_SetDrivemode(pin->regs, pin->pinum, drv_mode);
}

void soc_gpio_list_configure(const struct soc_gpio_pin pins[], size_t size)
{
	for (size_t i = 0; i < size; i++) {
		soc_gpio_configure(&pins[i]);
	}
}
