/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_kernel_threads
 * @{
 * @defgroup t_systhreads test_systhreads
 * @brief TestPurpose: verify 2 system threads - main thread and idle thread
 * @}
 */

#include <ztest.h>
#include "test_systhreads.h"

/* test case main entry */
void test_main(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	test_systhreads_setup();
	ztest_test_suite(test_systhreads,
		ztest_unit_test(test_systhreads_main),
		ztest_unit_test(test_systhreads_idle));
	ztest_run_test_suite(test_systhreads);
}
