/*
 * Copyright 2026 Renato Mauro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_AUXDISPLAY_AUXDISPLAY_SLCD_14SEG_H_
#define ZEPHYR_DRIVERS_AUXDISPLAY_AUXDISPLAY_SLCD_14SEG_H_

/**
 * @brief 14-segment digit coding tables.
 *
 * Standard 14-segment encoding (Bit 0=A, 1=B, 2=C, 3=D, 4=E, 5=F, 6=G, 7=H, 8=L, 9=M, 10=N, 11=O,
 * 12=P, 13=Q):
 *
 * Supported characters
 *
 * ASCII characters:
 *    - numbers:                0..9
 *    - uppercase letters:      A..Z
 *    - operators:              +  -  *  /
 *    - symbols:                Space (' ': 0x20)  _  (  )
 *    - indicators (pos 1..4):  . (dot)  : (double dot)
 *
 * Not ASCII indicators:
 *    - pos 1..4:               Triple dot (both dot and double dot)
 *    - pos 6:                  4 independent bars
 *
 * Custom characters (see sample auxdisplay_14seg):
 *    - unit prefixes:          d  c  m  u  n
 *    - other symbols:          Degree  Low ring  Full
 *
 * By default 14-segment display does not support lower-case letters.
 * User can override this by providing custom letter coding tables downstream.
 *
 *
 * GLASS LCD MAPPING
 * The LCD has six 14-segment digits with point/colon and 4 bars:
 *
 *    1       2       3       4       5       6
 *  -----   -----   -----   -----   -----   -----
 *  |\|/| o |\|/| o |\|/| o |\|/| o |\|/|   |\|/|   BAR3
 *  -- --   -- --   -- --   -- --   -- --   -- --   BAR2
 *  |/|\| o |/|\| o |/|\| o |/|\| o |/|\|   |/|\|   BAR1
 *  ----- * ----- * ----- * ----- * -----   -----   BAR0
 *
 *  LCD segment mapping:
 *
 *   Pos 1..6     Pos 1..4                 Pos 6
 *
 *  -----A-----
 *  |\   |   /|        _                  _______
 *  F H  J  K B   COL |_|           BAR3 |_______|  (same as pos 5 COL)
 *  |  \ | /  |                           _______
 *  --G-- --M--        _            BAR2 |_______|  (some as pos 5 DP)
 *  |  / | \  |   COL |_|                 _______
 *  E Q  P  N C                     BAR1 |_______|  (same as pos 6 COL)
 *  |/   |   \|        _                  _______
 *  -----D-----   DP  |_|           BAR0 |_______|  (some as pos 6 DP)
 *
 *
 *  -----0-----
 *  |\   |   /|
 *  5 7  8  9 1
 *  |  \ | /  |
 *  --6-- --10-
 *  |  / | \  |
 *  4 13 | 11 2
 *  |/  12   \|
 *  -----3-----
 *
 * Indicators indexes: 0..11
 *
 *  -----     -----     -----     -----     -----     -----
 *  |\|/|  o  |\|/|  o  |\|/|  o  |\|/|  o  |\|/|     |\|/|  11 BAR3
 *  -- --  1  -- --  3  -- --  5  -- --  7  -- --     -- --  10 BAR2
 *  |/|\|  o  |/|\|  o  |/|\|  o  |/|\|  o  |/|\|     |/|\|   9 BAR1
 *  -----  *  -----  *  -----  *  -----  *  -----     -----   8 BAR0
 *         0         2         4         6
 */

#define SLCD_PANEL_CODING_14SEG                                                                    \
	static const uint16_t digits[] = {                                                         \
		[0] = BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(4) | BIT(5) | BIT(9) | BIT(13),      \
		[1] = BIT(1) | BIT(2),                                                             \
		[2] = BIT(0) | BIT(1) | BIT(3) | BIT(4) | BIT(6) | BIT(10),                        \
		[3] = BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(6) | BIT(10),                        \
		[4] = BIT(1) | BIT(2) | BIT(5) | BIT(6) | BIT(10),                                 \
		[5] = BIT(0) | BIT(2) | BIT(3) | BIT(5) | BIT(6) | BIT(10),                        \
		[6] = BIT(0) | BIT(2) | BIT(3) | BIT(4) | BIT(5) | BIT(6) | BIT(10),               \
		[7] = BIT(0) | BIT(1) | BIT(2),                                                    \
		[8] = BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(4) | BIT(5) | BIT(6) | BIT(10),      \
		[9] = BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(5) | BIT(6) | BIT(10),               \
	};                                                                                         \
	static const uint16_t digits_rotated_180[] = {0};                                          \
	static const uint16_t letters_upper[] = {                                                  \
		['A' - 'A'] = BIT(0) | BIT(1) | BIT(2) | BIT(4) | BIT(5) | BIT(6) | BIT(10),       \
		['B' - 'A'] = BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(8) | BIT(10) | BIT(12),      \
		['C' - 'A'] = BIT(0) | BIT(3) | BIT(4) | BIT(5),                                   \
		['D' - 'A'] = BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(8) | BIT(12),                \
		['E' - 'A'] = BIT(0) | BIT(3) | BIT(4) | BIT(5) | BIT(6) /* | BIT(10) */,          \
		['F' - 'A'] = BIT(0) | BIT(4) | BIT(5) | BIT(6) /* | BIT(10) */,                   \
		['G' - 'A'] = BIT(0) | BIT(2) | BIT(3) | BIT(4) | BIT(5) | BIT(10),                \
		['H' - 'A'] = BIT(1) | BIT(2) | BIT(4) | BIT(5) | BIT(6) | BIT(10),                \
		['I' - 'A'] = BIT(0) | BIT(3) | BIT(8) | BIT(12),                                  \
		['J' - 'A'] = BIT(1) | BIT(2) | BIT(3) | BIT(4),                                   \
		['K' - 'A'] = BIT(4) | BIT(5) | BIT(6) | BIT(9) | BIT(11),                         \
		['L' - 'A'] = BIT(3) | BIT(4) | BIT(5),                                            \
		['M' - 'A'] = BIT(1) | BIT(2) | BIT(4) | BIT(5) | BIT(7) | BIT(9),                 \
		['N' - 'A'] = BIT(1) | BIT(2) | BIT(4) | BIT(5) | BIT(7) | BIT(11),                \
		['O' - 'A'] = BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(4) | BIT(5),                 \
		['P' - 'A'] = BIT(0) | BIT(1) | BIT(4) | BIT(5) | BIT(6) | BIT(10),                \
		['Q' - 'A'] = BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(4) | BIT(5) | BIT(11),       \
		['R' - 'A'] = BIT(0) | BIT(1) | BIT(4) | BIT(5) | BIT(6) | BIT(10) | BIT(11),      \
		['S' - 'A'] = BIT(0) | BIT(2) | BIT(3) | BIT(5) | BIT(6) | BIT(10),                \
		['T' - 'A'] = BIT(0) | BIT(8) | BIT(12),                                           \
		['U' - 'A'] = BIT(1) | BIT(2) | BIT(3) | BIT(4) | BIT(5),                          \
		['V' - 'A'] = BIT(4) | BIT(5) | BIT(9) | BIT(13),                                  \
		['W' - 'A'] = BIT(1) | BIT(2) | BIT(4) | BIT(5) | BIT(11) | BIT(13),               \
		['X' - 'A'] = BIT(7) | BIT(9) | BIT(11) | BIT(13),                                 \
		['Y' - 'A'] = BIT(7) | BIT(9) | BIT(12),                                           \
		['Z' - 'A'] = BIT(0) | BIT(3) | BIT(9) | BIT(13),                                  \
	};                                                                                         \
	static const uint16_t letters_upper_rotated_180[] = {0};                                   \
	static const uint16_t letters_lower[] = {0};                                               \
	static const uint16_t letters_lower_rotated_180[] = {0};

/* Letter Y could be more blocky using bits 1, 5, 6, 10 and 12 */

/* Pattern for ' ' character */
#define SLCD_C_SPACE_MAP ((uint16_t)0x0000)

/* Pattern for '_' character */
#define SLCD_C_UNDERSCORE_MAP ((uint16_t)BIT(3))

/* Pattern for '(' character */
#define SLCD_C_OPEN_PAR_MAP ((uint16_t)BIT(9) | BIT(11))

/* Pattern for ')' character */
#define SLCD_C_CLOSE_PAR_MAP ((uint16_t)BIT(7) | BIT(13))

/* Pattern for '*' character */
#define SLCD_C_STAR_MAP ((uint16_t)BIT(6) | BIT(7) | BIT(9) | BIT(10) | BIT(11) | BIT(13))

/* Pattern for '-' character */
#define SLCD_C_MINUS_MAP ((uint16_t)BIT(6) | BIT(10))

/* Pattern for '+' character */
#define SLCD_C_PLUS_MAP ((uint16_t)BIT(6) | BIT(8) | BIT(10) | BIT(12))

/* Pattern for '/' character */
#define SLCD_C_SLASH_MAP ((uint16_t)BIT(9) | BIT(13))

/* Convert an ascii char to the an LCD char pattern.
 * panel:     the LCD display panel.
 * one_char:  the char to display.
 */
uint16_t slcd_14seg_convert_ascii_char_to_14seg_pattern(const struct auxdisplay_panel_config *panel,
							uint8_t one_char);

#endif /* ZEPHYR_DRIVERS_AUXDISPLAY_AUXDISPLAY_SLCD_14SEG_H_ */
