/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_threads_lifecycle
 * @{
 * @defgroup t_threads_lifecycle_api test_threads_lifecycle_api
 * @brief TestPurpose: verify zephyr basic threads lifecycle apis
 * @}
 */

#include <ztest.h>
extern void test_threads_spawn_params(void);
extern void test_threads_spawn_priority(void);
extern void test_threads_spawn_delay(void);
extern void test_threads_suspend_resume_cooperative(void);
extern void test_threads_suspend_resume_preemptible(void);
extern void test_threads_cancel_undelayed(void);
extern void test_threads_cancel_delayed(void);
extern void test_threads_cancel_started(void);
extern void test_threads_abort_self(void);
extern void test_threads_abort_others(void);

/*test case main entry*/
void test_main(void *p1, void *p2, void *p3)
{
	ztest_test_suite(test_threads_lifecycle,
			 ztest_unit_test(test_threads_spawn_params),
			 ztest_unit_test(test_threads_spawn_priority),
			 ztest_unit_test(test_threads_spawn_delay),
			 ztest_unit_test(test_threads_suspend_resume_cooperative),
			 ztest_unit_test(test_threads_suspend_resume_preemptible),
			 ztest_unit_test(test_threads_cancel_undelayed),
			 ztest_unit_test(test_threads_cancel_delayed),
			 ztest_unit_test(test_threads_cancel_started),
			 ztest_unit_test(test_threads_abort_self),
			 ztest_unit_test(test_threads_abort_others)
			 );
	ztest_run_test_suite(test_threads_lifecycle);
}
