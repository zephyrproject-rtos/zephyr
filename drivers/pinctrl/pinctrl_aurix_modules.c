/*
 * Copyright (c) 2024 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "pinctrl_aurix_modules.h"

#include "IfxI2c_regdef.h"
#include "IfxAsclin_regdef.h"

void pinctrl_configure_asclin_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, mm_reg_t base)
{
	uint32_t iocr = 0;
	uint32_t j;

	for (j = 0; j < pin_cnt; j++) {
		if (pins[j].output) {
			continue;
		}
		switch (pins[j].type) {
		case 0:
			iocr |= 0x7 & pins[j].alt;
			break;
		case 1:
			iocr |= ((0x3 & pins[j].alt) << 16) | BIT(29);
			break;
		}
	}
	sys_write32(iocr, base + offsetof(Ifx_ASCLIN, IOCR));
}

void pinctrl_configure_i2c_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, mm_reg_t base)
{
	volatile Ifx_I2C *i2c = (Ifx_I2C *)base;
	Ifx_I2C_GPCTL gpctl = {.U = i2c->GPCTL.U};
	uint32_t j;

	for (j = 0; j < pin_cnt; j++) {
		if (pins[j].output) {
			continue;
		}
		gpctl.B.PISEL = pins[j].alt;
		break;
	}
	i2c->GPCTL = gpctl;
}

#if CONFIG_SOC_SERIES_TC3X
#include "IfxGeth_reg.h"
void pinctrl_configure_geth_mac_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, mm_reg_t base)
{
	volatile Ifx_GETH *geth = (Ifx_GETH *)base;
	uint32_t j;
	uint32_t gpctl = geth->GPCTL.U;

	for (j = 0; j < pin_cnt; j++) {
		if (pins[j].output) {
			continue;
		}
		if (pins[j].type > 10) {
			continue;
		}
		gpctl = (gpctl & ~(0x3 << pins[j].type * 2)) |
			FIELD_PREP(0x3 << pins[j].type * 2, pins[j].alt);
	}
	geth->GPCTL.U = gpctl;
}

void pinctrl_configure_geth_mdio_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, mm_reg_t base)
{
	volatile Ifx_GETH *geth = (Ifx_GETH *)(base - offsetof(Ifx_GETH, MAC_MDIO_ADDRESS));
	uint32_t j;
	uint32_t gpctl = geth->GPCTL.U;

	for (j = 0; j < pin_cnt; j++) {
		if (pins[j].output) {
			continue;
		}
		if (pins[j].type > 10) {
			continue;
		}
		gpctl = (gpctl & ~(0x3 << pins[j].type * 2)) |
			FIELD_PREP(0x3 << pins[j].type * 2, pins[j].alt);
	}
	geth->GPCTL.U = gpctl;
}
#endif

#if CONFIG_SOC_SERIES_TC4X
#if DT_NODE_EXISTS(DT_NODELABEL(leth0))
#include "IfxLeth_reg.h"
void pinctrl_configure_leth_mac_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, mm_reg_t base)
{
	uintptr_t offset = base - DT_REG_ADDR(DT_NODELABEL(leth0)) - 0x10000;
	uint8_t id = offset / 0x2000;
	uint32_t portctrl = 0;
	uint32_t j;

	for (j = 0; j < pin_cnt; j++) {
		if (pins[j].output) {
			continue;
		}
		if (pins[j].type == 0 || pins[j].type > 12) {
			continue;
		}
		portctrl = (portctrl & ~(0x3 << pins[j].type * 2)) |
			   FIELD_PREP(0x3 << pins[j].type * 2, pins[j].alt);
	}
	sys_write32(portctrl, DT_REG_ADDR(DT_NODELABEL(leth0)) + offsetof(Ifx_LETH, P) +
				      sizeof(Ifx_LETH_P) * id);
}
#endif /* DT_NODE_EXISTS leth0 */

#if DT_COMPAT_GET_ANY_STATUS_OKAY(infineon_tc4x_geth)
#include "IfxHsphy_reg.h"
void pinctrl_configure_geth_mdio_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, mm_reg_t base)
{
	uintptr_t offset = base - DT_REG_ADDR(DT_NODELABEL(geth0)) - 0x10000;
	uint8_t id = offset / 0x2000;
	uint32_t portctrl = MODULE_HSPHY.ETH[id].U;
	uint32_t j;

	for (j = 0; j < pin_cnt; j++) {
		if (pins[j].output) {
			continue;
		}
		if (pins[j].type != 0) {
			continue;
		}
		portctrl = (portctrl & ~(0x3 << pins[j].type * 2)) |
			   FIELD_PREP(0x3 << pins[j].type * 2, pins[j].alt);
	}
	MODULE_HSPHY.ETH[id].U = portctrl | 0x4;
}
#endif
#endif
