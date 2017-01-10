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
 * @addtogroup t_driver_i2c
 * @{
 * @defgroup t_i2c_basic test_i2c_basic_operations
 * @}
 *
 * Setup: Connect Sensor GY271 to I2C_0 on X86 and I2C_SS_0 on ARC
 *
 * quark_se_c1000_devboard - x86
 * ----------------------
 *
 * 1. GY271_SCL - I2C0_SCL
 * 2. GY271_SDA - I2C0_SDA
 *
 * quark_se_c1000_ss_devboard - arc
 * ----------------------
 *
 * 1. GY271_SCL - I2C0_SS_SCL
 * 2. GY271_SDA - I2C0_SS_SDA
 *
 * quark_d2000 - x86
 * ----------------------
 *
 * 1. GY271_SCL - I2C_SCL
 * 2. GY271_SDA - I2C_SDA
 *
 * arduino_101 - x86
 * ----------------------
 *
 * 1. GY271_SCL - I2C0_SCL
 * 2. GY271_SDA - I2C0_SDA
 *
 * arduino_101_ss - arc
 * ----------------------
 *
 * 1. GY271_SCL - I2C0_SS_SCL
 * 2. GY271_SDA - I2C0_SS_SDA
 */

#include <zephyr.h>
#include <ztest.h>

extern void test_i2c_gy271(void);
extern void test_i2c_burst_gy271(void);

void test_main(void)
{
	ztest_test_suite(i2c_test,
			 ztest_unit_test(test_i2c_gy271),
			 ztest_unit_test(test_i2c_burst_gy271));
	ztest_run_test_suite(i2c_test);
}
