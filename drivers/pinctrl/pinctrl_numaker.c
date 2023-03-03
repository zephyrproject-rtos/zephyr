/*
 * Copyright (c) 2023 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_numaker_pinctrl

#include <zephyr/drivers/pinctrl.h>
#include <NuMicro.h>

/**
 * Configure pin multi-function
 */
static void configure_pin(const pinctrl_soc_pin_t *pin)
{
	uint32_t pin_mux = pin->pin_mux;
	uint32_t pin_index = PIN_INDEX(pin_mux);
	uint32_t port_index = PORT_INDEX(pin_mux);
	uint32_t mfp_cfg = MFP_CFG(pin_mux);

	/* Get mfp_base, it should be == (&SYS->GPA_MFP0) in M467 */
	uint32_t mfp_base = DT_REG_ADDR(DT_NODELABEL(pinctrl));

	uint32_t *GPx_MFPx = ((uint32_t *)mfp_base) + port_index * 4 + (pin_index / 4);
	uint32_t pinMask = NU_MFP_MASK(pin_index);

	/*
	 * E.g.: SYS->GPA_MFP0  = (SYS->GPA_MFP0 & (~SYS_GPA_MFP0_PA0MFP_Msk) ) |
	 * SYS_GPA_MFP0_PA0MFP_SC0_CD;
	 */
	*GPx_MFPx = (*GPx_MFPx & (~pinMask)) | mfp_cfg;
}

/* Pinctrl API implementation */
int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);

	/* Configure all peripheral devices' properties here. */
	for (uint8_t i = 0U; i < pin_cnt; i++) {
		configure_pin(&pins[i]);
	}

	return 0;
}
