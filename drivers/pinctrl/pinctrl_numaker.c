/*
 * Copyright (c) 2023 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_numaker_pinctrl

#include <zephyr/drivers/pinctrl.h>
#include <NuMicro.h>

/* Get mfp_base, it should be == (&SYS->GPA_MFP0) */
#define MFP_BASE    DT_INST_REG_ADDR_BY_NAME(0, mfp)
#define MFOS_BASE   DT_INST_REG_ADDR_BY_NAME(0, mfos)
#define GPA_BASE    DT_REG_ADDR(DT_NODELABEL(gpioa))
#define GPIO_SIZE   DT_REG_SIZE(DT_NODELABEL(gpioa))

#define SLEWCTL_PIN_SHIFT(pin_idx)	((pin_idx) * 2)
#define SLEWCTL_MASK(pin_idx)		(3 << SLEWCTL_PIN_SHIFT(pin_idx))
#define DINOFF_PIN_SHIFT(pin_idx)	(pin_idx + GPIO_DINOFF_DINOFF0_Pos)
#define DINOFF_MASK(pin_idx)		(1 << DINOFF_PIN_SHIFT(pin_idx))

static void gpio_configure(const pinctrl_soc_pin_t *pin, uint8_t port_idx, uint8_t pin_idx)
{
	GPIO_T *port;

	port = (GPIO_T *)(GPA_BASE + port_idx * GPIO_SIZE);

	port->SMTEN = (port->SMTEN & ~BIT(pin_idx)) |
		      ((pin->schmitt_enable ? 1 : 0) << pin_idx);
	port->SLEWCTL = (port->SLEWCTL & ~SLEWCTL_MASK(pin_idx)) |
			(pin->slew_rate << SLEWCTL_PIN_SHIFT(pin_idx));
	port->DINOFF = (port->DINOFF & ~DINOFF_MASK(pin_idx)) |
		       ((pin->digital_disable ? 1 : 0) << DINOFF_PIN_SHIFT(pin_idx));
}
/**
 * Configure pin multi-function
 */
static void configure_pin(const pinctrl_soc_pin_t *pin)
{
	uint32_t pin_mux = pin->pin_mux;
	uint8_t pin_index = PIN_INDEX(pin_mux);
	uint8_t port_index = PORT_INDEX(pin_mux);
	uint32_t mfp_cfg = MFP_CFG(pin_mux);


	uint32_t *GPx_MFPx = ((uint32_t *)MFP_BASE) + port_index * 4 + (pin_index / 4);
	uint32_t *GPx_MFOSx = ((uint32_t *)MFOS_BASE) + port_index;
	uint32_t pinMask = NU_MFP_MASK(pin_index);

	/*
	 * E.g.: SYS->GPA_MFP0  = (SYS->GPA_MFP0 & (~SYS_GPA_MFP0_PA0MFP_Msk) ) |
	 * SYS_GPA_MFP0_PA0MFP_SC0_CD;
	 */
	*GPx_MFPx = (*GPx_MFPx & (~pinMask)) | mfp_cfg;
	if (pin->open_drain != 0) {
		*GPx_MFOSx |= BIT(pin_index);
	} else {
		*GPx_MFOSx &= ~BIT(pin_index);
	}

	gpio_configure(pin, port_index, pin_index);
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
