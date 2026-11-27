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

/**
 * @brief Release constraints acquired during device initialization.
 *
 * Called by suite setup after the device is ready. Releases the recorded
 * initialization locks and saves whether the constrained states are unlocked.
 *
 * @param dev Device instance.
 */
void test_driver_release_init_constraints(const struct device *dev);

/**
 * @brief Verify recorded initialization-time constraint handling.
 *
 * Checks that locks were acquired before the device was ready and released
 * by suite setup after initialization.
 *
 * @param dev Device instance.
 */
void test_driver_check_init_constraints(const struct device *dev);

#endif /* TESTS_SUBSYS_PM_POWER_STATES_API_TEST_DRIVER_H_ */
