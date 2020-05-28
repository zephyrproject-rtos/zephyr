/** @file
 *  @brief uint48 tools
 */

/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include "uint48_util.h"

uint64_t uint48array_to_u64(uint8_t arr[])
{
	uint8_t  k;
	uint64_t val = 0;

	for (k = 0; k < UINT48_LEN; k++) {
		val += (uint64_t)arr[k] << k * 8;
	}
	return val;
}

void u64_to_uint48array(uint64_t val, uint8_t arr[])
{
	uint8_t  k;

	for (k = 0; k < UINT48_LEN; k++) {
		arr[k] = (uint8_t)((val >> k * 8) & 0xff);
	}
}

void u64_to_uint48array_str(uint64_t val, char str[])
{
	uint8_t t[UINT48_LEN];

	u64_to_uint48array(val, t);

	/* Assumes that UINT48_LEN is 6 */
	sprintf(str, "%02x%02x%02x%02x%02x%02x",
		t[5], t[4], t[3], t[2], t[1], t[0]);
}

void uint48array_str(uint8_t array[], char str[])
{
	/* Assumes that UINT48_LEN is 6 */
	sprintf(str, "%02x%02x%02x%02x%02x%02x", array[5], array[4], array[3],
		array[2], array[1], array[0]);
}
