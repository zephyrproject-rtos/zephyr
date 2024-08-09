/*
 * Copyright (c) 2021 Vestas Wind Systems A/S
 * Copyright (c) 2018 Karsten Koenig
 * Copyright (c) 2018 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Inline syscall implementation for Controller Area Network (CAN) driver APIs.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CAN_H_
#error "Should only be included by zephyr/drivers/can.h"
#endif

#ifndef ZEPHYR_INCLUDE_DRIVERS_CAN_INTERNAL_CAN_IMPL_H_
#define ZEPHYR_INCLUDE_DRIVERS_CAN_INTERNAL_CAN_IMPL_H_

#include <errno.h>

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/sys_clock.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int z_impl_can_get_core_clock(const struct device *dev, uint32_t *rate)
{
	const struct can_driver_api *api = (const struct can_driver_api *)dev->api;

	return api->get_core_clock(dev, rate);
}

static inline uint32_t z_impl_can_get_bitrate_min(const struct device *dev)
{
	const struct can_driver_config *common = (const struct can_driver_config *)dev->config;

	return common->min_bitrate;
}

static inline uint32_t z_impl_can_get_bitrate_max(const struct device *dev)
{
	const struct can_driver_config *common = (const struct can_driver_config *)dev->config;

	return common->max_bitrate;
}

static inline const struct can_timing *z_impl_can_get_timing_min(const struct device *dev)
{
	const struct can_driver_api *api = (const struct can_driver_api *)dev->api;

	return &api->timing_min;
}

static inline const struct can_timing *z_impl_can_get_timing_max(const struct device *dev)
{
	const struct can_driver_api *api = (const struct can_driver_api *)dev->api;

	return &api->timing_max;
}

#ifdef CONFIG_CAN_FD_MODE
static inline const struct can_timing *z_impl_can_get_timing_data_min(const struct device *dev)
{
	const struct can_driver_api *api = (const struct can_driver_api *)dev->api;

	return &api->timing_data_min;
}

static inline const struct can_timing *z_impl_can_get_timing_data_max(const struct device *dev)
{
	const struct can_driver_api *api = (const struct can_driver_api *)dev->api;

	return &api->timing_data_max;
}
#endif /* CONFIG_CAN_FD_MODE */

static inline int z_impl_can_get_capabilities(const struct device *dev, can_mode_t *cap)
{
	const struct can_driver_api *api = (const struct can_driver_api *)dev->api;

	return api->get_capabilities(dev, cap);
}

static const struct device *z_impl_can_get_transceiver(const struct device *dev)
{
	const struct can_driver_config *common = (const struct can_driver_config *)dev->config;

	return common->phy;
}

static inline int z_impl_can_start(const struct device *dev)
{
	const struct can_driver_api *api = (const struct can_driver_api *)dev->api;

	return api->start(dev);
}

static inline int z_impl_can_stop(const struct device *dev)
{
	const struct can_driver_api *api = (const struct can_driver_api *)dev->api;

	return api->stop(dev);
}

static inline int z_impl_can_set_mode(const struct device *dev, can_mode_t mode)
{
	const struct can_driver_api *api = (const struct can_driver_api *)dev->api;

	return api->set_mode(dev, mode);
}

static inline can_mode_t z_impl_can_get_mode(const struct device *dev)
{
	const struct can_driver_data *common = (const struct can_driver_data *)dev->data;

	return common->mode;
}

static inline void z_impl_can_remove_rx_filter(const struct device *dev, int filter_id)
{
	const struct can_driver_api *api = (const struct can_driver_api *)dev->api;

	return api->remove_rx_filter(dev, filter_id);
}

static inline int z_impl_can_get_max_filters(const struct device *dev, bool ide)
{
	const struct can_driver_api *api = (const struct can_driver_api *)dev->api;

	if (api->get_max_filters == NULL) {
		return -ENOSYS;
	}

	return api->get_max_filters(dev, ide);
}

static inline int z_impl_can_get_state(const struct device *dev, enum can_state *state,
				       struct can_bus_err_cnt *err_cnt)
{
	const struct can_driver_api *api = (const struct can_driver_api *)dev->api;

	return api->get_state(dev, state, err_cnt);
}

#ifdef CONFIG_CAN_MANUAL_RECOVERY_MODE
static inline int z_impl_can_recover(const struct device *dev, k_timeout_t timeout)
{
	const struct can_driver_api *api = (const struct can_driver_api *)dev->api;

	if (api->recover == NULL) {
		return -ENOSYS;
	}

	return api->recover(dev, timeout);
}
#endif /* CONFIG_CAN_MANUAL_RECOVERY_MODE */

#ifdef CONFIG_CAN_STATS
static inline uint32_t z_impl_can_stats_get_bit_errors(const struct device *dev)
{
	return Z_CAN_GET_STATS(dev).bit_error;
}

static inline uint32_t z_impl_can_stats_get_bit0_errors(const struct device *dev)
{
	return Z_CAN_GET_STATS(dev).bit0_error;
}

static inline uint32_t z_impl_can_stats_get_bit1_errors(const struct device *dev)
{
	return Z_CAN_GET_STATS(dev).bit1_error;
}

static inline uint32_t z_impl_can_stats_get_stuff_errors(const struct device *dev)
{
	return Z_CAN_GET_STATS(dev).stuff_error;
}

static inline uint32_t z_impl_can_stats_get_crc_errors(const struct device *dev)
{
	return Z_CAN_GET_STATS(dev).crc_error;
}

static inline uint32_t z_impl_can_stats_get_form_errors(const struct device *dev)
{
	return Z_CAN_GET_STATS(dev).form_error;
}

static inline uint32_t z_impl_can_stats_get_ack_errors(const struct device *dev)
{
	return Z_CAN_GET_STATS(dev).ack_error;
}

static inline uint32_t z_impl_can_stats_get_rx_overruns(const struct device *dev)
{
	return Z_CAN_GET_STATS(dev).rx_overrun;
}
#endif /* CONFIG_CAN_STATS */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_CAN_INTERNAL_CAN_IMPL_H_ */
