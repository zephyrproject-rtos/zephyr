/*
 * Copyright 2026 Renato Mauro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __AUXDISPLAY_SLCD_7SEG_H_
#define __AUXDISPLAY_SLCD_7SEG_H_

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

#endif /* __AUXDISPLAY_SLCD_7SEG_H_ */
