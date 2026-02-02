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

LOG_MODULE_REGISTER(auxdisplay_mcux_slcd_lcd_od_6010, CONFIG_AUXDISPLAY_LOG_LEVEL);

#define PANEL_COM_PINS_MAX  (4U)
#define PANEL_DATA_PINS_MAX (12U)
#define PANEL_DIGITS_MAX    (6U)

/*
 * Panel-specific frontplane usage:
 * - Each digit uses two frontplane indices: (digit * 2) and (digit * 2 + 1).
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
	LOG_DBG("Slcd apply: com_pins=COM0:%u,COM1:%u,COM2:%u,COM3:%u", (unsigned int)com_pins[0],
		(unsigned int)com_pins[1], (unsigned int)com_pins[2], (unsigned int)com_pins[3]);
#endif

	SLCD_SetBackPlanePhase(base, com_pins[0], kSLCD_PhaseAActivate);
	SLCD_SetBackPlanePhase(base, com_pins[1], kSLCD_PhaseBActivate);
	SLCD_SetBackPlanePhase(base, com_pins[2], kSLCD_PhaseCActivate);
	SLCD_SetBackPlanePhase(base, com_pins[3], kSLCD_PhaseDActivate);

	return 0;
}

/*
 * Dot/colon position allow lists are provided by devicetree (panel child node)
 * and handled by the controller driver.
 */

/*
 * OD-6010 glass orientation note
 *
 * Some OD-6010 assemblies are mounted such that the glass segment labeling (A..G)
 * is rotated 180 degrees with respect to the logical 7-seg encoding used by
 * mcux_slcd_lcd_encode_char().
 *
 * To compensate, remap the internal segment bitmask before applying it to the
 * frontplane pins.
 */
static uint8_t od_6010_remap_segments(uint8_t segs)
{
	uint8_t out = 0U;

	/* 180-degree rotation mapping for 7-seg:
	 *  A <-> D, B <-> E, C <-> F, G stays, DP stays.
	 */
	if ((segs & SEG_A) != 0U) {
		out |= SEG_D;
	}
	if ((segs & SEG_B) != 0U) {
		out |= SEG_E;
	}
	if ((segs & SEG_C) != 0U) {
		out |= SEG_F;
	}
	if ((segs & SEG_D) != 0U) {
		out |= SEG_A;
	}
	if ((segs & SEG_E) != 0U) {
		out |= SEG_B;
	}
	if ((segs & SEG_F) != 0U) {
		out |= SEG_C;
	}
	if ((segs & SEG_G) != 0U) {
		out |= SEG_G;
	}
	if ((segs & SEG_DP) != 0U) {
		out |= SEG_DP;
	}

	return out;
}

static void panel_apply_colon_fallback(uint8_t *pin_val, size_t pin_val_len, uint8_t colon_mask)
{
	ARG_UNUSED(pin_val_len);

	/* Fallback to legacy hard-coded mapping if colon-routing is not provided. */
	if ((colon_mask & BIT(2)) != 0U) {
		pin_val[3] |= MCUX_SLCD_PHASE_A;
		pin_val[1] |= MCUX_SLCD_PHASE_A;
	}
	if ((colon_mask & BIT(3)) != 0U) {
		pin_val[5] |= MCUX_SLCD_PHASE_A;
		pin_val[11] |= MCUX_SLCD_PHASE_A;
	}
	if ((colon_mask & BIT(4)) != 0U) {
		pin_val[7] |= MCUX_SLCD_PHASE_A;
		pin_val[9] |= MCUX_SLCD_PHASE_A;
	}
	if ((colon_mask & BIT(5)) != 0U) {
		pin_val[7] |= MCUX_SLCD_PHASE_A;
		pin_val[9] |= MCUX_SLCD_PHASE_A;
	}
}

static void panel_apply(void *base, const uint8_t *d_pins, const uint8_t *digits,
			uint8_t colon_mask, uint32_t icon_mask, const void *cfg)
{
	const struct mcux_slcd_panel_cfg *panel_cfg = (const struct mcux_slcd_panel_cfg *)cfg;
	LCD_Type *slcd_base = (LCD_Type *)base;
	uint8_t pin_val[PANEL_DATA_PINS_MAX] = {0U};
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

	/* OD-6010 symbol mapping (6 digits, frontplane SEG1..SEG12):
	 * - Each SEGx carries up to 4 segments, one per COM phase (A..D).
	 * - Digit wiring (COM1..COM4 map to phases A..D via mcux_slcd_lcd_backplane_setting):
	 *   - SEG1 : 1A/1B/1C/1D on phases A/B/C/D
	 *   - SEG2 : P6/1F/1G/1E on phases A/B/C/D
	 *   - SEG3 : 2A/2B/2C/2D
	 *   - SEG4 : P3/2F/2G/2E
	 *   - SEG5 : 3A/3B/3C/3D
	 *   - SEG6 : P2/3F/3G/3E
	 *   - SEG7 : 4A/4B/4C/4D
	 *   - SEG8 : P1/4F/4G/4E
	 *   - SEG9 : 5A/5B/5C/5D
	 *   - SEG10: P4/5F/5G/5E
	 *   - SEG11: 6A/6B/6C/6D
	 *   - SEG12: P5/6F/6G/6E
	 */
	for (size_t digit = 0U; digit < digits_count; digit++) {
		uint8_t segs = digits[digit];

		if ((panel_cfg != NULL) && panel_cfg->rotate_180) {
			segs = od_6010_remap_segments(segs);
		}
		uint8_t seg_abcd = (uint8_t)(digit * 2U);
		uint8_t seg_dp_fge = (uint8_t)(digit * 2U + 1U);

		/* SEG(odd): A/B/C/D */
		if ((segs & SEG_A) != 0U) {
			pin_val[seg_abcd] |= MCUX_SLCD_PHASE_A;
		}
		if ((segs & SEG_B) != 0U) {
			pin_val[seg_abcd] |= MCUX_SLCD_PHASE_B;
		}
		if ((segs & SEG_C) != 0U) {
			pin_val[seg_abcd] |= MCUX_SLCD_PHASE_C;
		}
		if ((segs & SEG_D) != 0U) {
			pin_val[seg_abcd] |= MCUX_SLCD_PHASE_D;
		}

		/* SEG(even): DP/F/G/E */
		if ((segs & SEG_DP) != 0U) {
			pin_val[seg_dp_fge] |= MCUX_SLCD_PHASE_A;
		}
		if ((segs & SEG_F) != 0U) {
			pin_val[seg_dp_fge] |= MCUX_SLCD_PHASE_B;
		}
		if ((segs & SEG_G) != 0U) {
			pin_val[seg_dp_fge] |= MCUX_SLCD_PHASE_C;
		}
		if ((segs & SEG_E) != 0U) {
			pin_val[seg_dp_fge] |= MCUX_SLCD_PHASE_D;
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
	.name = "od-6010",
	.max_digits = PANEL_DIGITS_MAX,
	.com_pins_count = PANEL_COM_PINS_MAX,
	.d_pins_count = PANEL_DATA_PINS_MAX,
	.backplane_setting = panel_backplane_setting,
	.encode_char = mcux_slcd_lcd_encode_char,
	.apply = panel_apply,
};

const STRUCT_SECTION_ITERABLE(mcux_slcd_panel_api, mcux_slcd_panel_od_6010) = panel_api;
