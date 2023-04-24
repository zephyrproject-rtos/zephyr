/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <pthread.h>

extern void test_posix_clock(void);
extern void test_posix_mqueue(void);
extern void test_posix_normal_mutex(void);
extern void test_posix_recursive_mutex(void);
extern void test_posix_semaphore(void);
extern void test_posix_rw_lock(void);
extern void test_posix_realtime(void);
extern void test_posix_timer(void);
extern void test_posix_pthread_execution(void);
extern void test_posix_pthread_error_condition(void);
extern void test_posix_pthread_create_negative(void);
extern void test_posix_pthread_termination(void);
extern void test_posix_multiple_threads_single_key(void);
extern void test_posix_single_thread_multiple_keys(void);
extern void test_pthread_descriptor_leak(void);
extern void test_nanosleep_NULL_NULL(void);
extern void test_nanosleep_NULL_notNULL(void);
extern void test_nanosleep_notNULL_NULL(void);
extern void test_nanosleep_notNULL_notNULL(void);
extern void test_nanosleep_req_is_rem(void);
extern void test_nanosleep_n1_0(void);
extern void test_nanosleep_0_n1(void);
extern void test_nanosleep_n1_n1(void);
extern void test_nanosleep_0_1(void);
extern void test_nanosleep_0_1001(void);
extern void test_nanosleep_0_1000000000(void);
extern void test_nanosleep_0_500000000(void);
extern void test_nanosleep_1_0(void);
extern void test_nanosleep_1_1(void);
extern void test_nanosleep_1_1001(void);
extern void test_sleep(void);
extern void test_usleep(void);
extern void test_sched_policy(void);

void test_main(void)
{
	ztest_test_suite(posix_apis,
			ztest_unit_test(test_posix_pthread_execution),
			ztest_unit_test(test_posix_pthread_error_condition),
			ztest_unit_test(test_posix_pthread_termination),
			ztest_unit_test(test_posix_multiple_threads_single_key),
			ztest_unit_test(test_posix_single_thread_multiple_keys),
			ztest_unit_test(test_pthread_descriptor_leak),
			ztest_unit_test(test_posix_clock),
			ztest_unit_test(test_posix_semaphore),
			ztest_unit_test(test_posix_normal_mutex),
			ztest_unit_test(test_posix_recursive_mutex),
			ztest_unit_test(test_posix_mqueue),
			ztest_unit_test(test_posix_realtime),
			ztest_unit_test(test_posix_timer),
			ztest_unit_test(test_posix_rw_lock),
			ztest_unit_test(test_nanosleep_NULL_NULL),
			ztest_unit_test(test_nanosleep_NULL_notNULL),
			ztest_unit_test(test_nanosleep_notNULL_NULL),
			ztest_unit_test(test_nanosleep_notNULL_notNULL),
			ztest_unit_test(test_nanosleep_req_is_rem),
			ztest_unit_test(test_nanosleep_n1_0),
			ztest_unit_test(test_nanosleep_0_n1),
			ztest_unit_test(test_nanosleep_n1_n1),
			ztest_unit_test(test_nanosleep_0_1),
			ztest_unit_test(test_nanosleep_0_1001),
			ztest_unit_test(test_nanosleep_0_500000000),
			ztest_unit_test(test_nanosleep_1_0),
			ztest_unit_test(test_nanosleep_1_1),
			ztest_unit_test(test_nanosleep_1_1001),
			ztest_unit_test(test_posix_pthread_create_negative),
			ztest_unit_test(test_sleep),
			ztest_unit_test(test_usleep),
			ztest_unit_test(test_sched_policy)
			);
	ztest_run_test_suite(posix_apis);
}
