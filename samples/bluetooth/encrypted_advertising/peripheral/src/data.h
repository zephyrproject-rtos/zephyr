/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/kernel.h>

#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/bluetooth.h>

#include "common.h"

#define LF_ID_MSB ((BT_COMP_ID_LF >> 8) & 0xff)
#define LF_ID_LSB ((BT_COMP_ID_LF) & 0xff)

#define AD_DATA_0_SIZE 10
static uint8_t ad_data_0[] = {'E', 'A', 'D', ' ', 'S', 'a', 'm', 'p', 'l', 'e'};
BUILD_ASSERT(sizeof(ad_data_0) == AD_DATA_0_SIZE);

#define AD_DATA_1_SIZE 8
static uint8_t ad_data_1[] = {LF_ID_MSB, LF_ID_LSB, 'Z', 'e', 'p', 'h', 'y', 'r'};
BUILD_ASSERT(sizeof(ad_data_1) == AD_DATA_1_SIZE);

#define AD_DATA_2_SIZE 7
static uint8_t ad_data_2[] = {LF_ID_MSB, LF_ID_LSB, 0x49, 0xd2, 0xf4, 0x55, 0x76};
BUILD_ASSERT(sizeof(ad_data_2) == AD_DATA_2_SIZE);

#define AD_DATA_3_SIZE 4
static uint8_t ad_data_3[] = {LF_ID_MSB, LF_ID_LSB, 0xc1, 0x25};
BUILD_ASSERT(sizeof(ad_data_3) == AD_DATA_3_SIZE);

#define AD_DATA_4_SIZE 3
static uint8_t ad_data_4[] = {LF_ID_MSB, LF_ID_LSB, 0x17};
BUILD_ASSERT(sizeof(ad_data_4) == AD_DATA_4_SIZE);

static const struct bt_data ad[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, ad_data_0, sizeof(ad_data_0)),
	BT_DATA(BT_DATA_MANUFACTURER_DATA, ad_data_1, sizeof(ad_data_1)),
	BT_DATA(BT_DATA_MANUFACTURER_DATA, ad_data_2, sizeof(ad_data_2)),
	BT_DATA(BT_DATA_MANUFACTURER_DATA, ad_data_3, sizeof(ad_data_3)),
	BT_DATA(BT_DATA_MANUFACTURER_DATA, ad_data_4, sizeof(ad_data_4)),
};

static struct key_material mk = {
	.session_key = {0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB,
			0xCC, 0xCD, 0xCE, 0xCF},
	.iv = {0xFB, 0x56, 0xE1, 0xDA, 0xDC, 0x7E, 0xAD, 0xF5},
};
