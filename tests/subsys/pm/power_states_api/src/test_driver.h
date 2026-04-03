/*
 * Copyright (c) 2024 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TESTS_SUBSYS_PM_POWER_STATES_API_TEST_DRIVER_H_
#define TESTS_SUBSYS_PM_POWER_STATES_API_TEST_DRIVER_H_

#include <zephyr/device.h>

/**
 * @brief Async operation.
 *
 * The device simulates an async operation and shall
 * not be suspended while is in progress. This included
 * the SoC not transition to certain power states.
 *
 * @param dev Device instance.
 */
void test_driver_async_operation(const struct device *dev);


#endif /* TESTS_SUBSYS_PM_POWER_STATES_API_TEST_DRIVER_H_ */
