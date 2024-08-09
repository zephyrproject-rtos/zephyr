/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Inline syscall implementation for SDHC driver APIs.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SDHC_H_
#error "Should only be included by zephyr/drivers/sdhc.h"
#endif

#ifndef ZEPHYR_INCLUDE_DRIVERS_SDHC_INTERNAL_SDHC_IMPL_H_
#define ZEPHYR_INCLUDE_DRIVERS_SDHC_INTERNAL_SDHC_IMPL_H_

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/sd/sd_spec.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int z_impl_sdhc_hw_reset(const struct device *dev)
{
	const struct sdhc_driver_api *api = (const struct sdhc_driver_api *)dev->api;

	if (!api->reset) {
		return -ENOSYS;
	}

	return api->reset(dev);
}

static inline int z_impl_sdhc_request(const struct device *dev,
				      struct sdhc_command *cmd,
				      struct sdhc_data *data)
{
	const struct sdhc_driver_api *api = (const struct sdhc_driver_api *)dev->api;

	if (!api->request) {
		return -ENOSYS;
	}

	return api->request(dev, cmd, data);
}

static inline int z_impl_sdhc_set_io(const struct device *dev,
				     struct sdhc_io *io)
{
	const struct sdhc_driver_api *api = (const struct sdhc_driver_api *)dev->api;

	if (!api->set_io) {
		return -ENOSYS;
	}

	return api->set_io(dev, io);
}

static inline int z_impl_sdhc_card_present(const struct device *dev)
{
	const struct sdhc_driver_api *api = (const struct sdhc_driver_api *)dev->api;

	if (!api->get_card_present) {
		return -ENOSYS;
	}

	return api->get_card_present(dev);
}

static inline int z_impl_sdhc_execute_tuning(const struct device *dev)
{
	const struct sdhc_driver_api *api = (const struct sdhc_driver_api *)dev->api;

	if (!api->execute_tuning) {
		return -ENOSYS;
	}

	return api->execute_tuning(dev);
}

static inline int z_impl_sdhc_card_busy(const struct device *dev)
{
	const struct sdhc_driver_api *api = (const struct sdhc_driver_api *)dev->api;

	if (!api->card_busy) {
		return -ENOSYS;
	}

	return api->card_busy(dev);
}

static inline int z_impl_sdhc_get_host_props(const struct device *dev,
					     struct sdhc_host_props *props)
{
	const struct sdhc_driver_api *api = (const struct sdhc_driver_api *)dev->api;

	if (!api->get_host_props) {
		return -ENOSYS;
	}

	return api->get_host_props(dev, props);
}

static inline int z_impl_sdhc_enable_interrupt(const struct device *dev,
					       sdhc_interrupt_cb_t callback,
					       int sources, void *user_data)
{
	const struct sdhc_driver_api *api = (const struct sdhc_driver_api *)dev->api;

	if (!api->enable_interrupt) {
		return -ENOSYS;
	}

	return api->enable_interrupt(dev, callback, sources, user_data);
}

static inline int z_impl_sdhc_disable_interrupt(const struct device *dev,
						int sources)
{
	const struct sdhc_driver_api *api = (const struct sdhc_driver_api *)dev->api;

	if (!api->disable_interrupt) {
		return -ENOSYS;
	}

	return api->disable_interrupt(dev, sources);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SDHC_INTERNAL_SDHC_IMPL_H_ */
