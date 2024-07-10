/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Inline syscall implementation for ADC API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ADC_H_
#error "Should only be included by zephyr/drivers/adc.h"
#endif

#ifndef ZEPHYR_INCLUDE_DRIVERS_ADC_INTERNAL_ADC_IMPL_H_
#define ZEPHYR_INCLUDE_DRIVERS_ADC_INTERNAL_ADC_IMPL_H_

#include <zephyr/device.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int z_impl_adc_channel_setup(const struct device *dev,
					   const struct adc_channel_cfg *channel_cfg)
{
	const struct adc_driver_api *api =
				(const struct adc_driver_api *)dev->api;

	return api->channel_setup(dev, channel_cfg);
}

static inline int z_impl_adc_read(const struct device *dev,
				  const struct adc_sequence *sequence)
{
	const struct adc_driver_api *api =
				(const struct adc_driver_api *)dev->api;

	return api->read(dev, sequence);
}

#ifdef CONFIG_ADC_ASYNC
static inline int z_impl_adc_read_async(const struct device *dev,
					const struct adc_sequence *sequence,
					struct k_poll_signal *async)
{
	const struct adc_driver_api *api =
				(const struct adc_driver_api *)dev->api;

	return api->read_async(dev, sequence, async);
}
#endif /* CONFIG_ADC_ASYNC */

#ifdef __cplusplus
}
#endif

#endif  /* ZEPHYR_INCLUDE_DRIVERS_ADC_INTERNAL_ADC_IMPL_H_ */
