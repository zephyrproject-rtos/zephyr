/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_AUXDISPLAY_MCUX_SLCD_LCD_PANEL_H_
#define ZEPHYR_DRIVERS_AUXDISPLAY_MCUX_SLCD_LCD_PANEL_H_

#include "mcux_slcd_lcd.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Common helpers used by multiple MCUX SLCD LCD panel backends.
 *
 * These helpers are intentionally kept small:
 * - Each panel keeps its own digit/segment-to-frontplane wiring table.
 * - Common handling (colon routing + committing to hardware) is shared.
 */

typedef void (*mcux_slcd_panel_colon_fallback_t)(uint8_t *pin_val, size_t pin_val_len,
						 uint8_t colon_mask);

/**
 * @brief Apply a devicetree-provided colon routing map.
 *
 * colon-routing (stored in panel_cfg->colon_map) is a byte array of triples: <pos d0 d1>
 * - pos: colon position bit index in colon_mask
 * - d0/d1: indices into pin_val[] / d_pins[] (frontplane index)
 * - d1 may be 255 meaning "no second segment"
 *
 * When colon routing is missing/invalid, @p fallback is called.
 */
void mcux_slcd_panel_apply_colon_map(uint8_t *pin_val, size_t pin_val_len, uint8_t colon_mask,
				     const struct mcux_slcd_panel_cfg *panel_cfg,
				     mcux_slcd_panel_colon_fallback_t fallback);

/**
 * @brief Commit frontplane segment values to the SLCD peripheral.
 */
void mcux_slcd_panel_commit(void *slcd_base, const uint8_t *d_pins, size_t d_pins_count,
			    const uint8_t *pin_val);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_AUXDISPLAY_MCUX_SLCD_LCD_PANEL_H_ */
