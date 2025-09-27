/*
 * Copyright (c) 2025, Victor Chavez (vchavezb@protonmail.com)
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/logging/log.h>
#include <CppUTest/CommandLineTestRunner.h>
#include <CppUTestExt/MockSupport.h>

LOG_MODULE_REGISTER(test_suite, CONFIG_LOG_DEFAULT_LEVEL);

TEST_GROUP(my_test_group) {
    void setup() final
	{
    }
    void teardown() final
	{
    }
};

TEST(my_test_group, test_1) {

}
