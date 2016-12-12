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
 * @addtogroup t_driver_aio
 * @{
 * @defgroup t_aio_basic_api test_aio_basic_api
 * @}
 *
 * Setup: loop PIN_OUT with PIN_IN on target board
 *
 * quark_se_c1000_devboard
 * ------------------------
 * 1. PIN_OUT is GPIO_17
 * 2. PIN_IN is AIN_15
 */

#include "test_aio.h"

void test_main(void)
{
	ztest_test_suite(aio_basic_api_test,
			 ztest_unit_test(test_aio_callback_rise),
			 ztest_unit_test(test_aio_callback_rise_disable));
	ztest_run_test_suite(aio_basic_api_test);
}
