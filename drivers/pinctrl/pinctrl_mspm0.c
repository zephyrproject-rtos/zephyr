/*
 * Copyright (c) 2025 Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>

LOG_MODULE_REGISTER(pinctrl_mspm0, CONFIG_PINCTRL_LOG_LEVEL);

#define DT_DRV_COMPAT ti_mspm0_pinctrl

#define MSPM_PINCM(pinmux)        ((pinmux) >> 0x10)
#define MSPM_PIN_FUNCTION(pinmux) ((pinmux) & 0x3F)

/*
 * PINCM register array starts at IOMUX_BASE + 4 (RESERVED0 = 4 bytes).
 * Each PINCM register is 4 bytes wide.
 * Bit 7 (PC) must be set to connect the pin to a peripheral function.
 */
#define MSPM_PINCM_ADDR(n)      (DT_INST_REG_ADDR(0) + 4U + (uint32_t)(n) * 4U)
#define MSPM_PINCM_PC_CONNECTED BIT(7)

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);

	for (int i = 0; i < pin_cnt; i++) {
		uint32_t pin_cm = MSPM_PINCM(pins[i].pinmux);
		uint32_t function = MSPM_PIN_FUNCTION(pins[i].pinmux);
		uint32_t iomux = pins[i].iomux;
		mm_reg_t addr = MSPM_PINCM_ADDR(pin_cm);

		/* Check for invalid pull-up/pull-down configuration */
		if (((iomux >> MSP_GPIO_RESISTOR_PULL_UP) & 0x1U) &&
		    ((iomux >> MSP_GPIO_RESISTOR_PULL_DOWN) & 0x1U)) {
			LOG_ERR("Pin CM%d: Cannot enable both pull-up and pull-down "
				"simultaneously", pin_cm);
			return -EINVAL;
		}

		if (function == 0x00U) {
			/* Analog: disconnect pin from all peripheral functions */
			sys_write32(0U, addr);
		} else {
			sys_write32((iomux | function) | MSPM_PINCM_PC_CONNECTED, addr);
		}
	}

	return 0;
}
