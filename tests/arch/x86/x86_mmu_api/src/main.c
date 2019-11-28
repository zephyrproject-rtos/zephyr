/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
extern void test_buffer_rw_read(void);
extern void test_buffer_writeable_write(void);
extern void test_buffer_readable_read(void);
extern void test_buffer_readable_write(void);
extern void test_buffer_supervisor_rw(void);
extern void test_buffer_supervisor_w(void);
extern void test_buffer_user_rw_user(void);
extern void test_buffer_user_rw_supervisor(void);
extern void test_multi_page_buffer_user(void);
extern void test_multi_page_buffer_write_user(void);
extern void test_multi_page_buffer_read_user(void);
extern void test_multi_page_buffer_read(void);
extern void test_multi_pde_buffer_rw(void);
extern void test_multi_pde_buffer_writeable_write(void);
extern void test_multi_pde_buffer_readable_read(void);
extern void test_multi_pde_buffer_readable_write(void);
/*test case nain entry*/
void test_main(void)
{
	ztest_test_suite(userbuffer_validate,
	ztest_unit_test(test_buffer_rw_read),
	ztest_unit_test(test_buffer_writeable_write),
	ztest_unit_test(test_buffer_readable_read),
	ztest_unit_test(test_buffer_readable_write),
	ztest_unit_test(test_buffer_supervisor_rw),
	ztest_unit_test(test_buffer_supervisor_w),
	ztest_unit_test(test_buffer_user_rw_user),
	ztest_unit_test(test_buffer_user_rw_supervisor),
	ztest_unit_test(test_multi_page_buffer_user),
	ztest_unit_test(test_multi_page_buffer_write_user),
	ztest_unit_test(test_multi_page_buffer_read_user),
	ztest_unit_test(test_multi_page_buffer_read),
	ztest_unit_test(test_multi_pde_buffer_rw),
	ztest_unit_test(test_multi_pde_buffer_writeable_write),
	ztest_unit_test(test_multi_pde_buffer_readable_read),
	ztest_unit_test(test_multi_pde_buffer_readable_write));
	ztest_run_test_suite(userbuffer_validate);
}
