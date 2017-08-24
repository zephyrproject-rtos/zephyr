/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
extern void test_mbox_kinit(void);
extern void test_mbox_kdefine(void);
extern void test_mbox_put_get_null(void);
extern void test_mbox_put_get_buffer(void);
extern void test_mbox_async_put_get_buffer(void);
extern void test_mbox_async_put_get_block(void);
extern void test_mbox_target_source_thread_buffer(void);
extern void test_mbox_target_source_thread_block(void);

/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_mbox_api,
			 ztest_unit_test(test_mbox_kinit),/*keep init first!*/
			 ztest_unit_test(test_mbox_kdefine),
			 ztest_unit_test(test_mbox_put_get_null),
			 ztest_unit_test(test_mbox_put_get_buffer),
			 ztest_unit_test(test_mbox_async_put_get_buffer),
			 ztest_unit_test(test_mbox_async_put_get_block),
			 ztest_unit_test(test_mbox_target_source_thread_buffer),
			 ztest_unit_test(test_mbox_target_source_thread_block));
	ztest_run_test_suite(test_mbox_api);
}
