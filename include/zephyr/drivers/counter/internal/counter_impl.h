/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Inline syscall implementation for counter and timer driver APIs.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_COUNTER_H_
#error "Should only be included by zephyr/drivers/counter.h"
#endif

#ifndef ZEPHYR_INCLUDE_DRIVERS_COUNTER_INTERNAL_COUNTER_IMPL_H_
#define ZEPHYR_INCLUDE_DRIVERS_COUNTER_INTERNAL_COUNTER_IMPL_H_

#include <errno.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/device.h>
#include <zephyr/sys_clock.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline bool z_impl_counter_is_counting_up(const struct device *dev)
{
	const struct counter_config_info *config =
			(const struct counter_config_info *)dev->config;

	return config->flags & COUNTER_CONFIG_INFO_COUNT_UP;
}

static inline uint8_t z_impl_counter_get_num_of_channels(const struct device *dev)
{
	const struct counter_config_info *config =
			(const struct counter_config_info *)dev->config;

	return config->channels;
}

static inline uint32_t z_impl_counter_get_frequency(const struct device *dev)
{
	const struct counter_config_info *config =
			(const struct counter_config_info *)dev->config;
	const struct counter_driver_api *api =
				(struct counter_driver_api *)dev->api;

	return api->get_freq ? api->get_freq(dev) : config->freq;
}

static inline uint32_t z_impl_counter_us_to_ticks(const struct device *dev,
					       uint64_t us)
{
	uint64_t ticks = (us * z_impl_counter_get_frequency(dev)) / USEC_PER_SEC;

	return (ticks > (uint64_t)UINT32_MAX) ? UINT32_MAX : ticks;
}

static inline uint64_t z_impl_counter_ticks_to_us(const struct device *dev,
					       uint32_t ticks)
{
	return ((uint64_t)ticks * USEC_PER_SEC) / z_impl_counter_get_frequency(dev);
}

static inline uint32_t z_impl_counter_get_max_top_value(const struct device *dev)
{
	const struct counter_config_info *config =
			(const struct counter_config_info *)dev->config;

	return config->max_top_value;
}

static inline int z_impl_counter_start(const struct device *dev)
{
	const struct counter_driver_api *api =
				(struct counter_driver_api *)dev->api;

	return api->start(dev);
}

static inline int z_impl_counter_stop(const struct device *dev)
{
	const struct counter_driver_api *api =
				(struct counter_driver_api *)dev->api;

	return api->stop(dev);
}

static inline int z_impl_counter_get_value(const struct device *dev,
					   uint32_t *ticks)
{
	const struct counter_driver_api *api =
				(struct counter_driver_api *)dev->api;

	return api->get_value(dev, ticks);
}

static inline int z_impl_counter_get_value_64(const struct device *dev,
					   uint64_t *ticks)
{
	const struct counter_driver_api *api =
				(struct counter_driver_api *)dev->api;

	if (!api->get_value_64) {
		return -ENOTSUP;
	}

	return api->get_value_64(dev, ticks);
}

static inline int z_impl_counter_set_channel_alarm(const struct device *dev,
						   uint8_t chan_id,
						   const struct counter_alarm_cfg *alarm_cfg)
{
	const struct counter_driver_api *api =
				(struct counter_driver_api *)dev->api;

	if (chan_id >= counter_get_num_of_channels(dev)) {
		return -ENOTSUP;
	}

	return api->set_alarm(dev, chan_id, alarm_cfg);
}

static inline int z_impl_counter_cancel_channel_alarm(const struct device *dev,
						      uint8_t chan_id)
{
	const struct counter_driver_api *api =
				(struct counter_driver_api *)dev->api;

	if (chan_id >= counter_get_num_of_channels(dev)) {
		return -ENOTSUP;
	}

	return api->cancel_alarm(dev, chan_id);
}

static inline int z_impl_counter_set_top_value(const struct device *dev,
					       const struct counter_top_cfg
					       *cfg)
{
	const struct counter_driver_api *api =
				(struct counter_driver_api *)dev->api;

	if (cfg->ticks > counter_get_max_top_value(dev)) {
		return -EINVAL;
	}

	return api->set_top_value(dev, cfg);
}

static inline int z_impl_counter_get_pending_int(const struct device *dev)
{
	const struct counter_driver_api *api =
				(struct counter_driver_api *)dev->api;

	return api->get_pending_int(dev);
}

static inline uint32_t z_impl_counter_get_top_value(const struct device *dev)
{
	const struct counter_driver_api *api =
				(struct counter_driver_api *)dev->api;

	return api->get_top_value(dev);
}

static inline int z_impl_counter_set_guard_period(const struct device *dev,
						   uint32_t ticks, uint32_t flags)
{
	const struct counter_driver_api *api =
				(struct counter_driver_api *)dev->api;

	if (!api->set_guard_period) {
		return -ENOTSUP;
	}

	return api->set_guard_period(dev, ticks, flags);
}

static inline uint32_t z_impl_counter_get_guard_period(const struct device *dev,
							uint32_t flags)
{
	const struct counter_driver_api *api =
				(struct counter_driver_api *)dev->api;

	return (api->get_guard_period) ? api->get_guard_period(dev, flags) : 0;
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_COUNTER_INTERNAL_COUNTER_IMPL_H_ */
