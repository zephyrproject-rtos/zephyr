/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <ztest.h>
extern void net_buf_test_1(void);
extern void net_buf_test_2(void);
extern void net_buf_test_3(void);
extern void net_buf_test_4(void);
extern void net_buf_test_big_buf(void);
extern void net_buf_test_multi_frags(void);

void test_main(void)
{
	ztest_test_suite(net_buf_test,
			 ztest_unit_test(net_buf_test_1),
			 ztest_unit_test(net_buf_test_2),
			 ztest_unit_test(net_buf_test_3),
			 ztest_unit_test(net_buf_test_4),
			 ztest_unit_test(net_buf_test_big_buf),
			 ztest_unit_test(net_buf_test_multi_frags)
			 );

	ztest_run_test_suite(net_buf_test);
}
