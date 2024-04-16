/*
 * Copyright 2023 Yonatan Schachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/bindesc.h>
#include <version.h>

#define STR_ID 1
#define UINT_ID 2
#define BYTES_ID 3

#define STR_DATA "Hello world!"
#define UINT_DATA 5
#define BYTES_DATA {1, 2, 3, 4}

BINDESC_STR_DEFINE(bindesc_string, STR_ID, STR_DATA);
BINDESC_UINT_DEFINE(bindesc_uint, UINT_ID, UINT_DATA);
BINDESC_BYTES_DEFINE(bindesc_bytes, BYTES_ID, (BYTES_DATA));

ZTEST(bindesc_define, test_version_number)
{
	zassert_equal(BINDESC_GET_UINT(kernel_version_number), KERNEL_VERSION_NUMBER,
			"bindesc kernel version number is incorrect");
}

ZTEST(bindesc_define, test_custom_bindesc_str)
{
	zassert_equal(BINDESC_GET_SIZE(bindesc_string), sizeof(STR_DATA));
	zassert_mem_equal(BINDESC_GET_STR(bindesc_string), STR_DATA, sizeof(STR_DATA));
}

ZTEST(bindesc_define, test_custom_bindesc_uint)
{
	zassert_equal(BINDESC_GET_SIZE(bindesc_uint), 4);
	zassert_equal(BINDESC_GET_UINT(bindesc_uint), UINT_DATA);
}

ZTEST(bindesc_define, test_custom_bindesc_bytes)
{
	uint8_t expected_data[] = BYTES_DATA;

	zassert_equal(BINDESC_GET_SIZE(bindesc_bytes), 4);
	zassert_mem_equal(BINDESC_GET_STR(bindesc_bytes), expected_data, 4);
}

ZTEST_SUITE(bindesc_define, NULL, NULL, NULL, NULL, NULL);
