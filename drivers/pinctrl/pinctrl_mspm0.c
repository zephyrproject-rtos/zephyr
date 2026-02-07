/*
 * Copyright (c) 2025 Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <ti/driverlib/dl_gpio.h>

LOG_MODULE_REGISTER(pinctrl_mspm0, CONFIG_PINCTRL_LOG_LEVEL);

#define DT_DRV_COMPAT ti_mspm0_pinctrl

#define MSPM0_PINCM(pinmux)		(pinmux >> 0x10)
#define MSPM0_PIN_FUNCTION(pinmux)	(pinmux & 0x3F)

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins,
			   uint8_t pin_cnt,
			   uintptr_t reg)
{
	ARG_UNUSED(reg);

	uint8_t pin_function;
	uint32_t pin_cm;
	uint32_t iomux;

	for (int i = 0; i < pin_cnt; i++) {
		pin_cm = MSPM0_PINCM(pins[i].pinmux);
		pin_function = MSPM0_PIN_FUNCTION(pins[i].pinmux);
		iomux = pins[i].iomux;

		/* Check for invalid pull-up/pull-down configuration */
		if (((iomux >> MSP_GPIO_RESISTOR_PULL_UP) & 0x1) &&
		    ((iomux >> MSP_GPIO_RESISTOR_PULL_DOWN) & 0x1)) {
			LOG_ERR("Pin CM%d: Cannot enable both pull-up and pull-down "
				"simultaneously", pin_cm);
			return -EINVAL;
		}

		if (pin_function == 0x00) {
			DL_GPIO_initPeripheralAnalogFunction(pin_cm);
		} else {
			DL_GPIO_initPeripheralFunction(pin_cm,
						       (iomux | pin_function));
		}
	}

	return 0;
}
