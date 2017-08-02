/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
extern void iface_setup(void);
extern void send_iface1(void);
extern void send_iface2(void);
extern void send_iface3(void);
extern void send_iface1_down(void);
extern void send_iface1_up(void);
void test_main(void)
{
	ztest_test_suite(net_iface_test,
			 ztest_unit_test(iface_setup),
			 ztest_unit_test(send_iface1),
			 ztest_unit_test(send_iface2),
			 ztest_unit_test(send_iface3),
			 ztest_unit_test(send_iface1_down),
			 ztest_unit_test(send_iface1_up)
			 );

	ztest_run_test_suite(net_iface_test);
}
