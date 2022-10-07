/*
 * Copyright (c) 2022 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
 
 #define DT_DRV_COMPAT nuvoton_numaker_pinctrl
 
#include <assert.h>
#include <zephyr/drivers/pinctrl.h>
#include <soc.h>

typedef struct nvt_pinctrl_pin_t {
	uint32_t pinMux;
	uint32_t cfgData;
} nvt_pinctrl_pin_t;

/**
 * Configure pin multi-function
 */
static void configure_pin(nvt_pinctrl_pin_t pin)
{
    uint32_t pin_index = PIN_INDEX(pin.pinMux);
    uint32_t port_index = PORT_INDEX(pin.pinMux);
    uint32_t mfp_cfg = MFP_CFG(pin.pinMux);
    uint32_t pin_cfg = pin.cfgData;
    /* Get mfp_base, it should be == (&SYS->GPA_MFP0) in M467 */
    uint32_t mfp_base = DT_REG_ADDR(DT_NODELABEL(pinctrl));

    uint32_t *GPx_MFPx = ((uint32_t *) mfp_base) + port_index * 4 + (pin_index / 4);
    uint32_t pinMask = NU_MFP_MASK(pin_index);

    // E.g.: SYS->GPA_MFP0  = (SYS->GPA_MFP0 & (~SYS_GPA_MFP0_PA0MFP_Msk) ) | SYS_GPA_MFP0_PA0MFP_SC0_CD  ;
    *GPx_MFPx  = (*GPx_MFPx & (~pinMask)) | mfp_cfg;
}

/* Pinctrl API implementation */
int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
			   uintptr_t reg)
{
	ARG_UNUSED(reg);
    const nvt_pinctrl_pin_t *nvt_pins = (nvt_pinctrl_pin_t*)pins;
    uint8_t nvt_pin_cnt = (pin_cnt/(sizeof(nvt_pinctrl_pin_t)/sizeof(pinctrl_soc_pin_t)));
    
	/* Configure all peripheral devices' properties here. */
	for (uint8_t i = 0; i < nvt_pin_cnt; i++) {
        configure_pin(nvt_pins[i]);
	}

	return 0;
}
