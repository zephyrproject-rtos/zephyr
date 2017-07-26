/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <ztest.h>
extern void test_ipv6_multi_frags(void);
extern void test_fragment_copy(void);
extern void test_pkt_read_append(void);
extern void test_pkt_read_write_insert(void);
extern void test_fragment_compact(void);
extern void test_fragment_split(void);
void test_main(void)
{
	ztest_test_suite(net_pkt_tests,
			 ztest_unit_test(test_ipv6_multi_frags),
			 ztest_unit_test(test_fragment_copy),
			 ztest_unit_test(test_pkt_read_append),
			 ztest_unit_test(test_pkt_read_write_insert),
			 ztest_unit_test(test_fragment_compact),
			 ztest_unit_test(test_fragment_split)
			 );

	ztest_run_test_suite(net_pkt_tests);
}
