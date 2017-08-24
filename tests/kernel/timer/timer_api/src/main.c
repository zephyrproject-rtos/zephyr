/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_kernel_timer
 * @{
 * @defgroup t_timer_api test_timer_api
 * @brief TestPurpose: verify timer api functionality
 * @}
 */

#include "test_timer.h"
#include <ztest.h>

/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_timer_api,
			 ztest_unit_test(test_timer_duration_period),
			 ztest_unit_test(test_timer_period_0),
			 ztest_unit_test(test_timer_expirefn_null),
			 ztest_unit_test(test_timer_status_get),
			 ztest_unit_test(test_timer_status_get_anytime),
			 ztest_unit_test(test_timer_status_sync),
			 ztest_unit_test(test_timer_k_define),
			 ztest_unit_test(test_timer_user_data));
	ztest_run_test_suite(test_timer_api);
}
