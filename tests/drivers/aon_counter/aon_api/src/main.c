/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_driver_aon
 * @{
 * @defgroup t_aon_basic_operations test_aon_basic_operations
 * @}
 */

#include "test_aon.h"

void test_main(void)
{
	ztest_test_suite(aon_basic_test,
			 ztest_unit_test(test_aon_counter),
			 ztest_unit_test(test_aon_periodic_timer));
	ztest_run_test_suite(aon_basic_test);
}
