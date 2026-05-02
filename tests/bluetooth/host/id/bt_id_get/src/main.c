/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "testing_common_defs.h"

#include <zephyr/bluetooth/hci.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>

#include <host/hci_core.h>
#include <host/id.h>

DEFINE_FFF_GLOBALS;

/* This LUT contains different testing addresses. */
static const bt_addr_le_t *testing_addr_lut[] = {BT_LE_ADDR, BT_STATIC_RANDOM_LE_ADDR_1,
						 BT_STATIC_RANDOM_LE_ADDR_2, BT_RPA_LE_ADDR};

/* This is the destination address used during UTs */
static bt_addr_le_t copy_dst_addrs[CONFIG_BT_ID_MAX];

BUILD_ASSERT(ARRAY_SIZE(testing_addr_lut) == CONFIG_BT_ID_MAX);

static void fff_reset_rule_before(const struct ztest_unit_test *test, void *fixture)
{
	memset(&bt_dev, 0x00, sizeof(struct bt_dev));
	memset(copy_dst_addrs, 0x00, sizeof(copy_dst_addrs));

	for (size_t i = 0; i < CONFIG_BT_ID_MAX; i++) {
		const bt_addr_le_t *src = testing_addr_lut[i];
		bt_addr_le_t *dst = &bt_dev.id_addr[bt_dev.id_count++];

		memcpy(dst, src, sizeof(bt_addr_le_t));
	}

	for (size_t i = 0; i < CONFIG_BT_ID_MAX; i++) {
		memcpy(&copy_dst_addrs[i], BT_ADDR_LE_ANY, sizeof(bt_addr_le_t));
	}
}

ZTEST_RULE(fff_reset_rule, fff_reset_rule_before, NULL);

ZTEST_SUITE(bt_id_get, NULL, NULL, NULL, NULL, NULL);

/*
 *  Get currently stored ID count
 *
 *  Constraints:
 *   - Use NULL value for the address
 *
 *  Expected behaviour:
 *   - Count parameter pointer is dereferenced and loaded with the current bt_dev.id_count
 */
ZTEST(bt_id_get, test_get_current_id_count)
{
	size_t count;

	bt_id_get(NULL, &count);

	zassert_equal(count, CONFIG_BT_ID_MAX, "Incorrect ID count %d was returned", count);
}

/*
 *  Copy minimum number of addresses to the destination array
 *
 *  Constraints:
 *   - Destination array is initially cleared
 *
 *  Expected behaviour:
 *   - Count parameter pointer is dereferenced and loaded with actual number of copied items
 */
ZTEST(bt_id_get, test_copy_minimum_count)
{
	size_t stored_count = bt_dev.id_count;
	size_t testing_counts[] = {0, 1, bt_dev.id_count, bt_dev.id_count + 2};

	for (size_t it = 0; it < ARRAY_SIZE(testing_counts); it++) {
		size_t count = testing_counts[it];
		size_t expected_count = MIN(count, stored_count);

		bt_id_get(copy_dst_addrs, &count);

		zassert_equal(count, expected_count, "Incorrect ID count %d was returned", count);

		/* Verify copied items */
		for (size_t i = 0; i < count; i++) {
			const bt_addr_le_t *src = testing_addr_lut[i];
			bt_addr_le_t *dst = &copy_dst_addrs[i];

			zassert_mem_equal(src, dst, sizeof(bt_addr_le_t),
					  "Incorrect address was set");
		}

		/* Verify the rest of items */
		for (size_t i = count; i < stored_count; i++) {
			bt_addr_le_t *src = &copy_dst_addrs[i];

			zassert_mem_equal(src, BT_ADDR_LE_ANY, sizeof(bt_addr_le_t),
					  "Incorrect address was set");
		}
	}
}
