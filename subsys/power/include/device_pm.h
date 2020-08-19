/*
 * Copyright (c) 2020 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _DEVICE_PM_H_
#define _DEVICE_PM_H_

#include <device.h>
#include <power/power_state.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DEVICE_FOREACH(dev) \
	struct device *_devices, *dev; \
	for (size_t _index = 0, \
		_len = z_device_get_all_static(&_devices); \
	     _index < _len && dev = _devices[_index]; _index++)

#define DEVICE_FOREACH_REVERSE(dev) \
	struct device *_devices, *dev; \
	for (size_t _index = \
		z_device_get_all_static(&_devices) - 1; \
	     _index >= 0 && dev = _devices[_index]; _index--)

/**
 * @brief Suspend all the devices in system.
 *
 * Try to suspend all the devices in the system with
 * dependency order. If anyone's suspend failed, the
 * suspended devices will be resumed.
 *
 * @param state Device pm state.
 * @param arg Pointer to device pm state argument.
 *
 * @retval 0 If successful.
 * @retval Errno Negative errno code if failure.
 */
int device_pm_suspend_devices(device_pm_t state, void *arg);

/**
 * @brief Resume all the devices in system.
 *
 * Resume all the devices which are suspended previously
 * in the system with dependency order.
 */
void device_pm_resume_devices(void);

/**
 * @brief Suspend the given device.
 *
 * @param dev Device instance.
 * @param state Device pm state.
 * @param arg Pointer to device pm state argument.
 *
 * @retval 0 If successful.
 * @retval Errno Negative errno code if failure.
 */
int device_pm_suspend_device(struct device *dev,
			     device_pm_t state, void *arg);

/**
 * @brief Resume the given device.
 *
 * Resume the given device which are suspended previously.
 *
 * @param dev Device instance.
 */
void device_pm_resume_device(struct device *dev);

#ifdef __cplusplus
}
#endif

#endif
