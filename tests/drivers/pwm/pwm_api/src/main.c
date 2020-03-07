/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_driver_pwm
 * @{
 * @defgroup t_pwm_basic test_pwm_basic_operations
 * @}
 */

#include <zephyr.h>
#include <ztest.h>

void test_pwm_usec(void);
void test_pwm_cycle(void);
void test_pwm_nsec(void);

void test_main(void)
{
	ztest_test_suite(pwm_basic_test,
			 ztest_unit_test(test_pwm_usec),
			 ztest_unit_test(test_pwm_nsec),
			 ztest_unit_test(test_pwm_cycle));
	ztest_run_test_suite(pwm_basic_test);
}
