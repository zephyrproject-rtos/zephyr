/* test_xip_helper.c - XIP helper module */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
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

/*
DESCRIPTION
This module contains support code for the XIP regression
test.
 */

#include <zephyr.h>
#include "test.h"

/*
 * This array is deliberately defined outside of the scope of the main test
 * module to avoid optimization issues.
 */
uint32_t xip_array[XIP_TEST_ARRAY_SZ] = {
	TEST_VAL_1, TEST_VAL_2, TEST_VAL_3, TEST_VAL_4};
