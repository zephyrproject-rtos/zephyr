/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_kernel_sprintf
 * @{
 * @defgroup t_sprintf_api test_sorintf_fn
 * @}
 */

#include <ztest.h>
extern void sprintf_double_test(void);
extern void vsnprintf_test(void);
extern void vsprintf_test(void);
extern void snprintf_test(void);
extern void sprintf_misc_test(void);
extern void sprintf_integer_test(void);
extern void sprintf_stringtest(void);

/*test case main entry*/
void test_main(void *p1, void *p2, void *p3)
{
	ztest_test_suite(test_sprintf_fn,
		ztest_unit_test(sprintf_double_test),
		ztest_unit_test(vsnprintf_test),
		ztest_unit_test(vsprintf_test),
		ztest_unit_test(snprintf_test),
		ztest_unit_test(sprintf_misc_test),
		ztest_unit_test(sprintf_integer_test),
		ztest_unit_test(sprintf_stringtest));

	ztest_run_test_suite(test_sprintf_fn);
}
