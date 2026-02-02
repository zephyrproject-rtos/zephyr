/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mcux_slcd_lcd.h"
#include "mcux_slcd_lcd_panel.h"

#include <zephyr/logging/log.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/util.h>

#include <zephyr/toolchain/common.h>

#include <errno.h>

#include <fsl_slcd.h>

LOG_MODULE_REGISTER(auxdisplay_mcux_slcd_lcd_s401m16kr, CONFIG_AUXDISPLAY_LOG_LEVEL);

#define PANEL_COM_PINS_MAX  (4U)
#define PANEL_DATA_PINS_MAX (8U)
#define PANEL_DIGITS_MAX    (4U)

/*
 * Panel-specific frontplane usage:
 * - Each digit uses two frontplane indices: (digit * 2) and (digit * 2 + 1).
 * - Some panels may share extra symbols (e.g. COL) on an existing digit pin.
 */
static size_t panel_digits_capacity(uint8_t d_pins_cnt)
{
	size_t digits = 0U;

	while (digits < PANEL_DIGITS_MAX) {
		uint8_t last_idx = (uint8_t)(digits * 2U + 1U);

		if (last_idx >= d_pins_cnt) {
			break;
		}
		digits++;
	}

	return digits;
}

static int panel_backplane_setting(void *base, const uint8_t *com_pins, int pin_count)
{
	if (pin_count != PANEL_COM_PINS_MAX) {
		return -EINVAL;
	}

#if CONFIG_AUXDISPLAY_LOG_LEVEL >= LOG_LEVEL_DBG
	LOG_DBG("slcd apply: com_pins=COM0:%u,COM1:%u,COM2:%u,COM3:%u", (unsigned int)com_pins[0],
		(unsigned int)com_pins[1], (unsigned int)com_pins[2], (unsigned int)com_pins[3]);
#endif
	SLCD_SetBackPlanePhase(base, com_pins[0], kSLCD_PhaseAActivate);
	SLCD_SetBackPlanePhase(base, com_pins[1], kSLCD_PhaseBActivate);
	SLCD_SetBackPlanePhase(base, com_pins[2], kSLCD_PhaseCActivate);
	SLCD_SetBackPlanePhase(base, com_pins[3], kSLCD_PhaseDActivate);

	return 0;
}

static void panel_apply_colon_fallback(uint8_t *pin_val, size_t pin_val_len, uint8_t colon_mask)
{
	ARG_UNUSED(pin_val_len);

	/* Fallback: COM0 + D7 => phase A on D7 (colon between digits 2 and 3). */
	if ((colon_mask & BIT(2)) != 0U) {
		pin_val[7] |= MCUX_SLCD_PHASE_A;
	}
}

/*
 * Dot/colon position allow lists are provided by devicetree (panel child node)
 * and handled by the controller driver.
 */

static void panel_apply(void *base, const uint8_t *d_pins, const uint8_t *digits,
			uint8_t colon_mask, uint32_t icon_mask, const void *cfg)
{
	const struct mcux_slcd_panel_cfg *panel_cfg = (const struct mcux_slcd_panel_cfg *)cfg;
	LCD_Type *slcd_base = (LCD_Type *)base;
	uint8_t pin_val[8] = {0U};
	size_t digits_count = PANEL_DIGITS_MAX;

	ARG_UNUSED(icon_mask);

	/* Use DT-provided d-pins count (already validated by controller). */
	uint8_t d_cnt = PANEL_DATA_PINS_MAX;

	if ((panel_cfg != NULL) && (panel_cfg->d_pins_cnt < d_cnt)) {
		d_cnt = panel_cfg->d_pins_cnt;
	}

	if (panel_cfg != NULL) {
		/* Prefer DT-provided max-digits (binding default = panel max). */
		if (panel_cfg->max_digits > 0U) {
			digits_count = MIN((size_t)panel_cfg->max_digits, digits_count);
		}
	}

	/* Clamp digit count to what the panel mapping can address with the provided d-pins. */
	digits_count = MIN(digits_count, panel_digits_capacity(d_cnt));

	/* LCD-S401M16KR pin table mapping (4 digits, frontplane D0..D7):
	 * - Each frontplane Dx carries up to 4 segments, one per COM phase (A..D).
	 * - Digits are numbered 1..4, and the table wiring is:
	 *   - D0: 1D/1E/1G/1F on phases A/B/C/D (COM0/1/2/3)
	 *   - D1: 1DP/1C/1B/1A on phases A/B/C/D
	 *   - D2: 2D/2E/2G/2F on phases A/B/C/D
	 *   - D3: 2DP/2C/2B/2A on phases A/B/C/D
	 *   - D4: 3D/3E/3G/3F on phases A/B/C/D
	 *   - D5: 3DP/3C/3B/3A on phases A/B/C/D
	 *   - D6: 4D/4E/4G/4F on phases A/B/C/D
	 *   - D7: COL/4C/4B/4A on phases A/B/C/D
	 */
	for (size_t digit = 0U; digit < digits_count; digit++) {
		uint8_t segs = digits[digit];
		uint8_t d_pin0 = (uint8_t)(digit * 2U + 0U);
		uint8_t d_pin1 = (uint8_t)(digit * 2U + 1U);

		if ((segs & SEG_D) != 0U) {
			pin_val[d_pin0] |= MCUX_SLCD_PHASE_A;
		}
		if ((segs & SEG_E) != 0U) {
			pin_val[d_pin0] |= MCUX_SLCD_PHASE_B;
		}
		if ((segs & SEG_G) != 0U) {
			pin_val[d_pin0] |= MCUX_SLCD_PHASE_C;
		}
		if ((segs & SEG_F) != 0U) {
			pin_val[d_pin0] |= MCUX_SLCD_PHASE_D;
		}

		if ((segs & SEG_DP) != 0U) {
			pin_val[d_pin1] |= MCUX_SLCD_PHASE_A;
		}
		if ((segs & SEG_C) != 0U) {
			pin_val[d_pin1] |= MCUX_SLCD_PHASE_B;
		}
		if ((segs & SEG_B) != 0U) {
			pin_val[d_pin1] |= MCUX_SLCD_PHASE_C;
		}
		if ((segs & SEG_A) != 0U) {
			pin_val[d_pin1] |= MCUX_SLCD_PHASE_D;
		}
	}

	/* Colon support: driven by colon-routing in devicetree (pos -> d_pins index(es)). */
	mcux_slcd_panel_apply_colon_map(pin_val, ARRAY_SIZE(pin_val), colon_mask, panel_cfg,
					panel_apply_colon_fallback);

	/* If devicetree provides fewer than PANEL_DATA_PINS_MAX, clear any remaining pins. */
	for (size_t i = d_cnt; i < PANEL_DATA_PINS_MAX; i++) {
		pin_val[i] = 0U;
	}

#if CONFIG_AUXDISPLAY_LOG_LEVEL >= LOG_LEVEL_DBG
	LOG_DBG("Slcd apply: d_pins_cnt=%u", (unsigned int)d_cnt);
	for (size_t i = 0U; i < d_cnt; i++) {
		LOG_DBG("Slcd apply: d_pins[%u]=%u pin_val[%u]=0x%02x", (unsigned int)i,
			(unsigned int)d_pins[i], (unsigned int)i, pin_val[i]);
	}
#endif
	mcux_slcd_panel_commit(slcd_base, d_pins, d_cnt, pin_val);
}

static const struct mcux_slcd_panel_api panel_api = {
	.name = "s401m16kr",
	.max_digits = PANEL_DIGITS_MAX,
	.com_pins_count = PANEL_COM_PINS_MAX,
	.d_pins_count = PANEL_DATA_PINS_MAX,
	.backplane_setting = panel_backplane_setting,
	.encode_char = mcux_slcd_lcd_encode_char,
	.apply = panel_apply,
};

const STRUCT_SECTION_ITERABLE(mcux_slcd_panel_api, mcux_slcd_panel_s401m16kr) = panel_api;
