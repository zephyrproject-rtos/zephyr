/*
 * Copyright (c) 2026 Renato Mauro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_gh08172t

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/auxdisplay.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/logging/log.h>
#include <stm32_ll_bus.h>
#include <zephyr/sys/time_units.h>

/* This driver is for stm32l476g_discovery REVB or REVC only,
 * thus REVA is not supported.
 * The LCD display is GH08172T.
 */

LOG_MODULE_REGISTER(auxdisplay_gh08172t, CONFIG_AUXDISPLAY_LOG_LEVEL);

/* ST modules to refer for definitions:
 * - /deps/modules/hal/stm32/stm32cube/stm32l4xx/soc/stm32l4xx.h
 * - /deps/modules/hal/stm32/stm32cube/stm32l4xx/soc/stm32l476xx.h
 * - /deps/modules/hal/stm32/stm32cube/stm32l4xx/drivers/include/stm32l4xx_hal_lcd.h
 */

/* Supported characters
 *
 * Numbers:            0..9
 * Uppercase letters:  A..Z
 * Unit prefixes:      d  c  m  u  n
 * Operators:          +  -  *  /
 * Symbols:            Space (' ': 0x20)  %  _  (  )  Degree ('^': 0x5E)  Full ('#': 0x23)
 *
 * Points (pos 1..4):  . (dot)  : (double dot)  ; (triple dot)
 *
 * Bars (pos 6):         (0x00)..(0x0F)
 */

/* The percentage symbol is translated into three display positions.
 * It is made by a degree sign, a slash and a low ring.
 */
#define GH08172T_PERCENTAGE_SYMBOL_POSITIONS 3

/* The degree sign is passed as the ^ ASCII character */
#define GH08172T_ASCII_CHAR_DEGREE_SIGN 0x5E /* '^' */

/* All the segments in a position, but the dots.
 * This character is passed as the # ASCII character.
 */
#define GH08172T_ASCII_CHAR_FULL 0x23

#define GH08172T_ASCII_DOT        0x2E /* '.' */
#define GH08172T_ASCII_DOUBLE_DOT 0x3A /* ':' */
#define GH08172T_ASCII_TRIPLE_DOT 0x3B /* ';' */

/* LCD bars are handled as 4 bits, from 0 to 15.
 * Values from 0 to 9 are based on the '0' character.
 * Values from 10 to 15 are based on the 'A' character.
 */
enum auxdisplay_gh08172t_bar_value {
	GH08172T_BAR_0 = 0x00,
	GH08172T_BAR_OFF = GH08172T_BAR_0,
	GH08172T_BAR_EMPTY = GH08172T_BAR_0,
	GH08172T_BAR_0_PERC = GH08172T_BAR_0,
	GH08172T_BAR_1 = 0x01,
	GH08172T_BAR_1_QUARTER = GH08172T_BAR_1,
	GH08172T_BAR_25_PERC = GH08172T_BAR_1,
	GH08172T_BAR_2 = 0x02,
	GH08172T_BAR_3 = 0x03,
	GH08172T_BAR_HALF = GH08172T_BAR_3,
	GH08172T_BAR_50_PERC = GH08172T_BAR_3,
	GH08172T_BAR_4 = 0x04,
	GH08172T_BAR_5 = 0x05,
	GH08172T_BAR_6 = 0x06,
	GH08172T_BAR_7 = 0x07,
	GH08172T_BAR_3_QUARTERS = GH08172T_BAR_7,
	GH08172T_BAR_75_PERC = GH08172T_BAR_7,
	GH08172T_BAR_8 = 0x08,
	GH08172T_BAR_9 = 0x09,
	GH08172T_BAR_10 = 0x0A,
	GH08172T_BAR_11 = 0x0B,
	GH08172T_BAR_12 = 0x0C,
	GH08172T_BAR_13 = 0x0D,
	GH08172T_BAR_14 = 0x0E,
	GH08172T_BAR_15 = 0x0F,
	GH08172T_BAR_FULL = GH08172T_BAR_15,
	GH08172T_BAR_100_PERC = GH08172T_BAR_15,
	GH08172T_BAR_MAX = 0x10
};

/* LCD Glass digit position */
enum auxdisplay_gh08172t_digit_position {
	GH08172T_DIGIT_POSITION_1 = 0,
	GH08172T_DIGIT_POSITION_2 = 1,
	GH08172T_DIGIT_POSITION_3 = 2,
	GH08172T_DIGIT_POSITION_4 = 3,
	GH08172T_DIGIT_POSITION_5 = 4,
	GH08172T_DIGIT_POSITION_6 = 5,
	GH08172T_DIGIT_MAX_NUMBER = 6,
};

/* GPIO port to LCD pin to LCD crystal, sorted by GPIO port:
 *
 *    GPIO   |   LCD
 *  port pin |   pin
 *  ---------|--------
 *    A06    |  SEG23
 *    A07    |  SEG00
 *    A08    |  COM00
 *    A09    |  COM01
 *    A10    |  COM02
 *    A15    |  SEG10
 *    B00    |  SEG21
 *    B01    |  SEG02
 *    B04    |  SEG11
 *    B05    |  SEG12
 *    B09    |  COM03
 *    B12    |  SEG20
 *    B13    |  SEG03
 *    B14    |  SEG19
 *    B15    |  SEG04
 *    C03    |  VLCD
 *    C04    |  SEG22
 *    C05    |  SEG01
 *    C06    |  SEG14
 *    C07    |  SEG09
 *    C08    |  SEG13
 *    D08    |  SEG18
 *    D09    |  SEG05
 *    D10    |  SEG17
 *    D11    |  SEG06
 *    D12    |  SEG16
 *    D13    |  SEG07
 *    D14    |  SEG15
 *    D15    |  SEG08
 *
 *  GLASS LCD MAPPING
 *  The LCD has six 14-segment digits with point/colon and 4 bars:
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
 *  An LCD character coding is based on the following matrix:
 *
 *  COM         |   0     1     2     3   |
 *  ---------------------------------------
 *  SEG(n)      {   E ,   D ,   P ,   N   }
 *  SEG(n+1)    {   M ,   C ,   COL , DP  }
 *  SEG(23-n-1) {   B ,   A ,   K ,   J   }
 *  SEG(23-n)   {   G ,   F ,   Q ,   H   }
 *
 *  The character 'A' for example is:
 *  -----------------------
 *  LSB   { 1 , 0 , 0 , 0 }
 *        { 1 , 1 , 0 , 0 }
 *        { 1 , 1 , 0 , 0 }
 *  MSB   { 1 , 1 , 0 , 0 }
 *  -----------------------
 *   'A' =  F   E   0   0  hexadecimal
 *
 *  n is a positive even number, whose value is n = 2 * (Pos - 1):
 *
 *  POS          |  1 |  2 |  3 |  4 |  5 |  6 |
 *  --------------------------------------------
 *  n  2*(Pos-1) |  0 |  2 |  4 |  6 |  8 | 10 |
 *  --------------------------------------------
 *  SEG (n)      |  0 |  2 |  4 |  6 |  8 | 10 |
 *  SEG (n+1)    |  1 |  3 |  5 |  7 |  9 | 11 |
 *  SEG (23-n-1) | 22 | 20 | 18 | 16 | 14 | 12 |
 *  SEG (23-n)   | 23 | 21 | 19 | 17 | 15 | 13 |
 *
 *
 *  BARS
 *
 *  |      Graphic Bar    |   Segment   | Common |
 *  |      (position)     |  (MCU pin)  |  pin   |
 *  ----------------------------------------------
 *  | BAR3 (top)          | SEG9  (PC7) |  COM2  |
 *  | BAR2 (middle-upper) | SEG9  (PC7) |  COM3  |
 *  | BAR1 (middle-lower) | SEG11 (PB4) |  COM2  |
 *  | BAR0 (bottom)       | SEG11 (PB4) |  COM3  |
 *
 *  Position 0
 *
 *  COM              |   0     1     2     3   |
 *  --------------------------------------------
 *  SEG (n)        0 {   E ,   D ,   P ,   N   }
 *  SEG (n+1)      1 {   M ,   C ,   COL , DP  }
 *  SEG (23-n-1)  22 {   B ,   A ,   K ,   J   }
 *  SEG (23-n)    23 {   G ,   F ,   Q ,   H   }
 *
 *       ---------- A S22_C1 ---------
 *      |\             |             /|
 *      |  \           |           /  |
 *      |    \      J S22_C3     /    |
 *  F S23_C1   \       |       /   B S23_C0
 *      |    H S23_C3  |  K S23_C2    |               _
 *      |          \   |   /          |    COL S1_C2 |_|
 *      |            \ | /            |
 *       -- G S23_C0 -- -- M S1_C0  --
 *      |            / | \            |
 *      |          /   |   \          |               _
 *      |    Q S23_C2  | N S0_C3      |    COL S1_C2 |_|
 *  E S0_C0    /       |       \   C S1_C1
 *      |    /      P S0_C2      \    |
 *      |  /           |           \  |
 *      |/             |             \|               _
 *       ---------- D S0_C1 ----------     DP  S1_C3 |_|
 *
 *
 *  Summary for all positions
 *  GPIO port to LCD pin to LCD crystal, sorted by LCD pin:
 *
 *  | STM8L | STM32 | LCD   | LCD | COM3 | COM2 | COM1 | COM0 |
 *  | DISCO | L476G | Pin   | Pin |      |      |      |      |
 *  | GPIO  | DISCO | name  |     |      |      |      |      |
 *  |       | GPIO  |       |     |      |      |      |      |
 *  -----------------------------------------------------------
 *  |  PA7  |  A06  | SEG0  |   1 |  1N  |  1P  |  1D  |  1E  |
 *  |  PE0  |  C04  | SEG1  |   2 | 1DP  | 1COL |  1C  |  1M  |
 *  |  PE1  |  B00  | SEG2  |   3 |  2N  |  2P  |  2D  |  2E  |
 *  |  PE2  |  B12  | SEG3  |   4 | 2DP  | 2COL |  2C  |  2M  |
 *  |  PE3  |  B14  | SEG4  |   5 |  3N  |  3P  |  3D  |  3E  |
 *  |  PE4  |  D08  | SEG5  |   6 | 3DP  | 3COL |  3C  |  3M  |
 *  |  PE5  |  D10  | SEG6  |   7 |  4N  |  4P  |  4D  |  4E  |
 *  |  PD0  |  D12  | SEG7  |   8 | 4DP  | 4COL |  4C  |  4M  |
 *  |  PD2  |  D14  | SEG8  |   9 |  5N  |  5P  |  5D  |  5E  |
 *  |  PD3  |  C06  | SEG9  |  10 | BAR2 | BAR3 |  5C  |  5M  |
 *  |  PB0  |  C08  | SEG10 |  11 |  6N  |  6P  |  6D  |  6E  |
 *  |  PB1  |  B05  | SEG11 |  12 | BAR0 | BAR1 |  6C  |  6M  |
 *  |  PD1  |  A08  | COM3  |  13 | COM3 |      |      |      |
 *  |  PA6  |  A09  | COM2  |  14 |      | COM2 |      |      |
 *  |  PA5  |  A10  | COM1  |  15 |      |      | COM1 |      |
 *  |  PA4  |  B09  | COM0  |  16 |      |      |      | COM0 |
 *  |  PB2  |  B04  | SEG12 |  17 |  6J  |  6K  |  6A  |  6B  |
 *  |  PB3  |  A15  | SEG13 |  18 |  6H  |  6Q  |  6F  |  6G  |
 *  |  PB4  |  C07  | SEG14 |  19 |  5J  |  5K  |  5A  |  5B  |
 *  |  PB5  |  D15  | SEG15 |  20 |  5H  |  5Q  |  5F  |  5G  |
 *  |  PB6  |  D13  | SEG16 |  21 |  4J  |  4K  |  4A  |  4B  |
 *  |  PB7  |  D11  | SEG17 |  22 |  4H  |  4Q  |  4F  |  4G  |
 *  |  PD4  |  D09  | SEG18 |  23 |  3J  |  3K  |  3A  |  3B  |
 *  |  PD5  |  B15  | SEG19 |  24 |  3H  |  3Q  |  3F  |  3G  |
 *  |  PD6  |  B13  | SEG20 |  25 |  2J  |  2K  |  2A  |  2B  |
 *  |  PD7  |  B01  | SEG21 |  26 |  2H  |  2Q  |  2F  |  2G  |
 *  |  PC2  |  C05  | SEG22 |  27 |  1J  |  1K  |  1A  |  1B  |
 *  |  PC3  |  A07  | SEG23 |  28 |  1H  |  1Q  |  1F  |  1G  |
 *
 *  Truly it's more complicated, because each COM is able to drive
 *  more than 32 bits; so, if bits 0-31 are driven by COM0, bits
 *  32-63 (actually, in ST code, 38 for segments and 43 for shifts)
 *  are driven by a second register, named COM0_1. This means that
 *  a logical 63 bit set is handled via two 32 bit sets driven by
 *  two registers. This happens for positions 4 and 5 only.
 */

/* Constant table for cap characters 'A' --> 'Z' */
const uint16_t auxdisplay_gh08172t_cap_letter_map[26] = {
	/* A       B       C       D       E       F       G       H       I  */
	0xFE00, 0x6714, 0x1D00, 0x4714, 0x9D00, 0x9C00, 0x3F00, 0xFA00, 0x0014,
	/* J       K       L       M       N       O       P       Q       R  */
	0x5300, 0x9841, 0x1900, 0x5A48, 0x5A09, 0x5F00, 0xFC00, 0x5F01, 0xFC01,
	/* S       T       U       V       W       X       Y       Z  */
	0xAF00, 0x0414, 0x5b00, 0x18C0, 0x5A81, 0x00C9, 0x0058, 0x05C0};

/* Constant table for number '0' --> '9' */
const uint16_t auxdisplay_gh08172t_number_map[10] = {
	/* 0      1        2       3       4       5       6       7       8       9  */
	0x5FC0, 0x4200, 0xF500, 0x6700, 0xEA00, 0xAF00, 0xBF00, 0x04600, 0xFF00, 0xEF00};

/* Pattern for ' ' character */
#define GH08172T_C_SPACE_MAP ((uint16_t)0x0000)

/* Pattern for '_' character */
#define GH08172T_C_UNDERSCORE_MAP ((uint16_t)0x0100)

/* Pattern for '(' character */
#define GH08172T_C_OPEN_PAR_MAP ((uint16_t)0x0041)

/* Pattern for ')' character */
#define GH08172T_C_CLOSE_PAR_MAP ((uint16_t)0x0088)

/* Pattern for 'd' character */
#define GH08172T_C_D_MAP ((uint16_t)0xF300)

/* Pattern for 'c' character */
#define GH08172T_C_C_MAP ((uint16_t)0xB100)

/* Pattern for 'm' character */
#define GH08172T_C_M_MAP ((uint16_t)0xB210)

/* Pattern for 'u' character */
#define GH08172T_C_U_MAP ((uint16_t)0x1300)

/* Pattern for 'n' character */
#define GH08172T_C_N_MAP ((uint16_t)0x2210)

/* Pattern for '*' character */
#define GH08172T_C_STAR_MAP ((uint16_t)0xA0DD)

/* Pattern for '-' character */
#define GH08172T_C_MINUS_MAP ((uint16_t)0xA000)

/* Pattern for '+' character */
#define GH08172T_C_PLUS_MAP ((uint16_t)0xA014)

/* Pattern for '/' character */
#define GH08172T_C_SLASH_MAP ((uint16_t)0x00C0)

/* Pattern for degree character */
#define GH08172T_C_DEGREE_MAP ((uint16_t)0xEC00)

/* Pattern for small o character */
#define GH08172T_C_LOW_RING_MAP ((uint16_t)0xB300)

/* Pattern for all character segments but points and bars */
#define GH08172T_C_FULL_MAP ((uint16_t)0xFFDD)

/* LCD Digit COM & SEG definitions */
#define GH08172T_DIGIT1_COM0 GH08172T_COM0
#define GH08172T_DIGIT1_COM0_SEG_MASK                                                              \
	~(GH08172T_SEG0 | GH08172T_SEG1 | GH08172T_SEG22 | GH08172T_SEG23)
#define GH08172T_DIGIT1_COM1 GH08172T_COM1
#define GH08172T_DIGIT1_COM1_SEG_MASK                                                              \
	~(GH08172T_SEG0 | GH08172T_SEG1 | GH08172T_SEG22 | GH08172T_SEG23)
#define GH08172T_DIGIT1_COM2 GH08172T_COM2
#define GH08172T_DIGIT1_COM2_SEG_MASK                                                              \
	~(GH08172T_SEG0 | GH08172T_SEG1 | GH08172T_SEG22 | GH08172T_SEG23)
#define GH08172T_DIGIT1_COM3 GH08172T_COM3
#define GH08172T_DIGIT1_COM3_SEG_MASK                                                              \
	~(GH08172T_SEG0 | GH08172T_SEG1 | GH08172T_SEG22 | GH08172T_SEG23)

#define GH08172T_DIGIT2_COM0 GH08172T_COM0
#define GH08172T_DIGIT2_COM0_SEG_MASK                                                              \
	~(GH08172T_SEG2 | GH08172T_SEG3 | GH08172T_SEG20 | GH08172T_SEG21)
#define GH08172T_DIGIT2_COM1 GH08172T_COM1
#define GH08172T_DIGIT2_COM1_SEG_MASK                                                              \
	~(GH08172T_SEG2 | GH08172T_SEG3 | GH08172T_SEG20 | GH08172T_SEG21)
#define GH08172T_DIGIT2_COM2 GH08172T_COM2
#define GH08172T_DIGIT2_COM2_SEG_MASK                                                              \
	~(GH08172T_SEG2 | GH08172T_SEG3 | GH08172T_SEG20 | GH08172T_SEG21)
#define GH08172T_DIGIT2_COM3 GH08172T_COM3
#define GH08172T_DIGIT2_COM3_SEG_MASK                                                              \
	~(GH08172T_SEG2 | GH08172T_SEG3 | GH08172T_SEG20 | GH08172T_SEG21)

#define GH08172T_DIGIT3_COM0 GH08172T_COM0
#define GH08172T_DIGIT3_COM0_SEG_MASK                                                              \
	~(GH08172T_SEG4 | GH08172T_SEG5 | GH08172T_SEG18 | GH08172T_SEG19)
#define GH08172T_DIGIT3_COM1 GH08172T_COM1
#define GH08172T_DIGIT3_COM1_SEG_MASK                                                              \
	~(GH08172T_SEG4 | GH08172T_SEG5 | GH08172T_SEG18 | GH08172T_SEG19)
#define GH08172T_DIGIT3_COM2 GH08172T_COM2
#define GH08172T_DIGIT3_COM2_SEG_MASK                                                              \
	~(GH08172T_SEG4 | GH08172T_SEG5 | GH08172T_SEG18 | GH08172T_SEG19)
#define GH08172T_DIGIT3_COM3 GH08172T_COM3
#define GH08172T_DIGIT3_COM3_SEG_MASK                                                              \
	~(GH08172T_SEG4 | GH08172T_SEG5 | GH08172T_SEG18 | GH08172T_SEG19)

#define GH08172T_DIGIT4_COM0            GH08172T_COM0
#define GH08172T_DIGIT4_COM0_SEG_MASK   ~(GH08172T_SEG6 | GH08172T_SEG17)
#define GH08172T_DIGIT4_COM0_1          GH08172T_COM0_1
#define GH08172T_DIGIT4_COM0_1_SEG_MASK ~(GH08172T_SEG7 | GH08172T_SEG16)
#define GH08172T_DIGIT4_COM1            GH08172T_COM1
#define GH08172T_DIGIT4_COM1_SEG_MASK   ~(GH08172T_SEG6 | GH08172T_SEG17)
#define GH08172T_DIGIT4_COM1_1          GH08172T_COM1_1
#define GH08172T_DIGIT4_COM1_1_SEG_MASK ~(GH08172T_SEG7 | GH08172T_SEG16)
#define GH08172T_DIGIT4_COM2            GH08172T_COM2
#define GH08172T_DIGIT4_COM2_SEG_MASK   ~(GH08172T_SEG6 | GH08172T_SEG17)
#define GH08172T_DIGIT4_COM2_1          GH08172T_COM2_1
#define GH08172T_DIGIT4_COM2_1_SEG_MASK ~(GH08172T_SEG7 | GH08172T_SEG16)
#define GH08172T_DIGIT4_COM3            GH08172T_COM3
#define GH08172T_DIGIT4_COM3_SEG_MASK   ~(GH08172T_SEG6 | GH08172T_SEG17)
#define GH08172T_DIGIT4_COM3_1          GH08172T_COM3_1
#define GH08172T_DIGIT4_COM3_1_SEG_MASK ~(GH08172T_SEG7 | GH08172T_SEG16)

#define GH08172T_DIGIT5_COM0            GH08172T_COM0
#define GH08172T_DIGIT5_COM0_SEG_MASK   ~(GH08172T_SEG9 | GH08172T_SEG14)
#define GH08172T_DIGIT5_COM0_1          GH08172T_COM0_1
#define GH08172T_DIGIT5_COM0_1_SEG_MASK ~(GH08172T_SEG8 | GH08172T_SEG15)
#define GH08172T_DIGIT5_COM1            GH08172T_COM1
#define GH08172T_DIGIT5_COM1_SEG_MASK   ~(GH08172T_SEG9 | GH08172T_SEG14)
#define GH08172T_DIGIT5_COM1_1          GH08172T_COM1_1
#define GH08172T_DIGIT5_COM1_1_SEG_MASK ~(GH08172T_SEG8 | GH08172T_SEG15)
#define GH08172T_DIGIT5_COM2            GH08172T_COM2
#define GH08172T_DIGIT5_COM2_SEG_MASK   ~(GH08172T_SEG9 | GH08172T_SEG14)
#define GH08172T_DIGIT5_COM2_1          GH08172T_COM2_1
#define GH08172T_DIGIT5_COM2_1_SEG_MASK ~(GH08172T_SEG8 | GH08172T_SEG15)
#define GH08172T_DIGIT5_COM3            GH08172T_COM3
#define GH08172T_DIGIT5_COM3_SEG_MASK   ~(GH08172T_SEG9 | GH08172T_SEG14)
#define GH08172T_DIGIT5_COM3_1          GH08172T_COM3_1
#define GH08172T_DIGIT5_COM3_1_SEG_MASK ~(GH08172T_SEG8 | GH08172T_SEG15)

#define GH08172T_DIGIT6_COM0 GH08172T_COM0
#define GH08172T_DIGIT6_COM0_SEG_MASK                                                              \
	~(GH08172T_SEG10 | GH08172T_SEG11 | GH08172T_SEG12 | GH08172T_SEG13)
#define GH08172T_DIGIT6_COM1 GH08172T_COM1
#define GH08172T_DIGIT6_COM1_SEG_MASK                                                              \
	~(GH08172T_SEG10 | GH08172T_SEG11 | GH08172T_SEG12 | GH08172T_SEG13)
#define GH08172T_DIGIT6_COM2 GH08172T_COM2
#define GH08172T_DIGIT6_COM2_SEG_MASK                                                              \
	~(GH08172T_SEG10 | GH08172T_SEG11 | GH08172T_SEG12 | GH08172T_SEG13)
#define GH08172T_DIGIT6_COM3 GH08172T_COM3
#define GH08172T_DIGIT6_COM3_SEG_MASK                                                              \
	~(GH08172T_SEG10 | GH08172T_SEG11 | GH08172T_SEG12 | GH08172T_SEG13)

/* LCD Bar segments & coms definitions. */
#define GH08172T_BAR0_2_COM3   GH08172T_COM3
#define GH08172T_BAR1_3_COM2   GH08172T_COM2
#define GH08172T_BAR0_SEG      GH08172T_SEG11
#define GH08172T_BAR1_SEG      GH08172T_SEG11
#define GH08172T_BAR2_SEG      GH08172T_SEG9
#define GH08172T_BAR3_SEG      GH08172T_SEG9
#define GH08172T_BAR2_SEG_MASK ~(GH08172T_BAR2_SEG)
#define GH08172T_BAR3_SEG_MASK ~(GH08172T_BAR3_SEG)

/* LCD segments & coms redefinition.
 * LCD component segments & coms are not necessarily linked to MCU segmnents & coms output.
 */
#define GH08172T_COM0        MCU_GH08172T_COM0
#define GH08172T_COM0_1      MCU_GH08172T_COM0_1
#define GH08172T_COM1        MCU_GH08172T_COM1
#define GH08172T_COM1_1      MCU_GH08172T_COM1_1
#define GH08172T_COM2        MCU_GH08172T_COM2
#define GH08172T_COM2_1      MCU_GH08172T_COM2_1
#define GH08172T_COM3        MCU_GH08172T_COM3
#define GH08172T_COM3_1      MCU_GH08172T_COM3_1
#define GH08172T_SEG0        MCU_GH08172T_SEG4
#define GH08172T_SEG1        MCU_GH08172T_SEG23
#define GH08172T_SEG2        MCU_GH08172T_SEG6
#define GH08172T_SEG3        MCU_GH08172T_SEG13
#define GH08172T_SEG4        MCU_GH08172T_SEG15
#define GH08172T_SEG5        MCU_GH08172T_SEG29
#define GH08172T_SEG6        MCU_GH08172T_SEG31
#define GH08172T_SEG7        MCU_GH08172T_SEG33
#define GH08172T_SEG8        MCU_GH08172T_SEG35
#define GH08172T_SEG9        MCU_GH08172T_SEG25
#define GH08172T_SEG10       MCU_GH08172T_SEG17
#define GH08172T_SEG11       MCU_GH08172T_SEG8
#define GH08172T_SEG12       MCU_GH08172T_SEG9
#define GH08172T_SEG13       MCU_GH08172T_SEG26
#define GH08172T_SEG14       MCU_GH08172T_SEG24
#define GH08172T_SEG15       MCU_GH08172T_SEG34
#define GH08172T_SEG16       MCU_GH08172T_SEG32
#define GH08172T_SEG17       MCU_GH08172T_SEG30
#define GH08172T_SEG18       MCU_GH08172T_SEG28
#define GH08172T_SEG19       MCU_GH08172T_SEG14
#define GH08172T_SEG20       MCU_GH08172T_SEG12
#define GH08172T_SEG21       MCU_GH08172T_SEG5
#define GH08172T_SEG22       MCU_GH08172T_SEG22
#define GH08172T_SEG23       MCU_GH08172T_SEG3
#define GH08172T_SEG0_SHIFT  MCU_GH08172T_SEG4_SHIFT
#define GH08172T_SEG1_SHIFT  MCU_GH08172T_SEG23_SHIFT
#define GH08172T_SEG2_SHIFT  MCU_GH08172T_SEG6_SHIFT
#define GH08172T_SEG3_SHIFT  MCU_GH08172T_SEG13_SHIFT
#define GH08172T_SEG4_SHIFT  MCU_GH08172T_SEG15_SHIFT
#define GH08172T_SEG5_SHIFT  MCU_GH08172T_SEG29_SHIFT
#define GH08172T_SEG6_SHIFT  MCU_GH08172T_SEG31_SHIFT
#define GH08172T_SEG7_SHIFT  MCU_GH08172T_SEG33_SHIFT
#define GH08172T_SEG8_SHIFT  MCU_GH08172T_SEG35_SHIFT
#define GH08172T_SEG9_SHIFT  MCU_GH08172T_SEG25_SHIFT
#define GH08172T_SEG10_SHIFT MCU_GH08172T_SEG17_SHIFT
#define GH08172T_SEG11_SHIFT MCU_GH08172T_SEG8_SHIFT
#define GH08172T_SEG12_SHIFT MCU_GH08172T_SEG9_SHIFT
#define GH08172T_SEG13_SHIFT MCU_GH08172T_SEG26_SHIFT
#define GH08172T_SEG14_SHIFT MCU_GH08172T_SEG24_SHIFT
#define GH08172T_SEG15_SHIFT MCU_GH08172T_SEG34_SHIFT
#define GH08172T_SEG16_SHIFT MCU_GH08172T_SEG32_SHIFT
#define GH08172T_SEG17_SHIFT MCU_GH08172T_SEG30_SHIFT
#define GH08172T_SEG18_SHIFT MCU_GH08172T_SEG28_SHIFT
#define GH08172T_SEG19_SHIFT MCU_GH08172T_SEG14_SHIFT
#define GH08172T_SEG20_SHIFT MCU_GH08172T_SEG12_SHIFT
#define GH08172T_SEG21_SHIFT MCU_GH08172T_SEG5_SHIFT
#define GH08172T_SEG22_SHIFT MCU_GH08172T_SEG22_SHIFT
#define GH08172T_SEG23_SHIFT MCU_GH08172T_SEG3_SHIFT

/* STM32 LCD segments & coms definitions. */
#define MCU_GH08172T_COM0        GH08172T_RAM_REGISTER0
#define MCU_GH08172T_COM0_1      GH08172T_RAM_REGISTER1
#define MCU_GH08172T_COM1        GH08172T_RAM_REGISTER2
#define MCU_GH08172T_COM1_1      GH08172T_RAM_REGISTER3
#define MCU_GH08172T_COM2        GH08172T_RAM_REGISTER4
#define MCU_GH08172T_COM2_1      GH08172T_RAM_REGISTER5
#define MCU_GH08172T_COM3        GH08172T_RAM_REGISTER6
#define MCU_GH08172T_COM3_1      GH08172T_RAM_REGISTER7
#define MCU_GH08172T_COM4        GH08172T_RAM_REGISTER8
#define MCU_GH08172T_COM4_1      GH08172T_RAM_REGISTER9
#define MCU_GH08172T_COM5        GH08172T_RAM_REGISTER10
#define MCU_GH08172T_COM5_1      GH08172T_RAM_REGISTER11
#define MCU_GH08172T_COM6        GH08172T_RAM_REGISTER12
#define MCU_GH08172T_COM6_1      GH08172T_RAM_REGISTER13
#define MCU_GH08172T_COM7        GH08172T_RAM_REGISTER14
#define MCU_GH08172T_COM7_1      GH08172T_RAM_REGISTER15
#define MCU_GH08172T_SEG0        (1U << MCU_GH08172T_SEG0_SHIFT)
#define MCU_GH08172T_SEG1        (1U << MCU_GH08172T_SEG1_SHIFT)
#define MCU_GH08172T_SEG2        (1U << MCU_GH08172T_SEG2_SHIFT)
#define MCU_GH08172T_SEG3        (1U << MCU_GH08172T_SEG3_SHIFT)
#define MCU_GH08172T_SEG4        (1U << MCU_GH08172T_SEG4_SHIFT)
#define MCU_GH08172T_SEG5        (1U << MCU_GH08172T_SEG5_SHIFT)
#define MCU_GH08172T_SEG6        (1U << MCU_GH08172T_SEG6_SHIFT)
#define MCU_GH08172T_SEG7        (1U << MCU_GH08172T_SEG7_SHIFT)
#define MCU_GH08172T_SEG8        (1U << MCU_GH08172T_SEG8_SHIFT)
#define MCU_GH08172T_SEG9        (1U << MCU_GH08172T_SEG9_SHIFT)
#define MCU_GH08172T_SEG10       (1U << MCU_GH08172T_SEG10_SHIFT)
#define MCU_GH08172T_SEG11       (1U << MCU_GH08172T_SEG11_SHIFT)
#define MCU_GH08172T_SEG12       (1U << MCU_GH08172T_SEG12_SHIFT)
#define MCU_GH08172T_SEG13       (1U << MCU_GH08172T_SEG13_SHIFT)
#define MCU_GH08172T_SEG14       (1U << MCU_GH08172T_SEG14_SHIFT)
#define MCU_GH08172T_SEG15       (1U << MCU_GH08172T_SEG15_SHIFT)
#define MCU_GH08172T_SEG16       (1U << MCU_GH08172T_SEG16_SHIFT)
#define MCU_GH08172T_SEG17       (1U << MCU_GH08172T_SEG17_SHIFT)
#define MCU_GH08172T_SEG18       (1U << MCU_GH08172T_SEG18_SHIFT)
#define MCU_GH08172T_SEG19       (1U << MCU_GH08172T_SEG19_SHIFT)
#define MCU_GH08172T_SEG20       (1U << MCU_GH08172T_SEG20_SHIFT)
#define MCU_GH08172T_SEG21       (1U << MCU_GH08172T_SEG21_SHIFT)
#define MCU_GH08172T_SEG22       (1U << MCU_GH08172T_SEG22_SHIFT)
#define MCU_GH08172T_SEG23       (1U << MCU_GH08172T_SEG23_SHIFT)
#define MCU_GH08172T_SEG24       (1U << MCU_GH08172T_SEG24_SHIFT)
#define MCU_GH08172T_SEG25       (1U << MCU_GH08172T_SEG25_SHIFT)
#define MCU_GH08172T_SEG26       (1U << MCU_GH08172T_SEG26_SHIFT)
#define MCU_GH08172T_SEG27       (1U << MCU_GH08172T_SEG27_SHIFT)
#define MCU_GH08172T_SEG28       (1U << MCU_GH08172T_SEG28_SHIFT)
#define MCU_GH08172T_SEG29       (1U << MCU_GH08172T_SEG29_SHIFT)
#define MCU_GH08172T_SEG30       (1U << MCU_GH08172T_SEG30_SHIFT)
#define MCU_GH08172T_SEG31       (1U << MCU_GH08172T_SEG31_SHIFT)
#define MCU_GH08172T_SEG32       (1U << MCU_GH08172T_SEG32_SHIFT)
#define MCU_GH08172T_SEG33       (1U << MCU_GH08172T_SEG33_SHIFT)
#define MCU_GH08172T_SEG34       (1U << MCU_GH08172T_SEG34_SHIFT)
#define MCU_GH08172T_SEG35       (1U << MCU_GH08172T_SEG35_SHIFT)
#define MCU_GH08172T_SEG36       (1U << MCU_GH08172T_SEG36_SHIFT)
#define MCU_GH08172T_SEG37       (1U << MCU_GH08172T_SEG37_SHIFT)
#define MCU_GH08172T_SEG38       (1U << MCU_GH08172T_SEG38_SHIFT)
#define MCU_GH08172T_SEG0_SHIFT  0
#define MCU_GH08172T_SEG1_SHIFT  1
#define MCU_GH08172T_SEG2_SHIFT  2
#define MCU_GH08172T_SEG3_SHIFT  3
#define MCU_GH08172T_SEG4_SHIFT  4
#define MCU_GH08172T_SEG5_SHIFT  5
#define MCU_GH08172T_SEG6_SHIFT  6
#define MCU_GH08172T_SEG7_SHIFT  7
#define MCU_GH08172T_SEG8_SHIFT  8
#define MCU_GH08172T_SEG9_SHIFT  9
#define MCU_GH08172T_SEG10_SHIFT 10
#define MCU_GH08172T_SEG11_SHIFT 11
#define MCU_GH08172T_SEG12_SHIFT 12
#define MCU_GH08172T_SEG13_SHIFT 13
#define MCU_GH08172T_SEG14_SHIFT 14
#define MCU_GH08172T_SEG15_SHIFT 15
#define MCU_GH08172T_SEG16_SHIFT 16
#define MCU_GH08172T_SEG17_SHIFT 17
#define MCU_GH08172T_SEG18_SHIFT 18
#define MCU_GH08172T_SEG19_SHIFT 19
#define MCU_GH08172T_SEG20_SHIFT 20
#define MCU_GH08172T_SEG21_SHIFT 21
#define MCU_GH08172T_SEG22_SHIFT 22
#define MCU_GH08172T_SEG23_SHIFT 23
#define MCU_GH08172T_SEG24_SHIFT 24
#define MCU_GH08172T_SEG25_SHIFT 25
#define MCU_GH08172T_SEG26_SHIFT 26
#define MCU_GH08172T_SEG27_SHIFT 27
#define MCU_GH08172T_SEG28_SHIFT 28
#define MCU_GH08172T_SEG29_SHIFT 29
#define MCU_GH08172T_SEG30_SHIFT 30
#define MCU_GH08172T_SEG31_SHIFT 31
#define MCU_GH08172T_SEG32_SHIFT 0
#define MCU_GH08172T_SEG33_SHIFT 1
#define MCU_GH08172T_SEG34_SHIFT 2
#define MCU_GH08172T_SEG35_SHIFT 3
#define MCU_GH08172T_SEG36_SHIFT 4
#define MCU_GH08172T_SEG37_SHIFT 5
#define MCU_GH08172T_SEG38_SHIFT 6
#define MCU_GH08172T_SEG39_SHIFT 7
#define MCU_GH08172T_SEG40_SHIFT 8
#define MCU_GH08172T_SEG41_SHIFT 9
#define MCU_GH08172T_SEG42_SHIFT 10
#define MCU_GH08172T_SEG43_SHIFT 11

#define GH08172T_RAM_REGISTER0  (0x00000000U) /* LCD RAM Register 0  */
#define GH08172T_RAM_REGISTER1  (0x00000001U) /* LCD RAM Register 1  */
#define GH08172T_RAM_REGISTER2  (0x00000002U) /* LCD RAM Register 2  */
#define GH08172T_RAM_REGISTER3  (0x00000003U) /* LCD RAM Register 3  */
#define GH08172T_RAM_REGISTER4  (0x00000004U) /* LCD RAM Register 4  */
#define GH08172T_RAM_REGISTER5  (0x00000005U) /* LCD RAM Register 5  */
#define GH08172T_RAM_REGISTER6  (0x00000006U) /* LCD RAM Register 6  */
#define GH08172T_RAM_REGISTER7  (0x00000007U) /* LCD RAM Register 7  */
#define GH08172T_RAM_REGISTER8  (0x00000008U) /* LCD RAM Register 8  */
#define GH08172T_RAM_REGISTER9  (0x00000009U) /* LCD RAM Register 9  */
#define GH08172T_RAM_REGISTER10 (0x0000000AU) /* LCD RAM Register 10 */
#define GH08172T_RAM_REGISTER11 (0x0000000BU) /* LCD RAM Register 11 */
#define GH08172T_RAM_REGISTER12 (0x0000000CU) /* LCD RAM Register 12 */
#define GH08172T_RAM_REGISTER13 (0x0000000DU) /* LCD RAM Register 13 */
#define GH08172T_RAM_REGISTER14 (0x0000000EU) /* LCD RAM Register 14 */
#define GH08172T_RAM_REGISTER15 (0x0000000FU) /* LCD RAM Register 15 */

#define GH08172T_CR_OFFSET     0x00 /* LCD control register          */
#define GH08172T_FCR_OFFSET    0x04 /* LCD frame control register    */
#define GH08172T_SR_OFFSET     0x08 /* LCD status register           */
#define GH08172T_CLR_OFFSET    0x0C /* LCD clear register            */
#define GH08172T_RAM_OFFSET    0x14 /* LCD display memory, 0x14-0x50 */
#define GH08172T_RAM_REG_COUNT (GH08172T_RAM_REGISTER15 + 1)

#define GH08172T_CR_LCDEN_POS    (0U)
#define GH08172T_CR_LCDEN_MASK   (0x1UL << GH08172T_CR_LCDEN_POS)
#define GH08172T_CR_VSEL_POS     (1U)
#define GH08172T_CR_VSEL_MASK    (0x1UL << GH08172T_CR_VSEL_POS)
#define GH08172T_CR_DUTY_0_POS   (2U)
#define GH08172T_CR_DUTY_MASK    (0x7UL << GH08172T_CR_DUTY_0_POS)
#define GH08172T_CR_DUTY_0_MASK  (0x1UL << GH08172T_CR_DUTY_0_POS)
#define GH08172T_CR_DUTY_1_MASK  (0x2UL << GH08172T_CR_DUTY_0_POS)
#define GH08172T_CR_DUTY_2_MASK  (0x4UL << GH08172T_CR_DUTY_0_POS)
#define GH08172T_CR_BIAS_1_POS   (5U)
#define GH08172T_CR_BIAS_MASK    (0x3UL << GH08172T_CR_BIAS_1_POS)
#define GH08172T_CR_BIAS_0_MASK  (0x1UL << GH08172T_CR_BIAS_1_POS)
#define GH08172T_CR_BIAS_1_MASK  (0x2UL << GH08172T_CR_BIAS_1_POS)
#define GH08172T_CR_MUX_SEG_POS  (7U)
#define GH08172T_CR_MUX_SEG_MASK (0x1UL << GH08172T_CR_MUX_SEG_POS)
#define GH08172T_CR_BUFEN_POS    (8U)

#define GH08172T_FCR_HIGH_DRIVE_POS    (0U)
#define GH08172T_FCR_HIGH_DRIVE_MASK   (0x1UL << GH08172T_FCR_HIGH_DRIVE_POS)
#define GH08172T_FCR_SOFIE_POS         (1U)
#define GH08172T_FCR_UDDIE_POS         (3U)
#define GH08172T_FCR_PON_POS           (4U)
#define GH08172T_FCR_PON_MASK          (0x7UL << GH08172T_FCR_PON_POS)
#define GH08172T_FCR_PON_0_MASK        (0x1UL << GH08172T_FCR_PON_POS)
#define GH08172T_FCR_PON_1_MASK        (0x2UL << GH08172T_FCR_PON_POS)
#define GH08172T_FCR_PON_2_MASK        (0x4UL << GH08172T_FCR_PON_POS)
#define GH08172T_FCR_DEAD_POS          (7U)
#define GH08172T_FCR_DEAD_MASK         (0x7UL << GH08172T_FCR_DEAD_POS)
#define GH08172T_FCR_DEAD_0_MASK       (0x1UL << GH08172T_FCR_DEAD_POS)
#define GH08172T_FCR_DEAD_1_MASK       (0x2UL << GH08172T_FCR_DEAD_POS)
#define GH08172T_FCR_DEAD_2_MASK       (0x4UL << GH08172T_FCR_DEAD_POS)
#define GH08172T_FCR_CC_POS            (10U)
#define GH08172T_FCR_CC_MASK           (0x7UL << GH08172T_FCR_CC_POS)
#define GH08172T_FCR_CC_0_MASK         (0x1UL << GH08172T_FCR_CC_POS)
#define GH08172T_FCR_CC_1_MASK         (0x2UL << GH08172T_FCR_CC_POS)
#define GH08172T_FCR_CC_2_MASK         (0x4UL << GH08172T_FCR_CC_POS)
#define GH08172T_FCR_BLINK_FREQ_POS    (13U)
#define GH08172T_FCR_BLINK_FREQ_MASK   (0x7UL << GH08172T_FCR_BLINK_FREQ_POS)
#define GH08172T_FCR_BLINK_FREQ_0_MASK (0x1UL << GH08172T_FCR_BLINK_FREQ_POS)
#define GH08172T_FCR_BLINK_FREQ_1_MASK (0x2UL << GH08172T_FCR_BLINK_FREQ_POS)
#define GH08172T_FCR_BLINK_FREQ_2_MASK (0x4UL << GH08172T_FCR_BLINK_FREQ_POS)
#define GH08172T_FCR_BLINK_POS         (16U)
#define GH08172T_FCR_BLINK_MASK        (0x3UL << GH08172T_FCR_BLINK_POS)
#define GH08172T_FCR_DIV_POS           (18U)
#define GH08172T_FCR_DIV_MASK          (0xFUL << GH08172T_FCR_DIV_POS)
#define GH08172T_FCR_PS_POS            (22U)
#define GH08172T_FCR_PS_MASK           (0xFUL << GH08172T_FCR_PS_POS)

#define GH08172T_SR_ENS_POS    (0U)
#define GH08172T_SR_ENS_MASK   (0x1UL << GH08172T_SR_ENS_POS)
#define GH08172T_SR_SOF_POS    (1U)
#define GH08172T_SR_UDR_POS    (2U)
#define GH08172T_SR_UDR_MASK   (0x1UL << GH08172T_SR_UDR_POS)
#define GH08172T_SR_UDD_POS    (3U)
#define GH08172T_SR_UDD_MASK   (0x1UL << GH08172T_SR_UDD_POS)
#define GH08172T_SR_RDY_POS    (4U)
#define GH08172T_SR_RDY_MASK   (0x1UL << GH08172T_SR_RDY_POS)
#define GH08172T_SR_FCRSR_POS  (5U)
#define GH08172T_SR_FCRSR_MASK (0x1UL << GH08172T_SR_FCRSR_POS)

#define GH08172T_CLR_SOFC_POS  (1U)
#define GH08172T_CLR_SOFC_MASK (0x1UL << GH08172T_CLR_SOFC_POS)
#define GH08172T_CLR_UDDC_POS  (3U)
#define GH08172T_CLR_UDDC_MASK (0x1UL << GH08172T_CLR_UDDC_POS)

#define GH08172T_PRESCALER_1     (0x00000000U) /* CLKPS = LCDCLK        */
#define GH08172T_PRESCALER_2     (0x00400000U) /* CLKPS = LCDCLK/2      */
#define GH08172T_PRESCALER_4     (0x00800000U) /* CLKPS = LCDCLK/4      */
#define GH08172T_PRESCALER_8     (0x00C00000U) /* CLKPS = LCDCLK/8      */
#define GH08172T_PRESCALER_16    (0x01000000U) /* CLKPS = LCDCLK/16     */
#define GH08172T_PRESCALER_32    (0x01400000U) /* CLKPS = LCDCLK/32     */
#define GH08172T_PRESCALER_64    (0x01800000U) /* CLKPS = LCDCLK/64     */
#define GH08172T_PRESCALER_128   (0x01C00000U) /* CLKPS = LCDCLK/128    */
#define GH08172T_PRESCALER_256   (0x02000000U) /* CLKPS = LCDCLK/256    */
#define GH08172T_PRESCALER_512   (0x02400000U) /* CLKPS = LCDCLK/512    */
#define GH08172T_PRESCALER_1024  (0x02800000U) /* CLKPS = LCDCLK/1024   */
#define GH08172T_PRESCALER_2048  (0x02C00000U) /* CLKPS = LCDCLK/2048   */
#define GH08172T_PRESCALER_4096  (0x03000000U) /* CLKPS = LCDCLK/4096   */
#define GH08172T_PRESCALER_8192  (0x03400000U) /* CLKPS = LCDCLK/8192   */
#define GH08172T_PRESCALER_16384 (0x03800000U) /* CLKPS = LCDCLK/16384  */
#define GH08172T_PRESCALER_32768 (0x03C00000U) /* CLKPS = LCDCLK/32768  */

#define GH08172T_DIVIDER_16 (0x00000000U) /* LCD frequency = CLKPS/16 */
#define GH08172T_DIVIDER_17 (0x00040000U) /* LCD frequency = CLKPS/17 */
#define GH08172T_DIVIDER_18 (0x00080000U) /* LCD frequency = CLKPS/18 */
#define GH08172T_DIVIDER_19 (0x000C0000U) /* LCD frequency = CLKPS/19 */
#define GH08172T_DIVIDER_20 (0x00100000U) /* LCD frequency = CLKPS/20 */
#define GH08172T_DIVIDER_21 (0x00140000U) /* LCD frequency = CLKPS/21 */
#define GH08172T_DIVIDER_22 (0x00180000U) /* LCD frequency = CLKPS/22 */
#define GH08172T_DIVIDER_23 (0x001C0000U) /* LCD frequency = CLKPS/23 */
#define GH08172T_DIVIDER_24 (0x00200000U) /* LCD frequency = CLKPS/24 */
#define GH08172T_DIVIDER_25 (0x00240000U) /* LCD frequency = CLKPS/25 */
#define GH08172T_DIVIDER_26 (0x00280000U) /* LCD frequency = CLKPS/26 */
#define GH08172T_DIVIDER_27 (0x002C0000U) /* LCD frequency = CLKPS/27 */
#define GH08172T_DIVIDER_28 (0x00300000U) /* LCD frequency = CLKPS/28 */
#define GH08172T_DIVIDER_29 (0x00340000U) /* LCD frequency = CLKPS/29 */
#define GH08172T_DIVIDER_30 (0x00380000U) /* LCD frequency = CLKPS/30 */
#define GH08172T_DIVIDER_31 (0x003C0000U) /* LCD frequency = CLKPS/31 */

#define GH08172T_DUTY_STATIC (0x00000000U)                                         /* Static duty */
#define GH08172T_DUTY_1_2    (GH08172T_CR_DUTY_0_MASK)                             /* 1/2 duty    */
#define GH08172T_DUTY_1_3    (GH08172T_CR_DUTY_1_MASK)                             /* 1/3 duty    */
#define GH08172T_DUTY_1_4    ((GH08172T_CR_DUTY_1_MASK | GH08172T_CR_DUTY_0_MASK)) /* 1/4 duty    */
#define GH08172T_DUTY_1_8    (GH08172T_CR_DUTY_2_MASK)                             /* 1/8 duty    */

#define GH08172T_BIAS_1_4 (0x00000000U)           /* 1/4 Bias */
#define GH08172T_BIAS_1_2 GH08172T_CR_BIAS_0_MASK /* 1/2 Bias */
#define GH08172T_BIAS_1_3 GH08172T_CR_BIAS_1_MASK /* 1/3 Bias */

#define GH08172T_CR_VSEL_INTERNAL (0x00000000U)
#define GH08172T_CR_VSEL_EXTERNAL GH08172T_CR_VSEL

#define GH08172T_PULSEONDURATION_0 (0x00000000U)
#define GH08172T_PULSEONDURATION_1 (GH08172T_FCR_PON_0_MASK)
#define GH08172T_PULSEONDURATION_2 (GH08172T_FCR_PON_1_MASK)
#define GH08172T_PULSEONDURATION_3 (GH08172T_FCR_PON_1_MASK | GH08172T_FCR_PON_0_MASK)
#define GH08172T_PULSEONDURATION_4 (GH08172T_FCR_PON_2_MASK)
#define GH08172T_PULSEONDURATION_5 (GH08172T_FCR_PON_2_MASK | GH08172T_FCR_PON_0_MASK)
#define GH08172T_PULSEONDURATION_6 (GH08172T_FCR_PON_2_MASK | GH08172T_FCR_PON_1_MASK)
#define GH08172T_PULSEONDURATION_7 (GH08172T_FCR_PON_MASK)

#define GH08172T_DEADTIME_0 (0x00000000U)                                         /* No phases */
#define GH08172T_DEADTIME_1 (GH08172T_FCR_DEAD_0_MASK)                            /* 1 Phase */
#define GH08172T_DEADTIME_2 (GH08172T_FCR_DEAD_1_MASK)                            /* 2 Phase */
#define GH08172T_DEADTIME_3 (GH08172T_FCR_DEAD_1_MASK | GH08172T_FCR_DEAD_0_MASK) /* 3 Phase */
#define GH08172T_DEADTIME_4 (GH08172T_FCR_DEAD_2_MASK)                            /* 4 Phase */
#define GH08172T_DEADTIME_5 (GH08172T_FCR_DEAD_2_MASK | GH08172T_FCR_DEAD_0_MASK) /* 5 Phase */
#define GH08172T_DEADTIME_6 (GH08172T_FCR_DEAD_2_MASK | GH08172T_FCR_DEAD_1_MASK) /* 6 Phase */
#define GH08172T_DEADTIME_7 (GH08172T_FCR_DEAD_MASK)                              /* 7 Phase */

#define GH08172T_BLINK_OFF           (0x00000000U)
#define GH08172T_BLINK_SEG0_COM0     (GH08172T_FCR_BLINK_0) /* 1 pixel */
#define GH08172T_BLINK_SEG0_ALLCOM   (GH08172T_FCR_BLINK_1) /* up to 8 pixels as per duty) */
#define GH08172T_BLINK_ALLSEG_ALLCOM (GH08172T_FCR_BLINK)   /* all pixels */

#define GH08172T_BLINK_FREQ_DIV8  (0x00000000U)                    /* fLCD/8    */
#define GH08172T_BLINK_FREQ_DIV16 (GH08172T_FCR_BLINK_FREQ_0_MASK) /* fLCD/16   */
#define GH08172T_BLINK_FREQ_DIV32 (GH08172T_FCR_BLINK_FREQ_1_MASK) /* fLCD/32   */
#define GH08172T_BLINKFREQUENCY_DIV64                                                              \
	(GH08172T_FCR_BLINK_FREQ_1_MASK | GH08172T_FCR_BLINK_FREQ_0_MASK) /* fLCD/64   */
#define GH08172T_BLINKFREQUENCY_DIV128 (GH08172T_FCR_BLINK_FREQ_2_MASK)   /* fLCD/128  */
#define GH08172T_BLINKFREQUENCY_DIV256                                                             \
	(GH08172T_FCR_BLINK_FREQ_2_MASK | GH08172T_FCR_BLINK_FREQ_0_MASK) /* fLCD/256  */
#define GH08172T_BLINK_FREQ_DIV512                                                                 \
	(GH08172T_FCR_BLINK_FREQ_2_MASK | GH08172T_FCR_BLINK_FREQ_1_MASK) /* fLCD/512  */
#define GH08172T_BLINK_FREQ_DIV1024 (GH08172T_FCR_BLINK_FREQ_MASK)        /* fLCD/1024 */

#define GH08172T_CONTRASTLEVEL_0 (0x00000000U)                                     /* 2.60V */
#define GH08172T_CONTRASTLEVEL_1 (GH08172T_FCR_CC_0_MASK)                          /* 2.73V */
#define GH08172T_CONTRASTLEVEL_2 (GH08172T_FCR_CC_1_MASK)                          /* 2.86V */
#define GH08172T_CONTRASTLEVEL_3 (GH08172T_FCR_CC_1_MASK | GH08172T_FCR_CC_0_MASK) /* 2.99V */
#define GH08172T_CONTRASTLEVEL_4 (GH08172T_FCR_CC_2_MASK)                          /* 3.12V */
#define GH08172T_CONTRASTLEVEL_5 (GH08172T_FCR_CC_2_MASK | GH08172T_FCR_CC_0_MASK) /* 3.26V */
#define GH08172T_CONTRASTLEVEL_6 (GH08172T_FCR_CC_2_MASK | GH08172T_FCR_CC_1_MASK) /* 3.40V */
#define GH08172T_CONTRASTLEVEL_7 (GH08172T_FCR_CC_MASK)                            /* 3.55V */

#define GH08172T_FCR_HIGH_DRIVE_DISABLE ((uint32_t)0x00000000)
#define GH08172T_FCR_HIGH_DRIVE_ENABLE  (GH08172T_FCR_HIGH_DRIVE_MASK)

#define GH08172T_CR_MUX_SEG_DISABLE (0x00000000U)
#define GH08172T_CR_MUX_SEG_ENABLE  (GH08172T_CR_MUX_SEG_MASK) /* SEG[31:28] muxed SEG[43:40] */

/* Define for scrolling sentences*/
#define GH08172T_SCROLL_SPEED_HIGH   150
#define GH08172T_SCROLL_SPEED_MEDIUM 300
#define GH08172T_SCROLL_SPEED_LOW    450

#define GH08172T_NIBBLE_BUFFER_LEN          5
#define GH08172T_NIBBLE_BUFFER_BAR2_3_INDEX GH08172T_NIBBLE_BUFFER_LEN - 1

#define GH08172T_ASCII_CHAR_0                 0x30 /* '0' */
#define GH08172T_ASCII_CHAR_AT_SYMBOL         0x40 /* '@' */
#define GH08172T_ASCII_CHAR_LEFT_OPEN_BRACKET 0x5B /* '[' */
#define GH08172T_ASCII_CHAR_APOSTROPHE        0x60 /* '`' */
#define GH08172T_ASCII_CHAR_LEFT_OPEN_BRACE   0x7B /* '(' */

/* LCD Glass point
 * Element values correspond to LCD Glass point, for positions 1 to 4.
 */

enum GH08172T_point {
	GH08172T_POINT_OFF = 0,
	GH08172T_POINT_SINGLE,
	GH08172T_POINT_DOUBLE,
	GH08172T_POINT_TRIPLE
};

struct gh08172t_register_address {
	mem_addr_t cr;
	mem_addr_t fcr;
	mem_addr_t sr;
	mem_addr_t clr;
	uint32_t *ram;
};

#define GH08172T_RAM_ADDRESS(i) (mem_addr_t)(&config->lcd.ram[i])

/* Immutabile driver configuration structure stored in Flash */
struct auxdisplay_gh08172t_config {
	struct auxdisplay_capabilities capabilities;
	const struct gh08172t_register_address lcd;
	const struct stm32_pclken *pclken;
	const struct device *clk_dev;
	const struct pinctrl_dev_config *pcfg;
	uint32_t lcd_timeout_us;
};

struct gh08172t_init_data {
	/* register FCR */
	uint32_t prescaler;
	uint32_t divider;
	uint32_t blink_mode;
	uint32_t blink_frequency;
	uint32_t dead_time;
	uint32_t pulse_on_duration;
	uint32_t contrast;
	uint32_t high_drive;
	/* register CR */
	uint32_t duty;
	uint32_t bias;
	uint32_t voltage_source;
	uint32_t mux_segment;
};

/* Mutable driver runtime instance data structure stored in RAM */
struct auxdisplay_gh08172t_data {
	uint16_t character_x;
	uint16_t character_y;
};

#define GH08172T_WAIT_FOR(bool_res, cond_expr, timeout_us, delay_stmt)                             \
	{                                                                                          \
		uint32_t _gwf_cycle_count = k_us_to_cyc_ceil32(timeout_us);                        \
		uint32_t _gwf_start = k_cycle_get_32();                                            \
		while (!(bool_res = (cond_expr)) &&                                                \
		       (_gwf_cycle_count > (k_cycle_get_32() - _gwf_start))) {                     \
			delay_stmt;                                                                \
			Z_SPIN_DELAY(10);                                                          \
		}                                                                                  \
	}

static void gh08172t_enable(const struct device *dev)
{
	const struct auxdisplay_gh08172t_config *config = dev->config;

	sys_set_bits(config->lcd.cr, GH08172T_CR_LCDEN_MASK);
}

static void gh08172t_disable(const struct device *dev)
{
	const struct auxdisplay_gh08172t_config *config = dev->config;

	sys_clear_bits(config->lcd.cr, GH08172T_CR_LCDEN_MASK);
}

static int auxdisplay_gh08172t_capabilities_get(const struct device *dev,
						struct auxdisplay_capabilities *capabilities)
{
	const struct auxdisplay_gh08172t_config *config = dev->config;

	if (!capabilities) {
		return -EINVAL;
	}

	memcpy(capabilities, &config->capabilities, sizeof(struct auxdisplay_capabilities));
	return 0;
}

static int gh08172t_clear(const struct device *dev)
{
	const struct auxdisplay_gh08172t_config *config = dev->config;
	bool success;

	/* Wait Until the LCD is ready */
	GH08172T_WAIT_FOR(success, (sys_read32(config->lcd.sr) & GH08172T_SR_UDR_MASK) == 0,
			  config->lcd_timeout_us, k_msleep(1));
	if (!success) {
		LOG_ERR("Display clear timeout");
		return -ETIMEDOUT;
	}

	/* Clear the LCD RAM registers */
	for (int i = 0; i < GH08172T_RAM_REG_COUNT; i++) {
		sys_write32(0, GH08172T_RAM_ADDRESS(i));
	}
	return 0;
}

static int gh08172t_write_ram(const struct device *dev, uint32_t ram_register, uint32_t mask,
			      uint32_t data)
{
	const struct auxdisplay_gh08172t_config *config = dev->config;
	bool success;
	uint32_t current_val;
	uint32_t new_val;

	if (!(ram_register < GH08172T_RAM_REG_COUNT)) {
		LOG_ERR("Unsopported RAM register (%d)", ram_register);
		return EINVAL;
	}

	/* Wait Until the LCD is ready */
	GH08172T_WAIT_FOR(success, (sys_read32(config->lcd.sr) & GH08172T_SR_UDR_MASK) == 0,
			  config->lcd_timeout_us, k_msleep(1));
	if (!success) {
		LOG_ERR("Display write timeout");
		return -ETIMEDOUT;
	}

	current_val = sys_read32(GH08172T_RAM_ADDRESS(ram_register));
	new_val = (current_val & mask) | data;
	sys_write32(new_val, GH08172T_RAM_ADDRESS(ram_register));
	return 0;
}

static int gh08172t_update_request(const struct device *dev)
{
	const struct auxdisplay_gh08172t_config *config = dev->config;
	bool success;

	/* Clear the Update Display Done flag before starting the update display request */
	sys_write32(GH08172T_CLR_UDDC_MASK, config->lcd.clr);

	/* Enable the display request */
	sys_set_bits(config->lcd.sr, GH08172T_SR_UDR_MASK);

	/* Wait Until the LCD display is done */
	GH08172T_WAIT_FOR(success, (sys_read32(config->lcd.sr) & GH08172T_SR_UDD_MASK) != 0,
			  config->lcd_timeout_us, k_msleep(1));
	if (!success) {
		LOG_ERR("Display update request timeout");
		return -ETIMEDOUT;
	}
	return 0;
}

/* Convert an ascii char to the an LCD char pattern, handling points and bars too.
 * the_char:  the char to display.
 * position:  the character LCD destination [1:6].
 * point:     a point or colon to display right after the character.
 *            Allowed in positions 1 to 4, ignored otherwise.
 *            Valid values: 0x0, GH08172T_ASCII_DOT, GH08172T_ASCII_DOUBLE_DOT,
 *            GH08172T_ASCII_TRIPLE_DOT
 * bar_value: the value (enum lcd_bar_value) to display with bars, in position 6 only.
 * pattern:   output, pattern frame buffer (length is GH08172T_NIBBLE_BUFFER_LEN).
 */
static void gh08172t_convert_char_to_pattern(uint8_t the_char, uint8_t position, uint8_t point,
					     uint8_t bar_value, uint32_t *pattern)
{
	uint16_t ch = 0;

	switch (the_char) {
	case ' ':
		ch = GH08172T_C_SPACE_MAP;
		break;
	case '_':
		ch = GH08172T_C_UNDERSCORE_MAP;
		break;
	case 'd':
		ch = GH08172T_C_D_MAP;
		break;
	case 'c':
		ch = GH08172T_C_C_MAP;
		break;
	case 'm':
		ch = GH08172T_C_M_MAP;
		break;
	case 'u':
		ch = GH08172T_C_U_MAP;
		break;
	case 'n':
		ch = GH08172T_C_N_MAP;
		break;
	case '-':
		ch = GH08172T_C_MINUS_MAP;
		break;
	case '+':
		ch = GH08172T_C_PLUS_MAP;
		break;
	case '/':
		ch = GH08172T_C_SLASH_MAP;
		break;
	case '*':
		ch = GH08172T_C_STAR_MAP;
		break;
	case '(':
		ch = GH08172T_C_OPEN_PAR_MAP;
		break;
	case ')':
		ch = GH08172T_C_CLOSE_PAR_MAP;
		break;
	case GH08172T_ASCII_CHAR_DEGREE_SIGN:
		ch = GH08172T_C_DEGREE_MAP;
		break;
	case '%':
		ch = GH08172T_C_LOW_RING_MAP;
		break;
	case GH08172T_ASCII_CHAR_FULL:
		ch = GH08172T_C_FULL_MAP;
		break;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		ch = auxdisplay_gh08172t_number_map[the_char - GH08172T_ASCII_CHAR_0];
		break;
	default:
		/* The character is one upper case letter */
		if ((the_char < GH08172T_ASCII_CHAR_LEFT_OPEN_BRACKET) &&
		    (the_char > GH08172T_ASCII_CHAR_AT_SYMBOL)) {
			ch = auxdisplay_gh08172t_cap_letter_map[the_char - 'A'];
		}
		break;
	}

	/* Add bits for points for position 1..4 */
	if (position < GH08172T_DIGIT_POSITION_5) {
		if (point == GH08172T_POINT_SINGLE || point == GH08172T_POINT_TRIPLE) {
			ch |= 0x0002;
		}
		if (point == GH08172T_POINT_DOUBLE || point == GH08172T_POINT_TRIPLE) {
			ch |= 0x0020;
		}
	}

	/* Add bits for bars for position 6 */
	pattern[GH08172T_NIBBLE_BUFFER_BAR2_3_INDEX] = 0;
	if (position == GH08172T_DIGIT_POSITION_6 && bar_value) {
		/* Bars 0 and 1 are mapped on the same segments as points */
		if (bar_value & 0x01) {
			ch |= 0x0002;
		}
		if (bar_value & 0x02) {
			ch |= 0x0020;
		}
		/* Bars 2 and 3 are mapped on never used points segments of positions 5 */
		if (bar_value & 0x04) {
			pattern[GH08172T_NIBBLE_BUFFER_BAR2_3_INDEX] |= 0x0001;
		}
		if (bar_value & 0x08) {
			pattern[GH08172T_NIBBLE_BUFFER_BAR2_3_INDEX] |= 0x0002;
		}
	}

	/* Isolate the less significant 4 bits */
	for (int loop = 12, index = 0; index < 4; loop -= 4, index++) {
		pattern[index] = (ch >> loop) & 0x0f;
	}
}

/* Rationale to have a single function for each position to write patterns to the display registers.
 *
 * 1. Each position refers to different segments and different COM channels.
 * 2. Furthermore, channels 1, 2, 3 and 6 use just 4 COM channels, whilst channels 4 and 5 use 8 COM
 * channels.
 * 3. There is another asymmetry: positions 1..4 do have the single decimal point and the middle
 * colon, both handled independently with a single segment, thus they have two side segments each;
 * position 5 does not have any dot, thus it has zero side segments; and position 6 has 4 bars, 2
 * driven as much as pos 1..4 dot/colon and 2 driven as the theoretical position 5 dots, thus
 * position 6 has four side segments. We could use a table to handle all of this, but it would be
 * less performant than the current unrolled implementation.
 * 4. Each position has its own function to satisfy the SonarQube complexity check.
 *
 * The basic difference from usual 7 segments displays with indicators is that here we cannot use an
 * index as simple as
 *
 *                          position * 14 + segment
 *
 * and dots/bars segments are interleaved with segments used for characters. Even worse, they are
 * driven by the same COM channels; the current implementation makes it possible to set each COM
 * channel once per writing instead of two separated calls for character and indicator.
 */
static inline int gh08172t_write_pattern_to_pos_1(const struct device *dev, const uint32_t *pattern)
{
	uint32_t data;
	int ret;

	data = ((pattern[0] & 0x1) << GH08172T_SEG0_SHIFT) |
	       (((pattern[0] & 0x2) >> 1) << GH08172T_SEG1_SHIFT) |
	       (((pattern[0] & 0x4) >> 2) << GH08172T_SEG22_SHIFT) |
	       (((pattern[0] & 0x8) >> 3) << GH08172T_SEG23_SHIFT);
	ret = gh08172t_write_ram(dev, GH08172T_DIGIT1_COM0, GH08172T_DIGIT1_COM0_SEG_MASK,
				 data); /* 1G 1B 1M 1E */
	if (ret) {
		return ret;
	}

	data = ((pattern[1] & 0x1) << GH08172T_SEG0_SHIFT) |
	       (((pattern[1] & 0x2) >> 1) << GH08172T_SEG1_SHIFT) |
	       (((pattern[1] & 0x4) >> 2) << GH08172T_SEG22_SHIFT) |
	       (((pattern[1] & 0x8) >> 3) << GH08172T_SEG23_SHIFT);
	ret = gh08172t_write_ram(dev, GH08172T_DIGIT1_COM1, GH08172T_DIGIT1_COM1_SEG_MASK,
				 data); /* 1F 1A 1C 1D */
	if (ret) {
		return ret;
	}

	data = ((pattern[2] & 0x1) << GH08172T_SEG0_SHIFT) |
	       (((pattern[2] & 0x2) >> 1) << GH08172T_SEG1_SHIFT) |
	       (((pattern[2] & 0x4) >> 2) << GH08172T_SEG22_SHIFT) |
	       (((pattern[2] & 0x8) >> 3) << GH08172T_SEG23_SHIFT);
	ret = gh08172t_write_ram(dev, GH08172T_DIGIT1_COM2, GH08172T_DIGIT1_COM2_SEG_MASK,
				 data); /* 1Q 1K 1Col 1P  */
	if (ret) {
		return ret;
	}

	data = ((pattern[3] & 0x1) << GH08172T_SEG0_SHIFT) |
	       (((pattern[3] & 0x2) >> 1) << GH08172T_SEG1_SHIFT) |
	       (((pattern[3] & 0x4) >> 2) << GH08172T_SEG22_SHIFT) |
	       (((pattern[3] & 0x8) >> 3) << GH08172T_SEG23_SHIFT);
	ret = gh08172t_write_ram(dev, GH08172T_DIGIT1_COM3, GH08172T_DIGIT1_COM3_SEG_MASK,
				 data); /* 1H 1J 1DP 1N  */
	if (ret) {
		return ret;
	}

	return 0;
}

static inline int gh08172t_write_pattern_to_pos_2(const struct device *dev, const uint32_t *pattern)
{
	uint32_t data;
	int ret;

	data = ((pattern[0] & 0x1) << GH08172T_SEG2_SHIFT) |
	       (((pattern[0] & 0x2) >> 1) << GH08172T_SEG3_SHIFT) |
	       (((pattern[0] & 0x4) >> 2) << GH08172T_SEG20_SHIFT) |
	       (((pattern[0] & 0x8) >> 3) << GH08172T_SEG21_SHIFT);
	ret = gh08172t_write_ram(dev, GH08172T_DIGIT2_COM0, GH08172T_DIGIT2_COM0_SEG_MASK,
				 data); /* 1G 1B 1M 1E */
	if (ret) {
		return ret;
	}

	data = ((pattern[1] & 0x1) << GH08172T_SEG2_SHIFT) |
	       (((pattern[1] & 0x2) >> 1) << GH08172T_SEG3_SHIFT) |
	       (((pattern[1] & 0x4) >> 2) << GH08172T_SEG20_SHIFT) |
	       (((pattern[1] & 0x8) >> 3) << GH08172T_SEG21_SHIFT);
	ret = gh08172t_write_ram(dev, GH08172T_DIGIT2_COM1, GH08172T_DIGIT2_COM1_SEG_MASK,
				 data); /* 1F 1A 1C 1D */
	if (ret) {
		return ret;
	}

	data = ((pattern[2] & 0x1) << GH08172T_SEG2_SHIFT) |
	       (((pattern[2] & 0x2) >> 1) << GH08172T_SEG3_SHIFT) |
	       (((pattern[2] & 0x4) >> 2) << GH08172T_SEG20_SHIFT) |
	       (((pattern[2] & 0x8) >> 3) << GH08172T_SEG21_SHIFT);
	ret = gh08172t_write_ram(dev, GH08172T_DIGIT2_COM2, GH08172T_DIGIT2_COM2_SEG_MASK,
				 data); /* 1Q 1K 1Col 1P  */
	if (ret) {
		return ret;
	}

	data = ((pattern[3] & 0x1) << GH08172T_SEG2_SHIFT) |
	       (((pattern[3] & 0x2) >> 1) << GH08172T_SEG3_SHIFT) |
	       (((pattern[3] & 0x4) >> 2) << GH08172T_SEG20_SHIFT) |
	       (((pattern[3] & 0x8) >> 3) << GH08172T_SEG21_SHIFT);
	ret = gh08172t_write_ram(dev, GH08172T_DIGIT2_COM3, GH08172T_DIGIT2_COM3_SEG_MASK,
				 data); /* 1H 1J 1DP 1N  */
	if (ret) {
		return ret;
	}

	return 0;
}

static inline int gh08172t_write_pattern_to_pos_3(const struct device *dev, const uint32_t *pattern)
{
	uint32_t data;
	int ret;

	data = ((pattern[0] & 0x1) << GH08172T_SEG4_SHIFT) |
	       (((pattern[0] & 0x2) >> 1) << GH08172T_SEG5_SHIFT) |
	       (((pattern[0] & 0x4) >> 2) << GH08172T_SEG18_SHIFT) |
	       (((pattern[0] & 0x8) >> 3) << GH08172T_SEG19_SHIFT);
	ret = gh08172t_write_ram(dev, GH08172T_DIGIT3_COM0, GH08172T_DIGIT3_COM0_SEG_MASK,
				 data); /* 1G 1B 1M 1E */
	if (ret) {
		return ret;
	}

	data = ((pattern[1] & 0x1) << GH08172T_SEG4_SHIFT) |
	       (((pattern[1] & 0x2) >> 1) << GH08172T_SEG5_SHIFT) |
	       (((pattern[1] & 0x4) >> 2) << GH08172T_SEG18_SHIFT) |
	       (((pattern[1] & 0x8) >> 3) << GH08172T_SEG19_SHIFT);
	ret = gh08172t_write_ram(dev, GH08172T_DIGIT3_COM1, GH08172T_DIGIT3_COM1_SEG_MASK,
				 data); /* 1F 1A 1C 1D */
	if (ret) {
		return ret;
	}

	data = ((pattern[2] & 0x1) << GH08172T_SEG4_SHIFT) |
	       (((pattern[2] & 0x2) >> 1) << GH08172T_SEG5_SHIFT) |
	       (((pattern[2] & 0x4) >> 2) << GH08172T_SEG18_SHIFT) |
	       (((pattern[2] & 0x8) >> 3) << GH08172T_SEG19_SHIFT);
	ret = gh08172t_write_ram(dev, GH08172T_DIGIT3_COM2, GH08172T_DIGIT3_COM2_SEG_MASK,
				 data); /* 1Q 1K 1Col 1P  */
	if (ret) {
		return ret;
	}

	data = ((pattern[3] & 0x1) << GH08172T_SEG4_SHIFT) |
	       (((pattern[3] & 0x2) >> 1) << GH08172T_SEG5_SHIFT) |
	       (((pattern[3] & 0x4) >> 2) << GH08172T_SEG18_SHIFT) |
	       (((pattern[3] & 0x8) >> 3) << GH08172T_SEG19_SHIFT);
	ret = gh08172t_write_ram(dev, GH08172T_DIGIT3_COM3, GH08172T_DIGIT3_COM3_SEG_MASK,
				 data); /* 1H 1J 1DP 1N  */
	if (ret) {
		return ret;
	}

	return 0;
}

static inline int gh08172t_write_pattern_to_pos_4(const struct device *dev, const uint32_t *pattern)
{
	uint32_t data;
	int ret;

	data = ((pattern[0] & 0x1) << GH08172T_SEG6_SHIFT) |
	       (((pattern[0] & 0x8) >> 3) << GH08172T_SEG17_SHIFT);
	ret = gh08172t_write_ram(dev, GH08172T_DIGIT4_COM0, GH08172T_DIGIT4_COM0_SEG_MASK,
				 data); /* 1G 1B 1M 1E */
	if (ret) {
		return ret;
	}

	data = (((pattern[0] & 0x2) >> 1) << GH08172T_SEG7_SHIFT) |
	       (((pattern[0] & 0x4) >> 2) << GH08172T_SEG16_SHIFT);
	ret = gh08172t_write_ram(dev, GH08172T_DIGIT4_COM0_1, GH08172T_DIGIT4_COM0_1_SEG_MASK,
				 data); /* 1G 1B 1M 1E */
	if (ret) {
		return ret;
	}

	data = ((pattern[1] & 0x1) << GH08172T_SEG6_SHIFT) |
	       (((pattern[1] & 0x8) >> 3) << GH08172T_SEG17_SHIFT);
	ret = gh08172t_write_ram(dev, GH08172T_DIGIT4_COM1, GH08172T_DIGIT4_COM1_SEG_MASK,
				 data); /* 1F 1A 1C 1D */
	if (ret) {
		return ret;
	}

	data = (((pattern[1] & 0x2) >> 1) << GH08172T_SEG7_SHIFT) |
	       (((pattern[1] & 0x4) >> 2) << GH08172T_SEG16_SHIFT);
	ret = gh08172t_write_ram(dev, GH08172T_DIGIT4_COM1_1, GH08172T_DIGIT4_COM1_1_SEG_MASK,
				 data); /* 1F 1A 1C 1D  */
	if (ret) {
		return ret;
	}

	data = ((pattern[2] & 0x1) << GH08172T_SEG6_SHIFT) |
	       (((pattern[2] & 0x8) >> 3) << GH08172T_SEG17_SHIFT);
	ret = gh08172t_write_ram(dev, GH08172T_DIGIT4_COM2, GH08172T_DIGIT4_COM2_SEG_MASK,
				 data); /* 1Q 1K 1Col 1P  */
	if (ret) {
		return ret;
	}

	data = (((pattern[2] & 0x2) >> 1) << GH08172T_SEG7_SHIFT) |
	       (((pattern[2] & 0x4) >> 2) << GH08172T_SEG16_SHIFT);
	ret = gh08172t_write_ram(dev, GH08172T_DIGIT4_COM2_1, GH08172T_DIGIT4_COM2_1_SEG_MASK,
				 data); /* 1Q 1K 1Col 1P */
	if (ret) {
		return ret;
	}

	data = ((pattern[3] & 0x1) << GH08172T_SEG6_SHIFT) |
	       (((pattern[3] & 0x8) >> 3) << GH08172T_SEG17_SHIFT);
	ret = gh08172t_write_ram(dev, GH08172T_DIGIT4_COM3, GH08172T_DIGIT4_COM3_SEG_MASK,
				 data); /* 1H 1J 1DP 1N  */
	if (ret) {
		return ret;
	}

	data = (((pattern[3] & 0x2) >> 1) << GH08172T_SEG7_SHIFT) |
	       (((pattern[3] & 0x4) >> 2) << GH08172T_SEG16_SHIFT);
	ret = gh08172t_write_ram(dev, GH08172T_DIGIT4_COM3_1, GH08172T_DIGIT4_COM3_1_SEG_MASK,
				 data); /* 1H 1J 1DP 1N  */
	if (ret) {
		return ret;
	}

	return 0;
}

static inline int gh08172t_write_pattern_to_pos_5(const struct device *dev, const uint32_t *pattern)
{
	uint32_t data;
	int ret;

	data = (((pattern[0] & 0x2) >> 1) << GH08172T_SEG9_SHIFT) |
	       (((pattern[0] & 0x4) >> 2) << GH08172T_SEG14_SHIFT);
	ret = gh08172t_write_ram(dev, GH08172T_DIGIT5_COM0, GH08172T_DIGIT5_COM0_SEG_MASK,
				 data); /* 1G 1B 1M 1E */
	if (ret) {
		return ret;
	}

	data = ((pattern[0] & 0x1) << GH08172T_SEG8_SHIFT) |
	       (((pattern[0] & 0x8) >> 3) << GH08172T_SEG15_SHIFT);
	ret = gh08172t_write_ram(dev, GH08172T_DIGIT5_COM0_1, GH08172T_DIGIT5_COM0_1_SEG_MASK,
				 data); /* 1G 1B 1M 1E */
	if (ret) {
		return ret;
	}

	data = (((pattern[1] & 0x2) >> 1) << GH08172T_SEG9_SHIFT) |
	       (((pattern[1] & 0x4) >> 2) << GH08172T_SEG14_SHIFT);
	ret = gh08172t_write_ram(dev, GH08172T_DIGIT5_COM1, GH08172T_DIGIT5_COM1_SEG_MASK,
				 data); /* 1F 1A 1C 1D */
	if (ret) {
		return ret;
	}

	data = ((pattern[1] & 0x1) << GH08172T_SEG8_SHIFT) |
	       (((pattern[1] & 0x8) >> 3) << GH08172T_SEG15_SHIFT);
	ret = gh08172t_write_ram(dev, GH08172T_DIGIT5_COM1_1, GH08172T_DIGIT5_COM1_1_SEG_MASK,
				 data); /* 1F 1A 1C 1D  */
	if (ret) {
		return ret;
	}

	data = /* (((digit[2] & 0x2) >> 1) << GH08172T_SEG9_SHIFT) | */ (((pattern[2] & 0x4) >> 2)
									 << GH08172T_SEG14_SHIFT);
	ret = gh08172t_write_ram(dev, GH08172T_DIGIT5_COM2,
				 GH08172T_DIGIT5_COM2_SEG_MASK | GH08172T_SEG9,
				 data); /* 1Q 1K 1Col 1P  */
	if (ret) {
		return ret;
	}

	data = ((pattern[2] & 0x1) << GH08172T_SEG8_SHIFT) |
	       (((pattern[2] & 0x8) >> 3) << GH08172T_SEG15_SHIFT);
	ret = gh08172t_write_ram(dev, GH08172T_DIGIT5_COM2_1, GH08172T_DIGIT5_COM2_1_SEG_MASK,
				 data); /* 1Q 1K 1Col 1P */
	if (ret) {
		return ret;
	}

	data = /*(((digit[3] & 0x2) >> 1) << GH08172T_SEG9_SHIFT) | */ (((pattern[3] & 0x4) >> 2)
									<< GH08172T_SEG14_SHIFT);
	ret = gh08172t_write_ram(dev, GH08172T_DIGIT5_COM3,
				 GH08172T_DIGIT5_COM3_SEG_MASK | GH08172T_SEG9,
				 data); /* 1H 1J 1DP 1N  */
	if (ret) {
		return ret;
	}

	data = ((pattern[3] & 0x1) << GH08172T_SEG8_SHIFT) |
	       (((pattern[3] & 0x8) >> 3) << GH08172T_SEG15_SHIFT);
	ret = gh08172t_write_ram(dev, GH08172T_DIGIT5_COM3_1, GH08172T_DIGIT5_COM3_1_SEG_MASK,
				 data); /* 1H 1J 1DP 1N  */
	if (ret) {
		return ret;
	}

	return 0;
}

static inline int gh08172t_write_pattern_to_pos_6(const struct device *dev, const uint32_t *pattern)
{
	uint32_t data;
	int ret;

	data = ((pattern[0] & 0x1) << GH08172T_SEG10_SHIFT) |
	       (((pattern[0] & 0x2) >> 1) << GH08172T_SEG11_SHIFT) |
	       (((pattern[0] & 0x4) >> 2) << GH08172T_SEG12_SHIFT) |
	       (((pattern[0] & 0x8) >> 3) << GH08172T_SEG13_SHIFT);
	ret = gh08172t_write_ram(dev, GH08172T_DIGIT6_COM0, GH08172T_DIGIT6_COM0_SEG_MASK,
				 data); /* 1G 1B 1M 1E */
	if (ret) {
		return ret;
	}

	data = ((pattern[1] & 0x1) << GH08172T_SEG10_SHIFT) |
	       (((pattern[1] & 0x2) >> 1) << GH08172T_SEG11_SHIFT) |
	       (((pattern[1] & 0x4) >> 2) << GH08172T_SEG12_SHIFT) |
	       (((pattern[1] & 0x8) >> 3) << GH08172T_SEG13_SHIFT);
	ret = gh08172t_write_ram(dev, GH08172T_DIGIT6_COM1, GH08172T_DIGIT6_COM1_SEG_MASK,
				 data); /* 1F 1A 1C 1D */
	if (ret) {
		return ret;
	}

	data = ((pattern[2] & 0x1) << GH08172T_SEG10_SHIFT) |
	       (((pattern[2] & 0x2) >> 1) << GH08172T_SEG11_SHIFT) |
	       (((pattern[2] & 0x4) >> 2) << GH08172T_SEG12_SHIFT) |
	       (((pattern[2] & 0x8) >> 3) << GH08172T_SEG13_SHIFT) |
	       (((pattern[GH08172T_NIBBLE_BUFFER_BAR2_3_INDEX] & 0x2) >> 1) << GH08172T_SEG9_SHIFT);
	ret = gh08172t_write_ram(dev, GH08172T_DIGIT6_COM2,
				 GH08172T_DIGIT6_COM2_SEG_MASK & GH08172T_BAR3_SEG_MASK,
				 data); /* 1Q 1K BAR0 1P  */
	if (ret) {
		return ret;
	}

	data = ((pattern[3] & 0x1) << GH08172T_SEG10_SHIFT) |
	       (((pattern[3] & 0x2) >> 1) << GH08172T_SEG11_SHIFT) |
	       (((pattern[3] & 0x4) >> 2) << GH08172T_SEG12_SHIFT) |
	       (((pattern[3] & 0x8) >> 3) << GH08172T_SEG13_SHIFT) |
	       ((pattern[GH08172T_NIBBLE_BUFFER_BAR2_3_INDEX] & 0x1) << GH08172T_SEG9_SHIFT);
	ret = gh08172t_write_ram(dev, GH08172T_DIGIT6_COM3,
				 GH08172T_DIGIT6_COM3_SEG_MASK & GH08172T_BAR2_SEG_MASK,
				 data); /* 1H 1J BAR1 1N  */
	if (ret) {
		return ret;
	}

	return 0;
}

/* Write a character in the LCD display, including points and bars.
 * dev:       the LCD display device.
 * ch:        the character to display.
 * position:  the character LCD destination [1:6].
 * point:     a point or colon to display right after the character.
 *            Allowed in positions 1 to 4, ignored otherwise.
 *            Valid values: 0x0, GH08172T_ASCII_DOT, GH08172T_ASCII_DOUBLE_DOT,
 *            GH08172T_ASCII_TRIPLE_DOT
 * bar_value: the value (enum lcd_bar_value) to display with bars, in position 6 only.
 */
static int gh08172t_write_char(const struct device *dev, uint8_t ch, uint8_t position,
			       uint8_t point, uint8_t bar_value)
{
	/* To convert displayed character in segment in array digit */
	uint32_t pattern[GH08172T_NIBBLE_BUFFER_LEN];
	int ret = 0;

	gh08172t_convert_char_to_pattern(ch, position, point, bar_value, pattern);

	switch (position) {
	/* Position 1 on LCD (Digit1)*/
	case GH08172T_DIGIT_POSITION_1:
		ret = gh08172t_write_pattern_to_pos_1(dev, pattern);
		break;

	/* Position 2 on LCD (Digit2)*/
	case GH08172T_DIGIT_POSITION_2:
		ret = gh08172t_write_pattern_to_pos_2(dev, pattern);
		break;

	/* Position 3 on LCD (Digit3)*/
	case GH08172T_DIGIT_POSITION_3:
		ret = gh08172t_write_pattern_to_pos_3(dev, pattern);
		break;

	/* Position 4 on LCD (Digit4)*/
	case GH08172T_DIGIT_POSITION_4:
		ret = gh08172t_write_pattern_to_pos_4(dev, pattern);
		break;

	/* Position 5 on LCD (Digit5)*/
	case GH08172T_DIGIT_POSITION_5:
		ret = gh08172t_write_pattern_to_pos_5(dev, pattern);
		break;

	/* Position 6 on LCD (Digit6)*/
	case GH08172T_DIGIT_POSITION_6:
		ret = gh08172t_write_pattern_to_pos_6(dev, pattern);
		break;

	default:
		break;
	}
	return ret;
}

static int auxdisplay_gh08172t_clear(const struct device *dev)
{
	int ret;

	ret = gh08172t_clear(dev);
	if (ret) {
		return ret;
	}

	ret = gh08172t_update_request(dev);
	if (ret) {
		return ret;
	}

	return 0;
}

/* Symbols spanning over multiple positions. */
static inline int auxdisplay_gh08172t_write_long_symbol(const struct device *dev, uint8_t ch,
							int *position)
{
	const struct auxdisplay_gh08172t_config *config = dev->config;
	int ret;

	if (ch == '%') {
		if (*position + GH08172T_PERCENTAGE_SYMBOL_POSITIONS >
		    config->capabilities.columns) {
			return EINVAL;
		}
		ret = gh08172t_write_char(dev, GH08172T_ASCII_CHAR_DEGREE_SIGN, *position,
					  GH08172T_POINT_OFF, GH08172T_BAR_0);
		if (ret) {
			return ret;
		}
		(*position)++;
		ret = gh08172t_write_char(dev, '/', *position, GH08172T_POINT_OFF, GH08172T_BAR_0);
		if (ret) {
			(*position)--;
			return ret;
		}
		(*position)++;
	}
	return 0;
}

static inline void gh08172t_prepare_dot_and_bars(const struct device *dev, uint8_t ch, int position,
						 int *i, uint8_t *point, uint8_t *bar_value)
{
	/* points in position 1..4; position 5 is ignored in later in
	 * gh08172t_convert_char_to_pattern().
	 */
	if (position < GH08172T_DIGIT_POSITION_6) {
		switch (ch) {
		case GH08172T_ASCII_DOT:
			*point = GH08172T_POINT_SINGLE;
			(*i)++;
			break;
		case GH08172T_ASCII_DOUBLE_DOT:
			*point = GH08172T_POINT_DOUBLE;
			(*i)++;
			break;
		case GH08172T_ASCII_TRIPLE_DOT:
			*point = GH08172T_POINT_TRIPLE;
			(*i)++;
			break;
		default:
			break;
		}
	} else if (ch < GH08172T_BAR_MAX) { /* LCD bars in position 6. */
		*bar_value = ch;
		(*i)++;
	} else {
		/* Try to treat it as a regular character on next iteration. */
	}
}

/* Each position, but pos 5, handles dots/bars together with the character itself; this minimizes
 * the numbers of register writings.
 * For this reason, symbols spanning over multiple positions are treated by a specific function, but
 * not their last position, to have the usual dot/bars handling.
 */
static int auxdisplay_gh08172t_write(const struct device *dev, const uint8_t *data, uint16_t len)
{
	const struct auxdisplay_gh08172t_config *config = dev->config;
	struct auxdisplay_gh08172t_data *driver_data = dev->data;
	int ret;

	/* Loop to update segments sequentially up to the physical maximum string length
	 * restriction.
	 */
	for (int i = 0, position = driver_data->character_x;
	     i < len && position < config->capabilities.columns; i++, position++) {
		const uint8_t one_char = data[i];
		uint8_t point = GH08172T_POINT_OFF;
		uint8_t bar_value = 0;

		ret = auxdisplay_gh08172t_write_long_symbol(dev, one_char, &position);
		if (ret) {
			/* Since the symbol initial part couldn't be written, let's skip the last
			 * part and its dots/bars.
			 */
			continue;
		}

		if (i + 1 < len) {
			gh08172t_prepare_dot_and_bars(dev, data[i + 1], position, &i, &point,
						      &bar_value);
		}

		ret = gh08172t_write_char(dev, one_char, position, point, bar_value);
		if (ret) {
			return ret;
		}
	}

	/* Fire a hardware request telling the peripheral controller to push internal RAM
	 * data to the glass.
	 */
	ret = gh08172t_update_request(dev);
	if (ret) {
		return ret;
	}

	return 0;
}

/* Map implementation functions to the standard public Zephyr auxdisplay API interface. */
static DEVICE_API(auxdisplay, auxdisplay_gh08172t_auxdisplay_api) = {
	.capabilities_get = auxdisplay_gh08172t_capabilities_get,
	.clear = auxdisplay_gh08172t_clear,
	.write = auxdisplay_gh08172t_write,
};

/* Core device initialization logic executed automatically by the Zephyr kernel boot sequencer. */
static int auxdisplay_gh08172t_init(const struct device *dev)
{
	const struct auxdisplay_gh08172t_config *config = dev->config;
	struct auxdisplay_gh08172t_data *driver_data = dev->data;
	int ret;
	bool success;

	/* Request clock gating initialization from the generic Zephyr device sub-system tree. */
	if (!device_is_ready(config->clk_dev)) {
		LOG_ERR("Clock Control driver device is not ready");
		return -ENODEV;
	}

	/* Remove the const qualifier in a maintainable way.
	 * Any other way to cast it makes Sonarqube detect a "const drop" issue.
	 */
	void *config_pclken_subsys_ptr = (clock_control_subsys_t)(uintptr_t)config->pclken;

	ret = clock_control_on(config->clk_dev, (clock_control_subsys_t)config_pclken_subsys_ptr);
	if (ret) {
		LOG_ERR("Failed to enable peripheral clock gate (err: %d)", ret);
		return ret;
	}

	/* Enforce the required pin alternate configurations natively via the Pinctrl
	 * manager framework.
	 */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		LOG_ERR("Failed to apply pinctrl default operational states (err: %d)", ret);
		return ret;
	}

	/* Enable the LCD clock */
	k_busy_wait(2000);
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_LCD);

	/* Disable the peripheral */
	gh08172t_disable(dev);

	/* Clear the LCD RAM registers */
	for (int i = 0; i < GH08172T_RAM_REG_COUNT; i++) {
		sys_write32(0, GH08172T_RAM_ADDRESS(i));
	}

	/* Enable the display request */
	sys_set_bits(config->lcd.sr, GH08172T_SR_UDR_MASK);

	/* Hardware parameters tuning extracted directly from STM32Cube peripheral
	 * specifications.
	 */
	struct gh08172t_init_data init = {
		.prescaler = GH08172T_PRESCALER_1,
		.divider = GH08172T_DIVIDER_31,
		.blink_mode = GH08172T_BLINK_OFF,
		.blink_frequency = GH08172T_BLINK_FREQ_DIV32,
		.dead_time = GH08172T_DEADTIME_0,
		.pulse_on_duration = GH08172T_PULSEONDURATION_4,
		.contrast = GH08172T_CONTRASTLEVEL_5,
		.high_drive = GH08172T_FCR_HIGH_DRIVE_DISABLE,
		.duty = GH08172T_DUTY_1_4,
		.bias = GH08172T_BIAS_1_3,
		.voltage_source = GH08172T_CR_VSEL_INTERNAL,
		.mux_segment = GH08172T_CR_MUX_SEG_DISABLE,
	};

	/* Configure the LCD in register FCR*/
	uint32_t mask = GH08172T_FCR_PS_MASK | GH08172T_FCR_DIV_MASK | GH08172T_FCR_BLINK_MASK |
			GH08172T_FCR_BLINK_FREQ_MASK | GH08172T_FCR_DEAD_MASK |
			GH08172T_FCR_PON_MASK | GH08172T_FCR_CC_MASK | GH08172T_FCR_HIGH_DRIVE_MASK;
	uint32_t data = init.prescaler | init.divider | init.blink_mode | init.blink_frequency |
			init.dead_time | init.pulse_on_duration | init.contrast | init.high_drive;
	uint32_t current_val = sys_read32(config->lcd.fcr);
	uint32_t new_val = (current_val & mask) | data;

	sys_write32(new_val, config->lcd.fcr);

	/* Loop until FCRSF flag is set */
	GH08172T_WAIT_FOR(success, (sys_read32(config->lcd.sr) & GH08172T_SR_FCRSR_MASK) != 0,
			  config->lcd_timeout_us, k_msleep(1));
	if (!success) {
		LOG_ERR("Display init timeout");
		return -ETIMEDOUT;
	}

	/* Configure the LCD in register CR */
	mask = GH08172T_CR_DUTY_MASK | GH08172T_CR_BIAS_MASK | GH08172T_CR_VSEL_MASK |
	       GH08172T_CR_MUX_SEG_MASK;
	data = init.duty | init.bias | init.voltage_source | init.mux_segment;
	current_val = sys_read32(config->lcd.cr);
	new_val = (current_val & mask) | data;
	sys_write32(new_val, config->lcd.cr);

	gh08172t_enable(dev);

	/* Wait Until the LCD is enabled */
	GH08172T_WAIT_FOR(success, (sys_read32(config->lcd.sr) & GH08172T_SR_ENS_MASK) != 0,
			  config->lcd_timeout_us, k_msleep(1));
	if (!success) {
		LOG_ERR("Display init timeout");
		return -ETIMEDOUT;
	}

	/* Wait Until the LCD Booster is ready */
	GH08172T_WAIT_FOR(success, (sys_read32(config->lcd.sr) & GH08172T_SR_RDY_MASK) != 0,
			  config->lcd_timeout_us, k_msleep(1));
	if (!success) {
		LOG_ERR("Display init timeout");
		return -ETIMEDOUT;
	}

	driver_data->character_x = 0;
	driver_data->character_y = 0;

	LOG_DBG("GH08172T driver initialized successfully with %d digits",
		config->capabilities.columns);
	return 0;
}

/* Advanced Devicetree generation macro mapping compile-time definitions directly to C parameters */
#define AUXDISPLAY_GH08172T_DEVICE(inst)                                                           \
	static struct auxdisplay_gh08172t_data auxdisplay_gh08172t_data_##inst;                    \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
	static const struct stm32_pclken auxdisplay_gh08172t_pclken_##inst[] = {                   \
		STM32_DT_INST_CLOCK_INFO_BY_IDX(inst, 0)};                                         \
	static const struct auxdisplay_gh08172t_config auxdisplay_gh08172t_config_##inst = {       \
		.capabilities =                                                                    \
			{                                                                          \
				.columns = DT_INST_PROP(inst, columns),                            \
				.rows = DT_INST_PROP(inst, rows),                                  \
				.mode = 0,                                                         \
				.brightness.minimum = AUXDISPLAY_LIGHT_NOT_SUPPORTED,              \
				.brightness.maximum = AUXDISPLAY_LIGHT_NOT_SUPPORTED,              \
				.backlight.minimum = AUXDISPLAY_LIGHT_NOT_SUPPORTED,               \
				.backlight.maximum = AUXDISPLAY_LIGHT_NOT_SUPPORTED,               \
				.custom_characters = 0,                                            \
				.custom_character_width = 0,                                       \
				.custom_character_height = 0,                                      \
			},                                                                         \
		.lcd.cr = (mem_addr_t)(((unsigned char *)DT_INST_REG_ADDR(inst)) +                 \
				       GH08172T_CR_OFFSET),                                        \
		.lcd.fcr = (mem_addr_t)(((unsigned char *)DT_INST_REG_ADDR(inst)) +                \
					GH08172T_FCR_OFFSET),                                      \
		.lcd.sr = (mem_addr_t)(((unsigned char *)DT_INST_REG_ADDR(inst)) +                 \
				       GH08172T_SR_OFFSET),                                        \
		.lcd.clr = (mem_addr_t)(((unsigned char *)DT_INST_REG_ADDR(inst)) +                \
					GH08172T_CLR_OFFSET),                                      \
		.lcd.ram = (uint32_t *)(((unsigned char *)DT_INST_REG_ADDR(inst)) +                \
					GH08172T_RAM_OFFSET),                                      \
		.pclken = auxdisplay_gh08172t_pclken_##inst,                                       \
		.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_IDX(inst, 0)),                     \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                      \
		.lcd_timeout_us = 1000 * DT_INST_PROP(inst, lcd_timeout_ms),                       \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(                                                                     \
		inst, auxdisplay_gh08172t_init, NULL, &auxdisplay_gh08172t_data_##inst,            \
		&auxdisplay_gh08172t_config_##inst, POST_KERNEL, CONFIG_AUXDISPLAY_INIT_PRIORITY,  \
		&auxdisplay_gh08172t_auxdisplay_api)

/* Parse the active devicetree layout and execute the instantiation macro for matching okay
 * targets.
 */
DT_INST_FOREACH_STATUS_OKAY(AUXDISPLAY_GH08172T_DEVICE)
