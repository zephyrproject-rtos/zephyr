/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2024 Intel Corporation
 *
 * Author: Tomasz Leman <tomasz.m.leman@intel.com>
 */

#ifndef __INTEL_PM_DATA_H__
#define __INTEL_PM_DATA_H__

#include <zephyr/sys/atomic.h>
#include <zephyr/sys/__assert.h>

/**
 * @brief Structure to represent Intel platform-specific power management data.
 *
 * This structure contains data used for customization of power state operations
 * on Intel platforms.
 */
struct intel_adsp_ace_pm_data {
	/**
	 * @brief Reference counter for lock of clock switching operation.
	 *
	 * This atomic variable ensures thread-safe increments and decrements
	 * of the lock count, which is critical for managing clock switching
	 * in a multi-core environment.
	 */
	atomic_t clock_switch_lock_refcount;
};

/**
 * @brief Increment the clock switch lock reference counter.
 *
 * This function atomically increments the reference counter for the clock switch lock. It is used
 * to prevent clock switching operations that change the current DSP clock when a core becomes
 * active. The function returns true if this is the first core to request the lock, indicating that
 * the clock switching operation was previously not locked.
 *
 * @param data Pointer to the intel_adsp_ace_pm_data struct containing the lock reference counter.
 * @return True if this is the first increment operation, false otherwise.
 */
static ALWAYS_INLINE bool intel_adsp_acquire_clock_switch_lock(struct intel_adsp_ace_pm_data *data)
{
	if (data)
		return atomic_inc(&data->clock_switch_lock_refcount) == 0;

	return false;
}

/**
 * @brief Decrement the clock switch lock reference counter.
 *
 * This function atomically decrements the reference counter for the clock switch lock. It allows
 * clock switching operations to proceed when the last active core no longer needs the clock to be
 * stable. The function returns true if this is the first core to request the lock, indicating that
 * the clock switching operation was previously not locked.
 *
 * @param data Pointer to the intel_adsp_ace_pm_data struct containing the lock reference counter.
 * @return True if this is the last decrement operation, false otherwise.
 */
static ALWAYS_INLINE bool intel_adsp_release_clock_switch_lock(struct intel_adsp_ace_pm_data *data)
{
	if (data) {
		atomic_t old_value = atomic_dec(&data->clock_switch_lock_refcount);

		__ASSERT(old_value > 0, "Unbalanced clock switching lock use!");
		return old_value == 1;
	}

	return false;
}

#endif /* __INTEL_PM_DATA_H__ */
