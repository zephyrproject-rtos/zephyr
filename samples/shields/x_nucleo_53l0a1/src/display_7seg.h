#ifndef DISPLAY_7SEG_HEADER
#define DISPLAY_7SEG_HEADER

/*
 * Copyright (c) 2021 Titouan Christophe
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>

/*
 *    3
 *   ---
 * 1|   |5
 *   -2-
 * 0|   |6
 *   ---
 *    4
 */

#define CHAR_OFF        (0)

#define CHAR_0          (BIT(0) | BIT(1) | BIT(3) | BIT(4) | BIT(5) | BIT(6))
#define CHAR_1          (BIT(5) | BIT(6))
#define CHAR_2          (BIT(0) | BIT(2) | BIT(3) | BIT(4) | BIT(5))
#define CHAR_3          (BIT(2) | BIT(3) | BIT(4) | BIT(5) | BIT(6))
#define CHAR_4          (BIT(1) | BIT(2) | BIT(5) | BIT(6))
#define CHAR_5          (BIT(1) | BIT(2) | BIT(3) | BIT(4) | BIT(6))
#define CHAR_6          (BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(4) | BIT(6))
#define CHAR_7          (BIT(3) | BIT(5) | BIT(6))
#define CHAR_8          (BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(4) | BIT(5) | BIT(6))
#define CHAR_9          (BIT(1) | BIT(2) | BIT(3) | BIT(4) | BIT(5) | BIT(6))

#define CHAR_A          (BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(5) | BIT(6))
#define CHAR_C          (BIT(0) | BIT(1) | BIT(3) | BIT(4))
#define CHAR_E          (BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(4))
#define CHAR_F          (BIT(0) | BIT(1) | BIT(2) | BIT(3))
#define CHAR_H          (BIT(0) | BIT(1) | BIT(2) | BIT(5) | BIT(6))
#define CHAR_L          (BIT(0) | BIT(1) | BIT(4))
#define CHAR_P          (BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(5))
#define CHAR_U          (BIT(0) | BIT(1) | BIT(4) | BIT(5) | BIT(6))

#define CHAR_b          (BIT(0) | BIT(1) | BIT(2) | BIT(4) | BIT(6))
#define CHAR_d          (BIT(0) | BIT(2) | BIT(4) | BIT(5) | BIT(6))
#define CHAR_h          (BIT(0) | BIT(1) | BIT(2) | BIT(6))
#define CHAR_i          (BIT(6))
#define CHAR_o          (BIT(0) | BIT(2) | BIT(4) | BIT(6))
#define CHAR_r          (BIT(0) | BIT(2))
#define CHAR_t          (BIT(0) | BIT(1) | BIT(2) | BIT(4))
#define CHAR_u          (BIT(0) | BIT(4) | BIT(6))

#define CHAR_DASH       (BIT(2))
#define CHAR_OVERLINE   (BIT(3))
#define CHAR_UNDERSCORE (BIT(4))
#define CHAR_PIPE       (BIT(0) | BIT(1))

extern const uint8_t DISPLAY_OFF[4];
extern const uint8_t TEXT_Err[4];

/**
 * @brief      Display characters on the 4x7seg display
 * @param[in]  chars  The characters to be displayed
 * @return     0 on success, nonzero on error
 */
int display_chars(const uint8_t chars[4]);

/**
 * @brief      Display a number on the 4x7seg display
 * @param[in]  num   The number to be displayed
 * @param[in]  base  The base into which the number shall be displayed
 * @return     0 on success, nonzero on error
 */
int display_number(int num, unsigned int base);

#endif
