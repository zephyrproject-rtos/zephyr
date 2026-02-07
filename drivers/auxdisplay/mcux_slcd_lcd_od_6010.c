/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mcux_slcd_lcd.h"

#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain/common.h>

#include <errno.h>

#include <fsl_slcd.h>

LOG_MODULE_REGISTER(auxdisplay_mcux_slcd_lcd_od_6010, CONFIG_AUXDISPLAY_LOG_LEVEL);

#define NUM_COM_PINS  (4U)
#define NUM_DATA_PINS (12U)
#define NUM_DIGITS    (6U)

static int panel_backplane_setting(void *base, const uint8_t *com_pins, int pin_count)
{
	if (pin_count != NUM_COM_PINS) {
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

static bool panel_dot_pos_allow(int pos)
{
	/* OD-6010 has DP on digits 2..5 (1-based), i.e. indices 1..4 (0-based). */
	return (pos >= 1) && (pos < 5);
}

static bool panel_col_pos_allow(int pos)
{
	/*
	 * OD-6010 supports 3 colon "pairs" (implemented with P1..P6 dots).
	 *
	 * The auxdisplay core derives colon state from the current cursor position at
	 * the time ':' is encountered. Depending on how a user formats the write
	 * stream, the second separator in strings like "12:12:12" may land at cursor
	 * position 5. Allow pos 5 as an alias for the rightmost separator.
	 */
	return (pos == 2) || (pos == 3) || (pos == 4) || (pos == 5);
}

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

static void panel_apply(void *base, const uint8_t *d_pins, const uint8_t *digits,
			uint8_t colon_mask)
{
	LCD_Type *slcd_base = (LCD_Type *)base;
	uint8_t pin_val[NUM_DATA_PINS] = {0U};

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
	for (size_t digit = 0U; digit < NUM_DIGITS; digit++) {
		uint8_t segs = od_6010_remap_segments(digits[digit]);
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

	/*
	 * Colon support (OD-6010): there is no dedicated ':' segment.
	 * The glass provides 6 "P" points (P1..P6) which form 3 vertical pairs:
	 *   - Pair #1: P1 (top) + P4 (bottom)
	 *   - Pair #2: P2 (top) + P5 (bottom)
	 *   - Pair #3: P3 (top) + P6 (bottom)
	 *
	 * Electrically, each Pn is PHASE_A on one of the SEG2/4/6/8/10/12 lines:
	 *   - pin_val[7]  (SEG8)  : P1
	 *   - pin_val[5]  (SEG6)  : P2
	 *   - pin_val[3]  (SEG4)  : P3
	 *   - pin_val[9]  (SEG10) : P4
	 *   - pin_val[11] (SEG12) : P5
	 *   - pin_val[1]  (SEG2)  : P6
	 *
	 * We map logical ':' positions (digit index) to these 3 pairs (left->right):
	 *   - pos 2: between digit2 and digit3 -> P3 + P6
	 *   - pos 3: between digit3 and digit4 -> P2 + P5
	 *   - pos 4: between digit4 and digit5 -> P1 + P4
	 */
	if ((colon_mask & BIT(2)) != 0U) {
		pin_val[3] |= MCUX_SLCD_PHASE_A; /* P3 */
		pin_val[1] |= MCUX_SLCD_PHASE_A; /* P6 */
	}
	if ((colon_mask & BIT(3)) != 0U) {
		pin_val[5] |= MCUX_SLCD_PHASE_A;  /* P2 */
		pin_val[11] |= MCUX_SLCD_PHASE_A; /* P5 */
	}
	if ((colon_mask & BIT(4)) != 0U) {
		pin_val[7] |= MCUX_SLCD_PHASE_A; /* P1 */
		pin_val[9] |= MCUX_SLCD_PHASE_A; /* P4 */
	}

#if CONFIG_AUXDISPLAY_LOG_LEVEL >= LOG_LEVEL_DBG
	LOG_DBG("Slcd apply: d_pins=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u", (unsigned int)d_pins[0],
		(unsigned int)d_pins[1], (unsigned int)d_pins[2], (unsigned int)d_pins[3],
		(unsigned int)d_pins[4], (unsigned int)d_pins[5], (unsigned int)d_pins[6],
		(unsigned int)d_pins[7], (unsigned int)d_pins[8], (unsigned int)d_pins[9],
		(unsigned int)d_pins[10], (unsigned int)d_pins[11]);
	LOG_DBG("Slcd apply: pin_val=%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x",
		pin_val[0], pin_val[1], pin_val[2], pin_val[3], pin_val[4], pin_val[5], pin_val[6],
		pin_val[7], pin_val[8], pin_val[9], pin_val[10], pin_val[11]);
#endif

	/*
	 * Some applications may place the second separator at cursor position 5
	 * (e.g. depending on how the write stream is chunked). Treat pos 5 as an
	 * alias of the rightmost separator (pos 4).
	 */
	if ((colon_mask & BIT(5)) != 0U) {
		pin_val[7] |= MCUX_SLCD_PHASE_A; /* P1 */
		pin_val[9] |= MCUX_SLCD_PHASE_A; /* P4 */
	}

	for (size_t i = 0U; i < NUM_DATA_PINS; i++) {
		SLCD_SetFrontPlaneSegments(slcd_base, d_pins[i], pin_val[i]);
	}
}

static const struct mcux_slcd_panel_api panel_api = {
	.name = "OD-6010",
	.max_digits = NUM_DIGITS,
	.d_pins_count = NUM_DATA_PINS,
	.backplane_setting = panel_backplane_setting,
	.encode_char = mcux_slcd_lcd_encode_char,
	.dot_pos_allow = panel_dot_pos_allow,
	.col_pos_allow = panel_col_pos_allow,
	.apply = panel_apply,
};

const struct mcux_slcd_panel_api *mcux_slcd_lcd_panel_get(void)
{
	return &panel_api;
}
