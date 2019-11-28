/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <misc/printk.h>
#include <gpio.h>
#include <soc.h>
#include <kscan.h>

#define LOG_LEVEL LOG_LEVEL_DBG
#include <logging/log.h>

LOG_MODULE_REGISTER(main);

struct device *kscan_dev;
static struct k_timer typematic_timer;
static struct k_timer block_matrix_timer;

#define KEY_RSVD 0U
#define MAX_MATRIX_KEY_COLS 16
#define MAX_MATRIX_KEY_ROWS 8

/****************************************************************************/
/*  Fujitsu keyboard model N860-7401-TOO1                                   */
/*                                                                          */
/*  Sense7  Sense6  Sense5  Sense4  Sense3  Sense2  Sense1  Sense0          */
/*+---------------------------------------------------------------+         */
/*|       | Capslk|       |   1!  |  Tab  |   F1  |   `~  |       | Scan  0 */
/*|       |  (30) |       |  (2)  |  (16) | (112) |  (1)  |       | (KEY #) */
/*|-------+-------+-------+-------+-------+-------+-------+-------|         */
/*|       |   W   |   Q   |   F7  |       |  Esc  |   F6  |   F5  | Scan  1 */
/*|       |  (18) |  (17) | (118) |       | (110) | (117) | (116) | (KEY #) */
/*|-------+-------+-------+-------+-------+-------+-------+-------|         */
/*|       |   F8  |       |   2@  |   F4  |   F3  |       |   F2  | Scan  2 */
/*|       | (119) |       |  (3)  | (115) | (114) |       | (113) | (KEY #) */
/*|-------+-------+-------+-------+-------+-------+-------+-------|         */
/*|       |   R   |   E   |   3#  |   4$  |   C   |   F   |   V   | Scan  3 */
/*|       |  (20) |  (19) |  (4)  |  (5)  |  (48) |  (34) |  (49) | (KEY #) */
/*|-------+-------+-------+-------+-------+-------+-------+-------|         */
/*|   N   |   Y   |   6^  |   5%  |   B   |   T   |   H   |   G   | Scan  4 */
/*|  (51) |  (22) |  (7)  |  (6)  | (50)  |  (21) |  (36) |  (35) | (KEY #) */
/*|-------+-------+-------+-------+-------+-------+-------+-------|         */
/*| SpaceB|   M   |   A   |   7&  |   J   |   D   |   S   |   U   | Scan  5 */
/*|  (61) |  (52) |  (31) |  (8)  |  (37) |  (33) |  (32) |  (23) | (KEY #) */
/*|-------+-------+-------+-------+-------+-------+-------+-------|         */
/*|   F9  |   I   |   ,<  |   8*  |       |   Z   |   X   |   K   | Scan  6 */
/*| (120) |  (24) |  (53) |  (9)  |       |  (46) |  (47) |  (38) | (KEY #) */
/*|-------+-------+-------+-------+-------+-------+-------+-------|         */
/*|       |   =+  |   ]}  |   9(  |   L   |       |  CRSL |   O   | Scan  7 */
/*|       |  (13) |  (28) |  (10) |  (39) |       |  (79) |  (25) | (KEY #) */
/*|-------+-------+-------+-------+-------+-------+-------+-------|         */
/*|   -_  |   0)  |   /?  |   [{  |   ;:  |       |       |   '"  | Scan  8 */
/*|  (12) |  (11) |  (55) |  (27) |  (40) |       |       |  (41) | (KEY #) */
/*|-------+-------+-------+-------+-------+-------+-------+-------|         */
/*|  F10  | Pause | NumLK |   P   |       |       |       |       | Scan  9 */
/*| (121) | (126) |  (90) |  (26) |       |       |       |       | (KEY #) */
/*|-------+-------+-------+-------+-------+-------+-------+-------|         */
/*| BkSpac|   \|  |  F11  |   .>  |       |       | W-Appl|  CRSD | Scan 10 */
/*|  (15) |  (29) | (122) |  (54) |       |       |  (71) |  (84) | (KEY #) */
/*|-------+-------+-------+-------+-------+-------+-------+-------|         */
/*| Enter | Delete| Insert|  F12  |       |       |  CRSU |  CRSR | Scan 11 */
/*|  (43) |  (76) |  (75) | (123) |       |       |  (83) |  (89) | (KEY #) */
/*|-------+-------+-------+-------+-------+-------+-------+-------|         */
/*|       | R-WIN |       |       |       |       | L-WIN |   Fn  | Scan 12 */
/*|       |  (87) |       |       |       |       |  (59) | (255) | (KEY #) */
/*|-------+-------+-------+-------+-------+-------+-------+-------|         */
/*|       |       |       | RShift| LShift|       |       |       | Scan 13 */
/*|       |       |       |  (57) |  (44) |       |       |       | (KEY #) */
/*|-------+-------+-------+-------+-------+-------+-------+-------|         */
/*|       |       |       |       |       |       | L Alt | R Alt | Scan 14 */
/*|       |       |       |       |       |       |  (60) |  (62) | (KEY #) */
/*|-------+-------+-------+-------+-------+-------+-------+-------|         */
/*| R Ctrl|       |       |       |       | L Ctrl|       |       | Scan 15 */
/*|  (64) |       |       |       |       |  (58) |       |       | (KEY #) */
/*+---------------------------------------------------------------+         */
/*                                                                          */
/****************************************************************************/

static const u8_t keymap[MAX_MATRIX_KEY_COLS][MAX_MATRIX_KEY_ROWS] = {
	{KEY_RSVD, 1, 112, 16, 2, KEY_RSVD, 30, KEY_RSVD},
	{116, 117, 110, KEY_RSVD, 118, 17, 18, KEY_RSVD},
	{113, KEY_RSVD, 114, 115, 3, KEY_RSVD, 119, KEY_RSVD},
	{49, 34, 48, 5, 4, 19, 20, KEY_RSVD},
	{35, 36, 21, 50, 6, 7, 22, 51},
	{23, 32, 33, 37, 8, 31, 52, 61},
	{38, 47, 46, KEY_RSVD, 9, 53, 24, 120},
	{25, 79, KEY_RSVD, 39, 10, 28, 13, KEY_RSVD},
	{41, KEY_RSVD, KEY_RSVD, 40, 27, 55, 11, 12},
	{KEY_RSVD, KEY_RSVD, KEY_RSVD, KEY_RSVD, 26, 90, 126, 121},
	{84, 71, KEY_RSVD, KEY_RSVD, 54, 122, 29, 15},
	{89, 83, KEY_RSVD, KEY_RSVD, 123, 75, 76, 43},
	{255, 59, KEY_RSVD, KEY_RSVD, KEY_RSVD, KEY_RSVD, 87, KEY_RSVD},
	{KEY_RSVD, KEY_RSVD, 44, 57, KEY_RSVD, KEY_RSVD, KEY_RSVD},
	{62, 60, KEY_RSVD, KEY_RSVD, KEY_RSVD, KEY_RSVD, KEY_RSVD, KEY_RSVD},
	{KEY_RSVD, KEY_RSVD, 58, KEY_RSVD, KEY_RSVD, KEY_RSVD, KEY_RSVD, 64},
};

/* Key used for typematic */
static u8_t last_key;

/* Typematic rate and delay values correspond to the data passed after
 * the F3 command (See the 8042 spec online)
 */
static const u16_t period[] = {
	33U,  37U,  42U,  46U,  50U,  54U,  58U,  63U,
	67U,  75U,  83U,  92U, 100U, 109U, 116U, 125U,
	133U, 149U, 167U, 182U, 200U, 217U, 232U, 250U,
	270U, 303U, 333U, 370U, 400U, 435U, 470U, 500U
};

static const u16_t delay[] = { 250U, 500U, 750U, 1000U };

static bool block_kb_matrix;

static void typematic_callback(struct k_timer *timer)
{
	LOG_INF("Typematic : %u\n", last_key);
}

static void kb_callback(struct device *dev, u8_t row, u8_t col, bool pressed)
{
	ARG_UNUSED(dev);
	last_key = keymap[col][row];

	LOG_INF("Key code = %u Pressed = %u\n", last_key, pressed);

	if (pressed) {
		k_timer_start(&typematic_timer,
			      K_MSEC(delay[0]), K_MSEC(period[0]));
	} else {
		k_timer_stop(&typematic_timer);
	}
}

static void block_matrix_callback(struct k_timer *timer)
{
	LOG_DBG("block host : %d\n", block_kb_matrix);
	if (block_kb_matrix) {
		kscan_disable_callback(kscan_dev);
		block_kb_matrix = false;
		k_timer_stop(&typematic_timer);
	} else {
		kscan_enable_callback(kscan_dev);
		block_kb_matrix = true;
	}
}

void main(void)
{
	printk("Kscan matrix sample application\n");

	kscan_dev = device_get_binding(DT_KSCAN_0_NAME);
	if (!kscan_dev) {
		LOG_ERR("kscan device %s not found", DT_KSCAN_0_NAME);
		return;
	}

	kscan_config(kscan_dev, kb_callback);
	k_timer_init(&typematic_timer, typematic_callback, NULL);
	k_timer_init(&block_matrix_timer, block_matrix_callback, NULL);
	k_timer_start(&block_matrix_timer, K_SECONDS(1), K_SECONDS(3));

}

