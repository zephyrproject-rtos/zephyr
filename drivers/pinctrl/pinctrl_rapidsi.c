/*
 * Copyright (c) 2024 Rapid Silicon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>

#if DT_HAS_COMPAT_STATUS_OKAY(rapidsi_pinctrl)
	#define NUM_PINS DT_PROP(DT_NODELABEL(pinctrl), npins)
	struct pinctrl_registers *pinctrl_regs = \
	(struct pinctrl_registers *)DT_REG_ADDR(DT_NODELABEL(pinctrl));
#else
	#warning "rapidsi,pinctrl node status is disabled in the dts. Program may crash"
	#define NUM_PINS 0
	struct pinctrl_registers *pinctrl_regs = NULL;
#endif

struct pinctrl_registers {
	volatile uint32_t pin_csr;
	volatile uint32_t pin_cfg[NUM_PINS];
};

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
			   uintptr_t reg)
{
	ARG_UNUSED(reg);

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		pinctrl_regs->pin_cfg[pins->pin_num] = pins->pin_cfg;	
	}

	return 0;
}
