/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TESTS_SUBSYS_PM_DEVICE_RUNTIME_TEST_DRIVER_H_
#define TESTS_SUBSYS_PM_DEVICE_RUNTIME_TEST_DRIVER_H_

#include <zephyr/device.h>

/**
 * @brief Put test driver in async test mode.
 *
 * In this mode the driver will not end PM action until signaled, thus
 * allowing to have control of the sequence.
 *
 * @param dev Device instance.
 */
void test_driver_pm_async(const struct device *dev);

/**
 * @brief Unblock test driver PM action.
 *
 * @param dev Device instance.
 */
void test_driver_pm_done(const struct device *dev);

/**
 * @brief Check if PM actions is ongoing.
 *
 * @param dev Device instance.
 *
 * @return true If PM action is ongoing.
 * @return false If PM action is not ongoing.
 */
bool test_driver_pm_ongoing(const struct device *dev);

#endif /* TESTS_SUBSYS_PM_DEVICE_RUNTIME_TEST_DRIVER_H_ */
