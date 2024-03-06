/*
 * Copyright (c) 2024 Sendrato
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/init.h>

#if !defined(CONFIG_SOC_SERIES_LPC11U6X)
#include <fsl_clock.h>
#endif

#define OFFSET(mux) (((mux) & 0xFFF00000) >> 20)
#define TYPE(mux) (((mux) & 0xC0000) >> 18)

#define IOCON_TYPE_D 0x0
#define IOCON_TYPE_I 0x1

static volatile uint32_t *iocon =
	(volatile uint32_t *)DT_REG_ADDR(DT_NODELABEL(iocon));

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
			   uintptr_t reg)
{
	for (uint8_t i = 0u; i < pin_cnt; i++) {
		uint32_t pin_mux = pins[i];
		uint32_t offset = OFFSET(pin_mux);

		/* Check if this is an analog or i2c type pin */
		switch (TYPE(pin_mux)) {
		case IOCON_TYPE_D:
			pin_mux &= Z_PINCTRL_IOCON_D_PIN_MASK;
			break;
		case IOCON_TYPE_I:
			pin_mux &= Z_PINCTRL_IOCON_I_PIN_MASK;
			break;
		default:
			/* Should not occur */
			__ASSERT_NO_MSG(TYPE(pin_mux) <= IOCON_TYPE_A);
		}
		/* Set pinmux */
		*(iocon + offset) = pin_mux;
	}
	return 0;
}
