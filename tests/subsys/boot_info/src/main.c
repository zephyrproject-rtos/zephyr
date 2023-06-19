/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/devicetree.h>
#include <zephyr/boot_info/boot_info.h>

#define BOOT_INFO DT_NODELABEL(boot_info)
#define BOOT_INFO_ALIAS DT_ALIAS(bi)

ZTEST_USER(boot_info_api, test_get_size)
{
	size_t bi_size = boot_info_get_size(BOOT_INFO);

	zassert_not_equal(bi_size, 0U, "Get size returned invalid value");
}

ZTEST_USER(boot_info_api, test_get_set)
{
	uint8_t wr[boot_info_get_size(BOOT_INFO)];
	uint8_t rd[boot_info_get_size(BOOT_INFO)];
	int rc = 0;

	memset(wr, 0, sizeof(wr));
	memset(rd, 0, sizeof(rd));

	rc = boot_info_get(BOOT_INFO, wr);
	zassert_equal(rc, 0, "boot_info_get returned [%d]", rc);

	memset(wr, 0xa, sizeof(wr));
	rc = boot_info_set(BOOT_INFO, wr);
	zassert_equal(rc, 0, "boot_info_set returned [%d]", rc);

	rc = boot_info_get(BOOT_INFO, rd);
	zassert_equal(rc, 0, "boot_info_get returned [%d]", rc);

	zassert_equal(memcmp(rd, wr, sizeof(wr)), 0, "data mismatch");
}

ZTEST_SUITE(boot_info_api, NULL, NULL, NULL, NULL, NULL);
