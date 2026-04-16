/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __AUXDISPLAY_SLCD_CONFIG_H_
#define __AUXDISPLAY_SLCD_CONFIG_H_

#include <zephyr/drivers/auxdisplay.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>

#define SLCD_BLANK (0)

struct auxdisplay_panel_config {
	struct auxdisplay_capabilities capabilities;
	const uint8_t segment_type;
	const uint8_t *digits; /* Digits coding array. */
	const uint8_t *digits_rotated_180;
	const uint8_t *letters_upper; /* Upper Letter coding array. */
	const uint8_t *letters_upper_rotated_180;
	const uint8_t *letters_lower; /* Lower Letter coding array. */
	const uint8_t *letters_lower_rotated_180;
	/*
	 * Pin/com configurations of the segments/indicators read from the
	 * panel overlay configuration.
	 */
	const uint8_t *segment_pins;
	const uint8_t *segment_coms;
	const uint8_t num_indicators;
	const uint8_t *indicator_pins; /* Optional. NULL if DT property absent */
	const uint8_t *indicator_coms; /* Optional. NULL if DT property absent */
	const uint8_t *col_indicators; /* Optional. NULL if DT property absent */
	const uint8_t *upper_dot_indicators; /* Optional. NULL if DT property absent */
	const uint8_t *lower_dot_indicators; /* Optional. NULL if DT property absent */
};

/**
 * @brief 7-segment digit coding tables.
 *
 * Standard 7-segment encoding (Bit 0=A, 1=B, 2=C, 3=D, 4=E, 5=F, 6=G):
 *    AAA
 *   F   B
 *    GGG
 *   E   C
 *    DDD
 *
 * Rotated 180° encoding:
 *    DDD
 *   C   E
 *    GGG
 *   B   F
 *    AAA
 *
 * By default 7-segment display does not support letters, only digits.
 * User can override this by providing custom letter coding tables
 * downstream.
 */
#define SLCD_PANEL_CODING_7SEG							\
	static const uint8_t digits[] = {					\
		[0] = BIT(0)|BIT(1)|BIT(2)|BIT(3)|BIT(4)|BIT(5),		\
		[1] = BIT(1)|BIT(2),						\
		[2] = BIT(0)|BIT(1)|BIT(3)|BIT(4)|BIT(6),			\
		[3] = BIT(0)|BIT(1)|BIT(2)|BIT(3)|BIT(6),			\
		[4] = BIT(1)|BIT(2)|BIT(5)|BIT(6),				\
		[5] = BIT(0)|BIT(2)|BIT(3)|BIT(5)|BIT(6),			\
		[6] = BIT(0)|BIT(2)|BIT(3)|BIT(4)|BIT(5)|BIT(6),		\
		[7] = BIT(0)|BIT(1)|BIT(2),					\
		[8] = BIT(0)|BIT(1)|BIT(2)|BIT(3)|BIT(4)|BIT(5)|BIT(6),		\
		[9] = BIT(0)|BIT(1)|BIT(2)|BIT(3)|BIT(5)|BIT(6),		\
	};									\
	static const uint8_t digits_rotated_180[] = {				\
		[0] = BIT(0)|BIT(1)|BIT(2)|BIT(3)|BIT(4)|BIT(5),		\
		[1] = BIT(4)|BIT(5),						\
		[2] = BIT(0)|BIT(1)|BIT(3)|BIT(4)|BIT(6),			\
		[3] = BIT(0)|BIT(3)|BIT(4)|BIT(5)|BIT(6),			\
		[4] = BIT(2)|BIT(4)|BIT(5)|BIT(6),				\
		[5] = BIT(0)|BIT(2)|BIT(3)|BIT(5)|BIT(6),			\
		[6] = BIT(0)|BIT(1)|BIT(2)|BIT(3)|BIT(5)|BIT(6),		\
		[7] = BIT(3)|BIT(4)|BIT(5),					\
		[8] = BIT(0)|BIT(1)|BIT(2)|BIT(3)|BIT(4)|BIT(5)|BIT(6),		\
		[9] = BIT(0)|BIT(2)|BIT(3)|BIT(4)|BIT(5)|BIT(6),		\
	};									\
	static const uint8_t letters_upper[] = {0};				\
	static const uint8_t letters_upper_rotated_180[] = {0};			\
	static const uint8_t letters_lower[] = {0};				\
	static const uint8_t letters_lower_rotated_180[] = {0};

/* Pass child node through; used to expand the single child of a DRV instance. */
#define _SLCD_PANEL_CHILD_NODE(child) child

#define SLCD_PANEL_NODE(n)									\
	DT_FOREACH_CHILD_STATUS_OKAY(DT_DRV_INST(n), _SLCD_PANEL_CHILD_NODE)

/* Declare optional property array, expands to nothing if property absent. */
#define SLCD_PANEL_OPTIONAL_ARRAY(n, prop)							\
	COND_CODE_1(DT_NODE_HAS_PROP(SLCD_PANEL_NODE(n), prop),					\
		(static const uint8_t slcd_opt_##prop##_##n[] =					\
			DT_PROP(SLCD_PANEL_NODE(n), prop);), ())

/* Reference optional array, return NULL if property absent. */
#define SLCD_PANEL_OPTIONAL_REF(n, prop)							\
	COND_CODE_1(DT_NODE_HAS_PROP(SLCD_PANEL_NODE(n), prop),					\
		(slcd_opt_##prop##_##n), (NULL))

/* Panel-level configurations, non-related to specific IP implementation. */
#define SLCD_PANEL_CONFIG(n)									\
	BUILD_ASSERT(DT_CHILD_NUM_STATUS_OKAY(DT_DRV_INST(n)) == 1,				\
		     "One SLCD controller can have only one panel");				\
	COND_CODE_1(IS_EQ(DT_PROP(SLCD_PANEL_NODE(n), segment_type), 7),			\
		    (SLCD_PANEL_CODING_7SEG), ())						\
	static const uint8_t slcd_segment_pins_##n[] =						\
		DT_PROP(SLCD_PANEL_NODE(n), segment_pins);					\
	static const uint8_t slcd_segment_coms_##n[] =						\
		DT_PROP(SLCD_PANEL_NODE(n), segment_coms);					\
	SLCD_PANEL_OPTIONAL_ARRAY(n, indicator_pins)						\
	SLCD_PANEL_OPTIONAL_ARRAY(n, indicator_coms)						\
	SLCD_PANEL_OPTIONAL_ARRAY(n, col_indicators)						\
	SLCD_PANEL_OPTIONAL_ARRAY(n, upper_dot_indicators)					\
	SLCD_PANEL_OPTIONAL_ARRAY(n, lower_dot_indicators)					\
	static const struct auxdisplay_panel_config slcd_panel_config_##n = {			\
		.capabilities = {								\
			.columns = DT_PROP(SLCD_PANEL_NODE(n), columns),			\
			.rows    = DT_PROP(SLCD_PANEL_NODE(n), rows),				\
		},										\
		.segment_type         = DT_PROP(SLCD_PANEL_NODE(n), segment_type),		\
		.digits               = digits,							\
		.digits_rotated_180   = digits_rotated_180,					\
		.letters_upper        = (ARRAY_SIZE(letters_upper) == 26U)			\
			? letters_upper : NULL,							\
		.letters_upper_rotated_180 =							\
			(ARRAY_SIZE(letters_upper_rotated_180) == 26U)				\
			? letters_upper_rotated_180 : NULL,					\
		.letters_lower        = (ARRAY_SIZE(letters_lower) == 26U)			\
			? letters_lower : NULL,							\
		.letters_lower_rotated_180 =							\
			(ARRAY_SIZE(letters_lower_rotated_180) == 26U)				\
			? letters_lower_rotated_180 : NULL,					\
		.segment_pins         = slcd_segment_pins_##n,					\
		.segment_coms         = slcd_segment_coms_##n,					\
		.num_indicators       = DT_PROP(SLCD_PANEL_NODE(n), num_indicators),		\
		.indicator_pins       = SLCD_PANEL_OPTIONAL_REF(n, indicator_pins),		\
		.indicator_coms       = SLCD_PANEL_OPTIONAL_REF(n, indicator_coms),		\
		.col_indicators       = SLCD_PANEL_OPTIONAL_REF(n, col_indicators),		\
		.upper_dot_indicators = SLCD_PANEL_OPTIONAL_REF(n, upper_dot_indicators),	\
		.lower_dot_indicators = SLCD_PANEL_OPTIONAL_REF(n, lower_dot_indicators),	\
	};

#endif /* __AUXDISPLAY_SLCD_CONFIG_H_ */
