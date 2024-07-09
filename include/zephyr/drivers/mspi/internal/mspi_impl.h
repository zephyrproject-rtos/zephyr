/*
 * Copyright (c) 2024, Ambiq Micro Inc. <www.ambiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Inline syscall implementation for MSPI driver APIs
 */

#ifndef ZEPHYR_INCLUDE_MSPI_H_
#error "Should only be included by zephyr/drivers/mspi.h"
#endif

#ifndef ZEPHYR_INCLUDE_DRIVERS_MSPI_INTERNAL_MSPI_IMPL_H_
#define ZEPHYR_INCLUDE_DRIVERS_MSPI_INTERNAL_MSPI_IMPL_H_

#include <errno.h>

#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int z_impl_mspi_config(const struct mspi_dt_spec *spec)
{
	const struct mspi_driver_api *api = (const struct mspi_driver_api *)spec->bus->api;

	return api->config(spec);
}

static inline int z_impl_mspi_dev_config(const struct device *controller,
					 const struct mspi_dev_id *dev_id,
					 const enum mspi_dev_cfg_mask param_mask,
					 const struct mspi_dev_cfg *cfg)
{
	const struct mspi_driver_api *api = (const struct mspi_driver_api *)controller->api;

	return api->dev_config(controller, dev_id, param_mask, cfg);
}

static inline int z_impl_mspi_get_channel_status(const struct device *controller, uint8_t ch)
{
	const struct mspi_driver_api *api = (const struct mspi_driver_api *)controller->api;

	return api->get_channel_status(controller, ch);
}

static inline int z_impl_mspi_transceive(const struct device *controller,
					 const struct mspi_dev_id *dev_id,
					 const struct mspi_xfer *req)
{
	const struct mspi_driver_api *api = (const struct mspi_driver_api *)controller->api;

	if (!api->transceive) {
		return -ENOTSUP;
	}

	return api->transceive(controller, dev_id, req);
}

static inline int z_impl_mspi_xip_config(const struct device *controller,
					 const struct mspi_dev_id *dev_id,
					 const struct mspi_xip_cfg *cfg)
{
	const struct mspi_driver_api *api = (const struct mspi_driver_api *)controller->api;

	if (!api->xip_config) {
		return -ENOTSUP;
	}

	return api->xip_config(controller, dev_id, cfg);
}

static inline int z_impl_mspi_scramble_config(const struct device *controller,
					      const struct mspi_dev_id *dev_id,
					      const struct mspi_scramble_cfg *cfg)
{
	const struct mspi_driver_api *api = (const struct mspi_driver_api *)controller->api;

	if (!api->scramble_config) {
		return -ENOTSUP;
	}

	return api->scramble_config(controller, dev_id, cfg);
}

static inline int z_impl_mspi_timing_config(const struct device *controller,
					    const struct mspi_dev_id *dev_id,
					    const uint32_t param_mask, void *cfg)
{
	const struct mspi_driver_api *api = (const struct mspi_driver_api *)controller->api;

	if (!api->timing_config) {
		return -ENOTSUP;
	}

	return api->timing_config(controller, dev_id, param_mask, cfg);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MSPI_INTERNAL_MSPI_IMPL_H_ */
