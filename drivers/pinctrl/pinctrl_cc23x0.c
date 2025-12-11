/*
 * Copyright (c) 2024 Texas Instruments Incorporated
 * Copyright (c) 2024 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc23x0_pinctrl

#include <zephyr/drivers/pinctrl.h>

#include <inc/hw_types.h>

#define IOC_BASE_REG     DT_REG_ADDR(DT_NODELABEL(pinctrl))
#define IOC_BASE_PIN_REG 0x00000100
#define IOC_ADDR(index)  (IOC_BASE_REG + IOC_BASE_PIN_REG + (sizeof(uint32_t) * (index)))

static int pinctrl_cc23x0_set(uint32_t pin, uint32_t func, uint32_t mode)
{
	uint32_t iocfg_reg = IOC_ADDR(pin);

	HWREG(iocfg_reg) = mode | func;

	return 0;
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		pinctrl_cc23x0_set(pins[i].pin, pins[i].iofunc, pins[i].iomode);
	}

	return 0;
}
