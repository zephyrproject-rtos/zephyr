/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ipm.h>

int _impl_ipm_send(struct device *ipmdev, int wait, u32_t id,
		   const void *data, int size)
{
	const struct ipm_driver_api *api = ipmdev->driver_api;

	return api->send(ipmdev, wait, id, data, size);
}

int _impl_ipm_max_data_size_get(struct device *ipmdev)
{
	const struct ipm_driver_api *api = ipmdev->driver_api;

	return api->max_data_size_get(ipmdev);
}

u32_t _impl_ipm_max_id_val_get(struct device *ipmdev)
{
	const struct ipm_driver_api *api = ipmdev->driver_api;

	return api->max_id_val_get(ipmdev);
}

int _impl_ipm_set_enabled(struct device *ipmdev, int enable)
{
	const struct ipm_driver_api *api = ipmdev->driver_api;

	return api->set_enabled(ipmdev, enable);
}
