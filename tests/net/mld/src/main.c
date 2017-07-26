/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <ztest.h>
extern void mld_setup(void);
extern void join_group(void);
extern void leave_group(void);
extern void catch_join_group(void);
extern void catch_leave_group(void);
extern void verify_join_group(void);
extern void verify_leave_group(void);
extern void catch_query(void);
extern void verify_send_report(void);
extern void test_allnodes(void);
extern void test_solicit_node(void);

void test_main(void)
{
	ztest_test_suite(net_mld_test,
			 ztest_unit_test(mld_setup),
			 ztest_unit_test(join_group),
			 ztest_unit_test(leave_group),
			 ztest_unit_test(catch_join_group),
			 ztest_unit_test(catch_leave_group),
			 ztest_unit_test(verify_join_group),
			 ztest_unit_test(verify_leave_group),
			 ztest_unit_test(catch_query),
			 ztest_unit_test(verify_send_report),
			 ztest_unit_test(test_allnodes),
			 ztest_unit_test(test_solicit_node)
			 );

	ztest_run_test_suite(net_mld_test);
}
