/*
 * Copyright (c) 2020 Libre Solar Technologies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Inline syscall implementation for DAC APIs.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_DAC_H_
#error "Should only be included by zephyr/drivers/dac.h"
#endif

#ifndef ZEPHYR_INCLUDE_DRIVERS_DAC_INTERNAL_DAC_IMPL_H_
#define ZEPHYR_INCLUDE_DRIVERS_DAC_INTERNAL_DAC_IMPL_H_

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int z_impl_dac_channel_setup(const struct device *dev,
					   const struct dac_channel_cfg *channel_cfg)
{
	const struct dac_driver_api *api =
				(const struct dac_driver_api *)dev->api;

	return api->channel_setup(dev, channel_cfg);
}

static inline int z_impl_dac_write_value(const struct device *dev,
						uint8_t channel, uint32_t value)
{
	const struct dac_driver_api *api =
				(const struct dac_driver_api *)dev->api;

	return api->write_value(dev, channel, value);
}

#ifdef __cplusplus
}
#endif

#endif  /* ZEPHYR_INCLUDE_DRIVERS_DAC_INTERNAL_DAC_IMPL_H_ */
