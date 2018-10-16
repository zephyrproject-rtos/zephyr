/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file header for uwp pin multiplexing
 */

#ifndef _HAL_PINMUX_H_
#define _HAL_PINMUX_H_

#include "uwp_hal.h"
#include <device.h>

/* pretend that array will cover pin functions */

#define PINMUX_PIN_REG(x) \
			(PIN_PIN_CRTL_REG0 + 0x4 * x)

enum PINMUX_REG_UWP {
	PIN_CRTL_REG0,
	PIN_CRTL_REG1,
	PIN_CRTL_REG2,
	PIN_CRTL_REG3,
	P_GPIO0,
	P_GPIO1,
	P_GPIO2,
	P_GPIO3,
	ESMD3,
	ESMD2,
	ESMD1,
	ESMD0,
	ESMCSN,
	ESMSMP,
	ESMCLK,
	IISDO,
	IISCLK,
	IISLRCK,
	IISDI,
	MTMS,
	MTCK,
	XTLEN,
	INT,
	PTEST,
	CHIP_EN,
	RST_N,
	WCI_2_RXD,
	WCI_2_TXD,
	U0TXD,
	U0RXD,
	U0RTS,
	U0CTS,
	U3TXD,
	U3RXD,
	RFCTL0,
	RFCTL1,
	RFCTL2,
	RFCTL3,
	RFCTL4,
	RFCTL5,
	RFCTL6,
	RFCTL7,
	SD_D3,
	SD_D2,
	SD_D1,
	SD_D0,
	SD_CLK,
	SD_CMD,
	GNSS_LNA_EN,
	U1TXD,
	U1RXD,
	U1RTS,
	U1CTS,
	PCIE_CLKREQ_L,
	PCIE_RST_L,
	PCIE_WAKE_L,
};

static inline void __pin_enbable(u8_t enable)
{
	u32_t regval;

	regval  = sys_read32(REG_APB_EB);
	if (enable) {
		sys_write32(regval | BIT_APB_PIN_EB, REG_APB_EB);
	} else {
		sys_write32(regval & (~BIT_APB_PIN_EB), REG_APB_EB);
	}
}

static inline void uwp_pmux_func_clear(u32_t pin)
{
	u32_t pin_reg = PINMUX_PIN_REG(pin);
	u32_t conf = sys_read32(pin_reg);

	conf &= (~(PMUX_PIN_FUNC(1)));
	sys_write32(conf, pin_reg);
}

static inline void uwp_pmux_func_set(u32_t pin, u32_t func)
{
	u32_t pin_reg = PINMUX_PIN_REG(pin);
	u32_t conf = sys_read32(pin_reg);

	conf |= (PMUX_PIN_FUNC(func));
	sys_write32(conf, pin_reg);
}

static inline void uwp_pmux_get(u32_t pin, u32_t *func)
{
	u32_t pin_reg = PINMUX_PIN_REG(pin);
	u32_t conf = sys_read32(pin_reg);

	*func = conf;
}

static inline void uwp_pmux_pin_pullup(u32_t pin)
{
	u32_t pin_reg = PINMUX_PIN_REG(pin);
	u32_t conf = sys_read32(pin_reg);

	conf |= PIN_FPU_EN;
	sys_write32(conf, pin_reg);
}

static inline void uwp_pmux_pin_pulldown(u32_t pin)
{
	u32_t pin_reg = PINMUX_PIN_REG(pin);
	u32_t conf = sys_read32(pin_reg);

	conf |= PIN_FPD_EN;
	sys_write32(conf, pin_reg);
}

#endif	/* _STM32_PINMUX_H_ */
