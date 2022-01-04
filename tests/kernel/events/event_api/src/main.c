/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Tests for the events kernel object
 *
 * Verify zephyr event apis under different context
 *
 * - API coverage
 *   -# k_event_init K_EVENT_DEFINE
 *   -# k_event_post
 *   -# k_event_set
 *   -# k_event_wait
 *   -# k_event_wait_all
 *
 * @defgroup kernel_event_tests events
 * @ingroup all_tests
 * @{
 * @}
 */

#include <ztest.h>

extern void test_k_event_init(void);
extern void test_event_deliver(void);
extern void test_event_receive(void);

/*test case main entry*/

void test_main(void)
{
	ztest_test_suite(events_api,
			 ztest_1cpu_unit_test(test_k_event_init),
			 ztest_1cpu_unit_test(test_event_deliver),
			 ztest_1cpu_unit_test(test_event_receive));
	ztest_run_test_suite(events_api);
}
