/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTS_SUBSYS_PM_DEVICE_RUNTIME_STRESS_EMULATED_PM_DEVICE_H_
#define ZEPHYR_TESTS_SUBSYS_PM_DEVICE_RUNTIME_STRESS_EMULATED_PM_DEVICE_H_

#include <zephyr/device.h>

/** @return Pointer to the emulated stress test device. */
const struct device *emulated_pm_stress_dev(void);

/**
 * @brief Start one emulated operation (k_timer emulates completion IRQ).
 *
 * Calls pm_device_runtime_get() for the device, then arms a one-shot timer.
 * The timer expiry calls pm_device_runtime_put_async().
 *
 * @param dev Emulated device instance.
 *
 * @retval 0 Success.
 * @retval -errno From pm_device_runtime_get().
 */
int emulated_pm_stress_submit(const struct device *dev);

/**
 * @brief Wait until the emulated operation completes.
 *
 * @param dev Emulated device instance.
 *
 * @retval 0 Timer path completed successfully (put_async returned 0).
 * @retval non-zero pm_device_runtime_put_async() error from timer context.
 */
int emulated_pm_stress_wait(const struct device *dev);

#endif /* ZEPHYR_TESTS_SUBSYS_PM_DEVICE_RUNTIME_STRESS_EMULATED_PM_DEVICE_H_ */
