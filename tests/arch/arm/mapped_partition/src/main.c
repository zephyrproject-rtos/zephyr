/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/ztest.h>

#define TEST_PARTITION DT_CHOSEN(zephyr_code_partition)
#define TEST_EXPECTED  DT_PATH(zephyr_user)

ZTEST(mapped_partition, test_storage_region)
{
	zassert_equal(DT_MAPPED_PARTITION_OFFSET(TEST_PARTITION),
		      DT_PROP(TEST_EXPECTED, expected_storage_offset));
	zassert_equal(DT_REG_SIZE(TEST_PARTITION), DT_PROP(TEST_EXPECTED, expected_storage_size));
}

ZTEST_SUITE(mapped_partition, NULL, NULL, NULL, NULL, NULL);
