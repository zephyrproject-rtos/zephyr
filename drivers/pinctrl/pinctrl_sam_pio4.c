/*
 * Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/drivers/pinctrl.h>
#include <soc.h>

#define SAM_PIO_NPINS_PER_BANK          32
#define SAM_PIO_BANK(pin_id)            (pin_id / SAM_PIO_NPINS_PER_BANK)
#define SAM_PIO_LINE(pin_id)            (pin_id % SAM_PIO_NPINS_PER_BANK)
#define SAM_PIO_BANK_OFFSET             0x40

#define SAM_GET_PIN_NO(pinmux)          ((pinmux) & 0xff)
#define SAM_GET_PIN_FUNC(pinmux)        ((pinmux >> 16) & 0xf)
#define SAM_GET_PIN_IOSET(pinmux)       ((pinmux >> 20) & 0xf)

static pio_registers_t * const pio_reg =
	(pio_registers_t *)DT_REG_ADDR(DT_NODELABEL(pinctrl));

static void pinctrl_configure_pin(pinctrl_soc_pin_t pin)
{
	uint32_t pin_id = SAM_GET_PIN_NO(pin.pin_mux);
	uint32_t bank = SAM_PIO_BANK(pin_id);
	uint32_t line = SAM_PIO_LINE(pin_id);
	uint32_t func = SAM_GET_PIN_FUNC(pin.pin_mux);
	uint32_t conf = 0;

	pio_reg->PIO_GROUP[bank].PIO_MSKR = 1 << line;

	conf = pio_reg->PIO_GROUP[bank].PIO_CFGR;
	if (pin.drive_open_drain) {
		conf |= PIO_CFGR_OPD(PIO_CFGR_OPD_ENABLED_Val);
	}
	if (pin.bias_disable) {
		conf &= ~(PIO_CFGR_PUEN_Msk | PIO_CFGR_PDEN_Msk);
	}
	if (pin.bias_pull_down) {
		conf |= PIO_CFGR_PDEN(PIO_CFGR_PDEN_ENABLED_Val);
		conf &= ~PIO_CFGR_PUEN_Msk;
	}
	if (pin.bias_pull_up) {
		conf |= PIO_CFGR_PUEN(PIO_CFGR_PUEN_ENABLED_Val);
		conf &= ~PIO_CFGR_PDEN_Msk;
	}
	conf &= ~PIO_CFGR_FUNC_Msk;
	conf |= PIO_CFGR_FUNC(func);

	pio_reg->PIO_GROUP[bank].PIO_CFGR = conf;
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
			   uintptr_t reg)
{
	ARG_UNUSED(reg);

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		pinctrl_configure_pin(*pins++);
	}

	return 0;
}
