/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_IPM_H_
#error "Should only be included by zephyr/drivers/ipm.h"
#endif

#ifndef ZEPHYR_INCLUDE_DRIVERS_IPM_INTERNAL_IPM_IMPL_H_
#define ZEPHYR_INCLUDE_DRIVERS_IPM_INTERNAL_IPM_IMPL_H_

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int z_impl_ipm_send(const struct device *ipmdev, int wait,
				  uint32_t id,
				  const void *data, int size)
{
	const struct ipm_driver_api *api =
		(const struct ipm_driver_api *)ipmdev->api;

	return api->send(ipmdev, wait, id, data, size);
}

static inline int z_impl_ipm_max_data_size_get(const struct device *ipmdev)
{
	const struct ipm_driver_api *api =
		(const struct ipm_driver_api *)ipmdev->api;

	return api->max_data_size_get(ipmdev);
}

static inline uint32_t z_impl_ipm_max_id_val_get(const struct device *ipmdev)
{
	const struct ipm_driver_api *api =
		(const struct ipm_driver_api *)ipmdev->api;

	return api->max_id_val_get(ipmdev);
}

static inline int z_impl_ipm_set_enabled(const struct device *ipmdev,
					 int enable)
{
	const struct ipm_driver_api *api =
		(const struct ipm_driver_api *)ipmdev->api;

	return api->set_enabled(ipmdev, enable);
}

static inline void z_impl_ipm_complete(const struct device *ipmdev)
{
#ifdef CONFIG_IPM_CALLBACK_ASYNC
	const struct ipm_driver_api *api =
		(const struct ipm_driver_api *)ipmdev->api;

	if (api->complete != NULL) {
		api->complete(ipmdev);
	}
#endif
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_IPM_INTERNAL_IPM_IMPL_H_ */
