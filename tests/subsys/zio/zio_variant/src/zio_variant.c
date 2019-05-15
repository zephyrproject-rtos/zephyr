/*
 * Copyright (c) 2019 Thomas Burdick <thomas.burdick@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Test zio variant and macros
 *
 */


#include <tc_util.h>
#include <stdbool.h>
#include <zephyr.h>
#include <ztest.h>
#include <zio/zio_variant.h>

static void test_zio_variant_u32(void)
{
	u32_t myval = 5;
	struct zio_variant myvar = zio_variant_wrap(myval);

	zassert_equal(myvar.type, zio_variant_u32,
			"Unexpected in variant type");
	zassert_equal(myvar.u32_val, 5, "Unexpected value");
	u32_t myval2 = 0;
	int res = zio_variant_unwrap(myvar, myval2);

	zassert_equal(res, 0, "Unexpected result");
	zassert_equal(myval2, 5, "Unexpected value");
}

/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_zio_variant_list,
			 ztest_unit_test(test_zio_variant_u32)
			 );
	ztest_run_test_suite(test_zio_variant_list);
}
