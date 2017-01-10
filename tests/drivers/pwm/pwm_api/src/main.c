/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @addtogroup t_driver_pwm
 * @{
 * @defgroup t_pwm_basic test_pwm_basic_operations
 * @}
 */

#include <zephyr.h>
#include <ztest.h>

extern void test_pwm_usec(void);
extern void test_pwm_cycle(void);

void test_main(void)
{
	ztest_test_suite(pwm_basic_test,
			 ztest_unit_test(test_pwm_usec),
			 ztest_unit_test(test_pwm_cycle));
	ztest_run_test_suite(pwm_basic_test);
}
