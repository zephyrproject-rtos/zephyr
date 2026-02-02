/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_AUXDISPLAY_MCUX_SLCD_LCD_H_
#define ZEPHYR_DRIVERS_AUXDISPLAY_MCUX_SLCD_LCD_H_

#include <stdint.h>
#include <stddef.h>

#include <stdbool.h>

#include <zephyr/autoconf.h>
#include <zephyr/sys/util.h>

/**
 * @brief Compile-time allocation for the internal digit buffer.
 *
 * This is an upper bound; the active panel's `max_digits` controls how many
 * digits are actually used.
 */
#define MCUX_SLCD_MAX_DIGITS 8

/**
 * @name Segment bit layout
 *
 * Segment bit layout in the internal per-digit buffer.
 *
 * This is a shared convention between mcux_slcd_lcd_encode_char() and each
 * panel backend's apply() implementation.
 *
 * @{
 */
/** @brief Segment A bit */
#define SEG_A  BIT(0)
/** @brief Segment B bit */
#define SEG_B  BIT(1)
/** @brief Segment C bit */
#define SEG_C  BIT(2)
/** @brief Segment D bit */
#define SEG_D  BIT(3)
/** @brief Segment E bit */
#define SEG_E  BIT(4)
/** @brief Segment F bit */
#define SEG_F  BIT(5)
/** @brief Segment G bit */
#define SEG_G  BIT(6)
/** @brief Decimal point (DP) bit */
#define SEG_DP BIT(7)
/** @} */

/**
 * @name SLCD COM phase bit layout
 *
 * Bit positions used in SLCD waveform registers (WFx) to represent the time-
 * multiplexed COM/backplane phases.
 *
 * Hardware meaning (from the SLCD IP): bit0..bit7 correspond to phases A..H.
 *
 * How they are used:
 * - For a **frontplane pin** (segment): WFn bits select during which COM phase(s)
 *   the segment is driven ON.
 * - For a **backplane (COM) pin**: WFn bits assign which phase(s) that COM pin
 *   is driven/active.
 *
 * @{
 */
/** @brief COM phase A bit */
#define MCUX_SLCD_PHASE_A BIT(0)
/** @brief COM phase B bit */
#define MCUX_SLCD_PHASE_B BIT(1)
/** @brief COM phase C bit */
#define MCUX_SLCD_PHASE_C BIT(2)
/** @brief COM phase D bit */
#define MCUX_SLCD_PHASE_D BIT(3)
/** @brief COM phase E bit */
#define MCUX_SLCD_PHASE_E BIT(4)
/** @brief COM phase F bit */
#define MCUX_SLCD_PHASE_F BIT(5)
/** @brief COM phase G bit */
#define MCUX_SLCD_PHASE_G BIT(6)
/** @brief COM phase H bit */
#define MCUX_SLCD_PHASE_H BIT(7)
/** @} */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Panel abstraction for different SLCD glass wirings.
 */
struct mcux_slcd_panel_api {
	/** Human-readable panel name */
	const char *name;
	/** Maximum number of digits supported by this panel */
	uint8_t max_digits;
	/** Required number of SLCD backplane pins (COM pins).
	 *
	 * 0 means the backend does not enforce a fixed COM pin count.
	 */
	uint8_t com_pins_count;
	/** Number of SLCD frontplane pins supported by this panel (D pins max). */
	uint8_t d_pins_count;
	/** Configure SLCD backplane (COM) pins and phase assignments */
	int (*backplane_setting)(void *base, const uint8_t *com_pins, int pin_count);
	/** Encode an ASCII character into internal segment bitmask */
	uint8_t (*encode_char)(uint8_t ch, bool allow_dot);
	/**
	 * Apply the current digit/segment buffer to SLCD frontplane pins.
	 *
	 * @param base SLCD peripheral base.
	 * @param d_pins Frontplane pins array (LCD_Pn indices).
	 * @param digits Encoded digits array.
	 * @param colon_mask Colon enable mask, where BIT(n) enables colon at digit index n.
	 * @param icon_mask Panel-specific icon enable mask.
	 * @param cfg Panel configuration blob from devicetree (panel child node).
	 */
	void (*apply)(void *base, const uint8_t *d_pins, const uint8_t *digits, uint8_t colon_mask,
		      uint32_t icon_mask, const void *cfg);
};

/**
 * @brief Devicetree-derived panel configuration.
 *
 * This struct is instantiated per SLCD controller instance (because the panel
 * child node lives under a specific controller node).
 */
struct mcux_slcd_panel_cfg {
	const uint8_t *com_pins;
	uint8_t com_pins_cnt;
	const uint8_t *d_pins;
	uint8_t d_pins_cnt;
	uint8_t max_digits;
	uint8_t dot_pos_mask;   /* BIT(pos) indicates '.' supported */
	uint8_t colon_pos_mask; /* BIT(pos) indicates ':' supported */
	const uint8_t *colon_map; /* array of triples: pos, d_idx0, d_idx1 */
	uint8_t colon_map_len;    /* number of bytes in colon_map */
	bool rotate_180;
};

BUILD_ASSERT(MCUX_SLCD_MAX_DIGITS <= 8,
	     "colon_mask uses 8-bit positions; increase mask width if needed");

/**
 * @brief Encode an ASCII character into the internal segment bitmask.
 *
 * This function is used by panel backends as their `encode_char()` callback.
 *
 * The default implementation supports:
 * - Digits '0'..'9'
 * - '-' (segment G)
 * - ' ' (blank)
 * - Optional decimal point (DP) when @p allow_dot is true
 *
 * Applications may override this function (provide a non-weak definition with the
 * same name) to implement custom character sets.
 *
 * @param ch ASCII character.
 * @param allow_dot When true, include DP in the returned segment mask.
 *
 * @return Segment bitmask in the shared A..G+DP layout.
 */
uint8_t mcux_slcd_lcd_encode_char(uint8_t ch, bool allow_dot);

/**
 * @brief Panel APIs exported by this driver.
 *
 * Panel selection is performed per devicetree instance.
 */
/*
 * Panel backends register themselves using STRUCT_SECTION_ITERABLE().
 *
 * The SLCD controller driver looks up the requested panel at runtime by
 * matching the devicetree "panel" string to mcux_slcd_panel_api.name.
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_AUXDISPLAY_MCUX_SLCD_LCD_H_ */
