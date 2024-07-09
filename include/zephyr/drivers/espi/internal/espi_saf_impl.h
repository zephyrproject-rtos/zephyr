/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Inline syscall implementation for eSPI driver APIs
 */

#ifndef ZEPHYR_INCLUDE_ESPI_SAF_H_
#error "Should only be included by zephyr/drivers/espi_saf.h"
#endif

#ifndef ZEPHYR_INCLUDE_DRIVERS_ESPI_INTERNAL_ESPI_SAF_IMPL_H_
#define ZEPHYR_INCLUDE_DRIVERS_ESPI_INTERNAL_ESPI_SAF_IMPL_H_

#include <errno.h>

#include <zephyr/types.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int z_impl_espi_saf_config(const struct device *dev,
					 const struct espi_saf_cfg *cfg)
{
	const struct espi_saf_driver_api *api =
		(const struct espi_saf_driver_api *)dev->api;

	return api->config(dev, cfg);
}

static inline int z_impl_espi_saf_set_protection_regions(
					const struct device *dev,
					const struct espi_saf_protection *pr)
{
	const struct espi_saf_driver_api *api =
		(const struct espi_saf_driver_api *)dev->api;

	return api->set_protection_regions(dev, pr);
}

static inline int z_impl_espi_saf_activate(const struct device *dev)
{
	const struct espi_saf_driver_api *api =
		(const struct espi_saf_driver_api *)dev->api;

	return api->activate(dev);
}

static inline bool z_impl_espi_saf_get_channel_status(
					const struct device *dev)
{
	const struct espi_saf_driver_api *api =
		(const struct espi_saf_driver_api *)dev->api;

	return api->get_channel_status(dev);
}

static inline int z_impl_espi_saf_flash_read(const struct device *dev,
					     struct espi_saf_packet *pckt)
{
	const struct espi_saf_driver_api *api =
		(const struct espi_saf_driver_api *)dev->api;

	if (!api->flash_read) {
		return -ENOTSUP;
	}

	return api->flash_read(dev, pckt);
}

static inline int z_impl_espi_saf_flash_write(const struct device *dev,
					      struct espi_saf_packet *pckt)
{
	const struct espi_saf_driver_api *api =
		(const struct espi_saf_driver_api *)dev->api;

	if (!api->flash_write) {
		return -ENOTSUP;
	}

	return api->flash_write(dev, pckt);
}

static inline int z_impl_espi_saf_flash_erase(const struct device *dev,
					      struct espi_saf_packet *pckt)
{
	const struct espi_saf_driver_api *api =
		(const struct espi_saf_driver_api *)dev->api;

	if (!api->flash_erase) {
		return -ENOTSUP;
	}

	return api->flash_erase(dev, pckt);
}

static inline int z_impl_espi_saf_flash_unsuccess(const struct device *dev,
						  struct espi_saf_packet *pckt)
{
	const struct espi_saf_driver_api *api =
		(const struct espi_saf_driver_api *)dev->api;

	if (!api->flash_unsuccess) {
		return -ENOTSUP;
	}

	return api->flash_unsuccess(dev, pckt);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_ESPI_INTERNAL_ESPI_SAF_IMPL_H_ */
