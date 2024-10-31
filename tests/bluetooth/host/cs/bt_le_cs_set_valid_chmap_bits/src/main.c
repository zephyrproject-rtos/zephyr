/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/cs.h>
#include <zephyr/fff.h>

DEFINE_FFF_GLOBALS;

ZTEST_SUITE(bt_le_cs_set_valid_chmap_bits, NULL, NULL, NULL, NULL, NULL);

/*
 *  Test uninitialized chmap buffer is populated correctly
 *
 *  Expected behaviour:
 *   - test_chmap matches correct_chmap
 */
ZTEST(bt_le_cs_set_valid_chmap_bits, test_uninitialized_chmap)
{
	uint8_t test_chmap[10];

	bt_le_cs_set_valid_chmap_bits(test_chmap);

	uint8_t correct_chmap[10] = {0xFC, 0xFF, 0x7F, 0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x1F};

	zassert_mem_equal(test_chmap, correct_chmap, 10);
}
