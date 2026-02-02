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

LOG_MODULE_REGISTER(auxdisplay_mcux_slcd_lcd_s401m16kr, CONFIG_AUXDISPLAY_LOG_LEVEL);

#define NUM_COM_PINS  (4U)
#define NUM_DATA_PINS (8U)
#define NUM_DIGITS    (4U)

static int panel_backplane_setting(void *base, const uint8_t *com_pins, int pin_count)
{
	if (pin_count != NUM_COM_PINS) {
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

static bool panel_dot_pos_allow(int pos)
{
	/* LCD-S401M16KR has DP on digits 1..3; digit 4 uses COL on D7 instead. */
	return (pos >= 0) && (pos < 3);
}

static bool panel_col_pos_allow(int pos)
{
	/* colon is between digits 2 and 3 */
	return (pos == 2);
}

static void panel_apply(void *base, const uint8_t *d_pins, const uint8_t *digits,
			uint8_t colon_mask)
{
	LCD_Type *slcd_base = (LCD_Type *)base;
	uint8_t pin_val[8] = {0U};

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
	for (size_t digit = 0U; digit < NUM_DIGITS; digit++) {
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
	/* Colon segment support: COM0 + D7 => phase A on D7. */
	if ((colon_mask & BIT(2)) != 0U) {
		pin_val[7] |= MCUX_SLCD_PHASE_A;
	}

#if CONFIG_AUXDISPLAY_LOG_LEVEL >= LOG_LEVEL_DBG
	LOG_DBG("Slcd apply: d_pins=%u,%u,%u,%u,%u,%u,%u,%u", (unsigned int)d_pins[0],
		(unsigned int)d_pins[1], (unsigned int)d_pins[2], (unsigned int)d_pins[3],
		(unsigned int)d_pins[4], (unsigned int)d_pins[5], (unsigned int)d_pins[6],
		(unsigned int)d_pins[7]);
	LOG_DBG("Slcd apply: pin_val=%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x", pin_val[0],
		pin_val[1], pin_val[2], pin_val[3], pin_val[4], pin_val[5], pin_val[6], pin_val[7]);
#endif

	for (size_t i = 0U; i < 8U; i++) {
#if CONFIG_AUXDISPLAY_LOG_LEVEL >= LOG_LEVEL_DBG
		LOG_DBG("Slcd apply: d_pins[%u]=%u pin_val[%u]=0x%02x", (unsigned int)i,
			(unsigned int)d_pins[i], (unsigned int)i, pin_val[i]);
#endif
		SLCD_SetFrontPlaneSegments(slcd_base, d_pins[i], pin_val[i]);
	}
}

static const struct mcux_slcd_panel_api panel_api = {
	.name = "LCD-S401M16KR",
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
