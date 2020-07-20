/*
 * Copyright (c) 2020 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_POWER_RT_DPM_H
#define ZEPHYR_INCLUDE_POWER_RT_DPM_H

#include <device.h>
#include <sys/atomic.h>
#include <kernel_structs.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Callbacks defined for device runtime power management.
 *
 * The post_resume/pre_suspend callbacks are used to do stuff without
 * irq locked (e.g. states sync between driver and firmware). Device
 * driver should take care all the possible errors during this irq
 * unlocked period and the result will be indicated by the return value.
 *
 * The resume/suspend callbacks will do the real power/clock involved
 * stuff with irq locked. The specific state devices will go completely
 * depends on every device, resume/suspend is generic definition here.
 *
 * @retval 0 if successfully excute post_resume/pre_suspend.
 * @retval -EIO if error happens during irq unlocked stage.
 */
struct rt_dpm_ops {
	void (*resume)(struct device *dev);
	void (*suspend)(struct device *dev);
	int (*post_resume)(struct device *dev);
	int (*pre_suspend)(struct device *dev);
};

/**
 * @brief Structure used to do device runtime power management.
 */
struct rt_dpm {
	/* device runtime pm state */
	int state;

	/* wait queue on which threads are pended
	 * because of the possible concurrency.
	 */
	_wait_q_t wait_q;

	/* work item used to do asynchronously state transition */
	struct k_work work;

	/* usage count of the device */
	atomic_t usage_count;

	struct k_spinlock lock;
	struct rt_dpm_ops *ops;

	/* protected by spinlock so don't use atomic */
	unsigned int disable_count;
};

/**
 * @brief Loop over parent for the given device.
 */
#define DEVICE_PARENT_FOREACH(dev, iterator) \
	for (struct device *iterator = dev; iterator != NULL; )

/**
 * @brief Initialize device runtime power management.
 *
 * Initialize device runtime power management for the given device.
 *
 * @param dev Pointer to the given device.
 * @param is_suspend Initial state of given device.
 */
void rt_dpm_init(struct device *dev, bool is_suspend);

/**
 * @brief Enable device runtime power management.
 *
 * Enable device runtime power management for the given device.
 *
 * @param dev Pointer to the given device.
 */
void rt_dpm_enable(struct device *dev);

/**
 * @brief Disable device runtime power management.
 *
 * Disable device runtime power management for the given device.
 *
 * @param dev Pointer to the given device.
 */
void rt_dpm_disable(struct device *dev);

/**
 * @brief Claim a device to mark the device as being used.
 *
 * Claim the given device to make sure any device operation protected
 * by the succeful claim is safe. Can't be used in ISRs.
 *
 * @param dev Pointer to the given device.
 *
 * @retval 0 if successfully claimed the given device.
 * @retval -EIO if error happens during device post resume.
 * @retval -EACCES if disabled device runtime power management.
 */
int rt_dpm_claim(struct device *dev);

/**
 * @brief Release the given device.
 *
 * Synchronously decrease usage count of the given device and suspend
 * that device if all the conditions satisfied, this function must be
 * called after any claim happens, forbid asymmetric release. Can't be
 * used in ISRs.
 *
 * @param dev Pointer to the given device.
 *
 * @retval 0 if successfully release the given device.
 * @retval -EIO if error happens during device suspend prepare.
 * @retval -EACCES if disabled device runtime power management.
 */
int rt_dpm_release(struct device *dev);

/**
 * @brief Release the given device asynchronously.
 *
 * Decrease usage count of the given device and submit suspend request
 * to work queue when usage count crossing 1 - 0 boundary. Can be used
 * in ISRs.
 *
 * @param dev Pointer to the given device.
 *
 * @retval 0 if successfully release the given device.
 * @retval 1 if successfully submit the release request.
 */
int rt_dpm_release_async(struct device *dev);

/**
 * @brief Whether given device is in active state.
 *
 * @param dev Pointer to the given device.
 *
 * @retval True if given device is in active state.
 * @retval False if given device isn't in active state.
 */
bool rt_dpm_is_active_state(struct device *dev);

/**
 * @brief Whether given device is in suspended state.
 *
 * @param dev Pointer to the given device.
 *
 * @retval True if given device is in suspended state.
 * @retval False if given device isn't in suspended state.
 */
bool rt_dpm_is_suspend_state(struct device *dev);

#ifdef __cplusplus
}
#endif

#endif
