/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 * Copyright (c) 2026 NXP
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

/**
 * @brief Arm a one-shot timer whose expiry calls pm_device_runtime_get().
 *
 * The timer expiry handler calls pm_device_runtime_get() for the device and
 * records the result, emulating a device interrupt that acquires a runtime PM
 * reference.
 *
 * @param dev Emulated device instance.
 */
void emulated_pm_stress_isr_get_submit(const struct device *dev);

/**
 * @brief Wait for the emulated ISR get() and return its result.
 *
 * @param dev Emulated device instance.
 *
 * @retval 0 pm_device_runtime_get() succeeded in timer context.
 * @retval -errno pm_device_runtime_get() error from timer context.
 */
int emulated_pm_stress_isr_get_result(const struct device *dev);

/** Reset the maximum concurrent PM callback count. */
void emulated_pm_stress_callback_max_reset(const struct device *dev);

/** @return Maximum concurrent PM callbacks since the last reset. */
int emulated_pm_stress_callback_max_get(const struct device *dev);

#endif /* ZEPHYR_TESTS_SUBSYS_PM_DEVICE_RUNTIME_STRESS_EMULATED_PM_DEVICE_H_ */
