/*
 * Copyright (c) 2024 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/devicetree.h>
#include <zephyr/drivers/pinctrl.h>

#include <IfxPort_reg.h>
#include <soc.h>

#include "pinctrl_aurix_modules.h"

#define PINCTRL_BASE DT_REG_ADDR_BY_IDX(DT_NODELABEL(pinctrl), 0)
#define PORT_BASE(x) (&MODULE_P00 + x)

static void ALWAYS_INLINE atomic_ldmst_pdr(void *addr, uint32_t offset, uint32_t value)
{
	__asm("	imask %%e14, %0, %1, 4\n"
	      "	ldmst [%2]+0, %%e14\n"
	      :
	      : "d"(value), "d"(offset), "a"((uint32_t *)(addr))
	      : "e14");
}

static void ALWAYS_INLINE atomic_ldmst_bit(void *addr, uint32_t offset, uint32_t value)
{
	__asm("	imask %%e14, %0, %1, 1\n"
	      "	ldmst [%2]+0, %%e14\n"
	      :
	      : "d"(value), "d"(offset), "a"((uint32_t *)(addr))
	      : "e14");
}

static void pinctrl_configure_pin(const pinctrl_soc_pin_t *pin)
{
	void *protse = (void *)&PORT_BASE(pin->port)->PROTSE;

	if (pin->analog) {
		aurix_prot_set_state(protse, AURIX_PROT_STATE_CONFIG);
		atomic_ldmst_bit((uint8_t *)PORT_BASE(pin->port) + offsetof(Ifx_P, PCSRSEL),
				 pin->pin, 1);
		aurix_prot_set_state(protse, AURIX_PROT_STATE_RUN);
		return;
	}
	if (pin->output_select) {
		aurix_prot_set_state(protse, AURIX_PROT_STATE_CONFIG);
		atomic_ldmst_bit((uint8_t *)PORT_BASE(pin->port) + offsetof(Ifx_P, PCSRSEL),
				 pin->pin, pin->output_select);
		aurix_prot_set_state(protse, AURIX_PROT_STATE_RUN);
	}

	void *accgrp_prote = (void *)&PORT_BASE(pin->port)->ACCGRP[0].PROTE;
	Ifx_P_PADCFG_DRVCFG drvcfg = {
		.B = {.DIR = pin->output,
		      .OD = pin->open_drain,
		      .MODE = pin->output ? pin->alt : (pin->pull_down | (pin->pull_up << 1)),
		      .PD = pin->pad_driver,
		      .PL = pin->pad_level}};

	if (aurix_prot_get_state(accgrp_prote) == AURIX_PROT_STATE_RUN) {
		aurix_prot_set_state(accgrp_prote, AURIX_PROT_STATE_CONFIG);
	}
	PORT_BASE(pin->port)->PADCFG[pin->pin].DRVCFG = drvcfg;
	aurix_prot_set_state(accgrp_prote, AURIX_PROT_STATE_RUN);
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	switch (reg) {
		DT_FOREACH_STATUS_OKAY(infineon_asclin_uart, ASCLIN_ENABLED_CASE)
		DT_FOREACH_STATUS_OKAY(infineon_aurix_i2c, I2C_ENABLED_CASE)
		DT_FOREACH_STATUS_OKAY(snps_designware_ethernet, LETH_MAC_ENABLED_CASE)
		DT_FOREACH_STATUS_OKAY(snps_dwc_ether_xgmac_mdio, GETH_MDIO_ENABLED_CASE)
	}

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		pinctrl_configure_pin(pins++);
	}

	return 0;
}
