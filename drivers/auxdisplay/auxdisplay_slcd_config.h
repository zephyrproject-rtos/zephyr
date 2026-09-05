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
	const bool rotated;
	const uint8_t segment_type;
	const uint16_t *digits; /* Digits coding array. */
	const uint16_t *digits_rotated_180;
	const uint16_t *letters_upper; /* Upper Letter coding array. */
	const uint16_t *letters_upper_rotated_180;
	const uint16_t *letters_lower; /* Lower Letter coding array. */
	const uint16_t *letters_lower_rotated_180;
	/*
	 * Pin/com configurations of the segments/indicators read from the
	 * panel overlay configuration.
	 */
	const uint8_t *segment_pins;
	const uint8_t *segment_coms;
	const uint8_t num_indicators;
	const uint8_t *indicator_pins;       /* Optional. NULL if DT property absent */
	const uint8_t *indicator_coms;       /* Optional. NULL if DT property absent */
	const uint8_t *col_indicators;       /* Optional. NULL if DT property absent */
	const uint8_t *upper_dot_indicators; /* Optional. NULL if DT property absent */
	const uint8_t *lower_dot_indicators; /* Optional. NULL if DT property absent */
};

static inline uint16_t slcd_char_to_pattern(const struct auxdisplay_panel_config *panel, uint8_t ch)
{
	const bool rot = panel->rotated;

	if (ch >= '0' && ch <= '9') {
		const uint16_t *tbl = rot ? panel->digits_rotated_180 : panel->digits;

		return tbl ? tbl[ch - '0'] : SLCD_BLANK;
	} else if (ch >= 'A' && ch <= 'Z') {
		const uint16_t *tbl = rot ? panel->letters_upper_rotated_180 : panel->letters_upper;

		return tbl ? tbl[ch - 'A'] : SLCD_BLANK;
	} else if (ch >= 'a' && ch <= 'z') {
		const uint16_t *tbl =
			rot ? (panel->letters_lower_rotated_180 ? panel->letters_lower_rotated_180
								: panel->letters_upper_rotated_180)
			    : (panel->letters_lower ? panel->letters_lower : panel->letters_upper);
		return tbl ? tbl[ch - 'a'] : SLCD_BLANK;
	}
	return SLCD_BLANK;
}

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
#define SLCD_PANEL_CODING_7SEG                                                                     \
	static const uint16_t digits[] = {                                                         \
		[0] = BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(4) | BIT(5),                         \
		[1] = BIT(1) | BIT(2),                                                             \
		[2] = BIT(0) | BIT(1) | BIT(3) | BIT(4) | BIT(6),                                  \
		[3] = BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(6),                                  \
		[4] = BIT(1) | BIT(2) | BIT(5) | BIT(6),                                           \
		[5] = BIT(0) | BIT(2) | BIT(3) | BIT(5) | BIT(6),                                  \
		[6] = BIT(0) | BIT(2) | BIT(3) | BIT(4) | BIT(5) | BIT(6),                         \
		[7] = BIT(0) | BIT(1) | BIT(2),                                                    \
		[8] = BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(4) | BIT(5) | BIT(6),                \
		[9] = BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(5) | BIT(6),                         \
	};                                                                                         \
	static const uint16_t digits_rotated_180[] = {                                             \
		[0] = BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(4) | BIT(5),                         \
		[1] = BIT(4) | BIT(5),                                                             \
		[2] = BIT(0) | BIT(1) | BIT(3) | BIT(4) | BIT(6),                                  \
		[3] = BIT(0) | BIT(3) | BIT(4) | BIT(5) | BIT(6),                                  \
		[4] = BIT(2) | BIT(4) | BIT(5) | BIT(6),                                           \
		[5] = BIT(0) | BIT(2) | BIT(3) | BIT(5) | BIT(6),                                  \
		[6] = BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(5) | BIT(6),                         \
		[7] = BIT(3) | BIT(4) | BIT(5),                                                    \
		[8] = BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(4) | BIT(5) | BIT(6),                \
		[9] = BIT(0) | BIT(2) | BIT(3) | BIT(4) | BIT(5) | BIT(6),                         \
	};                                                                                         \
	static const uint16_t letters_upper[] = {0};                                               \
	static const uint16_t letters_upper_rotated_180[] = {0};                                   \
	static const uint16_t letters_lower[] = {0};                                               \
	static const uint16_t letters_lower_rotated_180[] = {0};

/**
 * @brief 14-segment digit and letter coding tables.
 *
 * Segment bit assignment (alphabetical, standard 14-segment convention):
 *   Bit  0 = A   top horizontal
 *   Bit  1 = B   upper-right vertical
 *   Bit  2 = C   lower-right vertical
 *   Bit  3 = D   bottom horizontal
 *   Bit  4 = E   lower-left vertical
 *   Bit  5 = F   upper-left vertical
 *   Bit  6 = G   middle horizontal (upper half)
 *   Bit  7 = H   inner diagonal, upper-right
 *   Bit  8 = J   inner vertical, centre-right
 *   Bit  9 = K   inner diagonal, lower-right
 *   Bit 10 = M   inner vertical, centre
 *   Bit 11 = N   inner diagonal, lower-left
 *   Bit 12 = P   inner vertical, centre-left
 *   Bit 13 = Q   inner diagonal, upper-left
 *
 */
#define SLCD_PANEL_CODING_14SEG                                                                    \
	static const uint16_t digits[] = {                                                         \
		[0] = 0x223F, /* A+B+C+D+E+F+K+Q */                                                \
		[1] = 0x0006, /* B+C */                                                            \
		[2] = 0x045B, /* A+B+D+E+G+M */                                                    \
		[3] = 0x044F, /* A+B+C+D+G+M */                                                    \
		[4] = 0x0466, /* B+C+F+G+M */                                                      \
		[5] = 0x0869, /* A+D+F+G+N */                                                      \
		[6] = 0x047D, /* A+C+D+E+F+G+M */                                                  \
		[7] = 0x0007, /* A+B+C */                                                          \
		[8] = 0x047F, /* A+B+C+D+E+F+G+M */                                                \
		[9] = 0x046F, /* A+B+C+D+F+G+M */                                                  \
	};                                                                                         \
	/* Rotation swaps: A<->D, B<->E, C<->F, H<->N, J<->P, K<->Q; G,M fixed */                  \
	static const uint16_t digits_rotated_180[] = {                                             \
		[0] = 0x223F, /* 0: symmetric */                                                   \
		[1] = 0x0030, /* 1: B+C -> E+F */                                                  \
		[2] = 0x045B, /* 2: symmetric */                                                   \
		[3] = 0x0479, /* 3: A+B+C+D+G+M -> A+D+E+F+G+M */                                  \
		[4] = 0x0474, /* 4: B+C+F+G+M   -> C+E+F+G+M  */                                   \
		[5] = 0x00CD, /* 5: A+D+F+G+N   -> A+C+D+G+H  */                                   \
		[6] = 0x046F, /* 6 -> 9 pattern */                                                 \
		[7] = 0x0038, /* 7: A+B+C       -> D+E+F       */                                  \
		[8] = 0x047F, /* 8: symmetric */                                                   \
		[9] = 0x047D, /* 9 -> 6 pattern */                                                 \
	};                                                                                         \
	static const uint16_t letters_upper[] = {                                                  \
		['A' - 'A'] = 0x0477, /* E+F+G+A+B+C+M */                                          \
		['B' - 'A'] = 0x150F, /* A+B+C+D+J+M+P */                                          \
		['C' - 'A'] = 0x0039, /* A+D+E+F */                                                \
		['D' - 'A'] = 0x110F, /* A+B+C+D+J+P */                                            \
		['E' - 'A'] = 0x0079, /* A+D+E+F+G */                                              \
		['F' - 'A'] = 0x0071, /* A+E+F+G */                                                \
		['G' - 'A'] = 0x043D, /* A+C+D+E+F+M */                                            \
		['H' - 'A'] = 0x0476, /* B+C+E+F+G+M */                                            \
		['I' - 'A'] = 0x1109, /* A+D+J+P */                                                \
		['J' - 'A'] = 0x001E, /* B+C+D+E */                                                \
		['K' - 'A'] = 0x0A70, /* E+F+G+K+N */                                              \
		['L' - 'A'] = 0x0038, /* D+E+F */                                                  \
		['M' - 'A'] = 0x02B6, /* B+C+E+F+H+K */                                            \
		['N' - 'A'] = 0x08B6, /* B+C+E+F+H+N */                                            \
		['O' - 'A'] = 0x003F, /* A+B+C+D+E+F */                                            \
		['P' - 'A'] = 0x0473, /* A+B+E+F+G+M */                                            \
		['Q' - 'A'] = 0x083F, /* A+B+C+D+E+F+N */                                          \
		['R' - 'A'] = 0x0C73, /* A+B+E+F+G+M+N */                                          \
		['S' - 'A'] = 0x046D, /* A+C+D+F+G+M */                                            \
		['T' - 'A'] = 0x1101, /* A+J+P */                                                  \
		['U' - 'A'] = 0x003E, /* B+C+D+E+F */                                              \
		['V' - 'A'] = 0x2230, /* E+F+K+Q */                                                \
		['W' - 'A'] = 0x2836, /* B+C+E+F+N+Q */                                            \
		['X' - 'A'] = 0x2A80, /* H+K+N+Q */                                                \
		['Y' - 'A'] = 0x046E, /* B+C+D+F+G+M */                                            \
		['Z' - 'A'] = 0x2209, /* A+D+K+Q */                                                \
	};                                                                                         \
	static const uint16_t letters_upper_rotated_180[] = {                                      \
		['A' - 'A'] = 0x0477, ['B' - 'A'] = 0x150F, ['C' - 'A'] = 0x0039,                  \
		['D' - 'A'] = 0x110F, ['E' - 'A'] = 0x0079, ['F' - 'A'] = 0x0071,                  \
		['G' - 'A'] = 0x043D, ['H' - 'A'] = 0x0476, ['I' - 'A'] = 0x1109,                  \
		['J' - 'A'] = 0x001E, ['K' - 'A'] = 0x0A70, ['L' - 'A'] = 0x0038,                  \
		['M' - 'A'] = 0x02B6, ['N' - 'A'] = 0x08B6, ['O' - 'A'] = 0x003F,                  \
		['P' - 'A'] = 0x0473, ['Q' - 'A'] = 0x083F, ['R' - 'A'] = 0x0C73,                  \
		['S' - 'A'] = 0x046D, ['T' - 'A'] = 0x1101, ['U' - 'A'] = 0x003E,                  \
		['V' - 'A'] = 0x2230, ['W' - 'A'] = 0x2836, ['X' - 'A'] = 0x2A80,                  \
		['Y' - 'A'] = 0x046E, ['Z' - 'A'] = 0x2209,                                        \
	};                                                                                         \
	static const uint16_t letters_lower[] = {0};                                               \
	static const uint16_t letters_lower_rotated_180[] = {0};

/* Pass child node through; used to expand the single child of a DRV instance. */
#define _SLCD_PANEL_CHILD_NODE(child) child

#define SLCD_PANEL_NODE(n) DT_FOREACH_CHILD_STATUS_OKAY(DT_DRV_INST(n), _SLCD_PANEL_CHILD_NODE)

/* Declare optional property array, expands to nothing if property absent. */
#define SLCD_PANEL_OPTIONAL_ARRAY(n, prop)                                                         \
	COND_CODE_1(DT_NODE_HAS_PROP(SLCD_PANEL_NODE(n), prop),					\
		(static const uint8_t slcd_opt_##prop##_##n[] =					\
			DT_PROP(SLCD_PANEL_NODE(n), prop);), ())

/* Reference optional array, return NULL if property absent. */
#define SLCD_PANEL_OPTIONAL_REF(n, prop)                                                           \
	COND_CODE_1(DT_NODE_HAS_PROP(SLCD_PANEL_NODE(n), prop),					\
		(slcd_opt_##prop##_##n), (NULL))

/* Panel-level configurations, non-related to specific IP implementation. */
#define SLCD_PANEL_CONFIG(n)                                                                       \
	BUILD_ASSERT(DT_CHILD_NUM_STATUS_OKAY(DT_DRV_INST(n)) == 1,                                \
		     "One SLCD controller can have only one panel");                               \
	COND_CODE_1(IS_EQ(DT_PROP(SLCD_PANEL_NODE(n), segment_type), 7),			\
		    (SLCD_PANEL_CODING_7SEG), ())                       \
	COND_CODE_1(IS_EQ(DT_PROP(SLCD_PANEL_NODE(n), segment_type), 14),			\
		    (SLCD_PANEL_CODING_14SEG), ())                      \
	BUILD_ASSERT(IS_EQ(DT_PROP(SLCD_PANEL_NODE(n), segment_type), 7) ||                        \
			     IS_EQ(DT_PROP(SLCD_PANEL_NODE(n), segment_type), 14),                 \
		     "Unsupported segment-type (must be 7 or 14)");                                \
	static const uint8_t slcd_segment_pins_##n[] = DT_PROP(SLCD_PANEL_NODE(n), segment_pins);  \
	static const uint8_t slcd_segment_coms_##n[] = DT_PROP(SLCD_PANEL_NODE(n), segment_coms);  \
	SLCD_PANEL_OPTIONAL_ARRAY(n, indicator_pins)                                               \
	SLCD_PANEL_OPTIONAL_ARRAY(n, indicator_coms)                                               \
	SLCD_PANEL_OPTIONAL_ARRAY(n, col_indicators)                                               \
	SLCD_PANEL_OPTIONAL_ARRAY(n, upper_dot_indicators)                                         \
	SLCD_PANEL_OPTIONAL_ARRAY(n, lower_dot_indicators)                                         \
	static const struct auxdisplay_panel_config slcd_panel_config_##n = {                      \
		.capabilities =                                                                    \
			{                                                                          \
				.columns = DT_PROP(SLCD_PANEL_NODE(n), columns),                   \
				.rows = DT_PROP(SLCD_PANEL_NODE(n), rows),                         \
			},                                                                         \
		.rotated = DT_PROP(SLCD_PANEL_NODE(n), rotated),                                   \
		.segment_type = DT_PROP(SLCD_PANEL_NODE(n), segment_type),                         \
		.digits = digits,                                                                  \
		.digits_rotated_180 = digits_rotated_180,                                          \
		.letters_upper = (ARRAY_SIZE(letters_upper) == 26U) ? letters_upper : NULL,        \
		.letters_upper_rotated_180 = (ARRAY_SIZE(letters_upper_rotated_180) == 26U)        \
						     ? letters_upper_rotated_180                   \
						     : NULL,                                       \
		.letters_lower = (ARRAY_SIZE(letters_lower) == 26U) ? letters_lower : NULL,        \
		.letters_lower_rotated_180 = (ARRAY_SIZE(letters_lower_rotated_180) == 26U)        \
						     ? letters_lower_rotated_180                   \
						     : NULL,                                       \
		.segment_pins = slcd_segment_pins_##n,                                             \
		.segment_coms = slcd_segment_coms_##n,                                             \
		.num_indicators = DT_PROP(SLCD_PANEL_NODE(n), num_indicators),                     \
		.indicator_pins = SLCD_PANEL_OPTIONAL_REF(n, indicator_pins),                      \
		.indicator_coms = SLCD_PANEL_OPTIONAL_REF(n, indicator_coms),                      \
		.col_indicators = SLCD_PANEL_OPTIONAL_REF(n, col_indicators),                      \
		.upper_dot_indicators = SLCD_PANEL_OPTIONAL_REF(n, upper_dot_indicators),          \
		.lower_dot_indicators = SLCD_PANEL_OPTIONAL_REF(n, lower_dot_indicators),          \
	};

#endif /* __AUXDISPLAY_SLCD_CONFIG_H_ */
