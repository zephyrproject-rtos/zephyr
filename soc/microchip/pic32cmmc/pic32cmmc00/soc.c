/*
 * Copyright (c) 2025 GP Orcullo
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include "pic32cmmc00.h"

void soc_reset_hook(void)
{
	uint32_t data;

	/* Configure flash wait state to 3 @ 48MHz */
	data = FIELD_PREP(NVMCTRL_CTRLB_RWS_MASK, 3);
	sys_write32(data, NVMCTRL_CTRLB);

	/* Apply factory calibration to internal osc */
	data = FIELD_GET(NVM_SW_CAL_CAL48M_MASK, sys_read64(NVM_SW_CAL_ADDR));
	sys_write32(FIELD_PREP(OSCCTRL_CAL48M_MASK, data), OSCCTRL_CAL48M);

	/* Enable internal 48MHz osc */
	sys_write8(0, OSCCTRL_OSC48MDIV);

	while (sys_test_bit(OSCCTRL_OSC48MSYNCBUSY, OSCCTRL_OSC48MSYNCBUSY_OSC48MDIV_BIT)) {
	}

	while (!sys_test_bit(OSCCTRL_STATUS, OSCCTRL_STATUS_OSC48MRDY_BIT)) {
	}

	/* Configure mclk, no divisor */
	sys_write8(MCLK_CPUDIV_CPUDIV_DIV1_VAL, MCLK_CPUDIV);

	/* Configure gclk to use internal osc */
	data = FIELD_PREP(GCLK_GENCTRL_DIV_MASK, 1) | GCLK_GENCTRL_GENEN |
	       FIELD_PREP(GCLK_GENCTRL_SRC_MASK, GCLK_GENCTRL_SRC_OSC48M_VAL);
	sys_write32(data, GCLK_GENCTRL0);

	while (sys_test_bit(GCLK_SYNCBUSY, GCLK_SYNCBUSY_GENCTRL0_BIT)) {
	}
}
