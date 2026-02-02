/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mcux_slcd_lcd_panel.h"

#include <zephyr/sys/util.h>

#include <fsl_slcd.h>

void mcux_slcd_panel_apply_colon_map(uint8_t *pin_val, size_t pin_val_len, uint8_t colon_mask,
				     const struct mcux_slcd_panel_cfg *panel_cfg,
				     mcux_slcd_panel_colon_fallback_t fallback)
{
	if ((panel_cfg != NULL) && (panel_cfg->colon_map != NULL) &&
	    (panel_cfg->colon_map_len >= 3U)) {
		for (uint8_t i = 0U; (i + 2U) < panel_cfg->colon_map_len; i = (uint8_t)(i + 3U)) {
			uint8_t pos = panel_cfg->colon_map[i + 0U];
			uint8_t d0 = panel_cfg->colon_map[i + 1U];
			uint8_t d1 = panel_cfg->colon_map[i + 2U];

			if ((colon_mask & (uint8_t)BIT(pos)) == 0U) {
				continue;
			}

			if (d0 < pin_val_len) {
				pin_val[d0] |= MCUX_SLCD_PHASE_A;
			}
			if ((d1 != 255U) && (d1 < pin_val_len)) {
				pin_val[d1] |= MCUX_SLCD_PHASE_A;
			}
		}

		return;
	}

	if (fallback != NULL) {
		fallback(pin_val, pin_val_len, colon_mask);
	}
}

void mcux_slcd_panel_commit(void *slcd_base, const uint8_t *d_pins, size_t d_pins_count,
			    const uint8_t *pin_val)
{
	LCD_Type *base = (LCD_Type *)slcd_base;

	for (size_t i = 0U; i < d_pins_count; i++) {
		SLCD_SetFrontPlaneSegments(base, d_pins[i], pin_val[i]);
	}
}
