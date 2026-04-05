/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup ptp_clock_interface
 * @brief Main header file for PTP (Precision Time Protocol) clock driver API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_PTP_CLOCK_H_
#define ZEPHYR_INCLUDE_DRIVERS_PTP_CLOCK_H_

/**
 * @brief Interfaces for Precision Time Protocol (PTP) clocks.
 * @defgroup ptp_clock_interface PTP Clock
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/net/ptp_time.h>
#include <errno.h>

struct phy_latency;

#ifdef __cplusplus
extern "C" {
#endif

/* Name of the PTP clock driver */
#if !defined(PTP_CLOCK_NAME)
#define PTP_CLOCK_NAME "PTP_CLOCK"
#endif

__subsystem struct ptp_clock_driver_api {
	int (*set)(const struct device *dev, struct net_ptp_time *tm);
	int (*get)(const struct device *dev, struct net_ptp_time *tm);
	int (*adjust)(const struct device *dev, int increment);
	int (*rate_adjust)(const struct device *dev, double ratio);
	int (*set_latency)(const struct device *dev, const struct phy_latency *latency);
};

/**
 * @brief Set the time of the PTP clock.
 *
 * @param dev PTP clock device
 * @param tm Time to set
 *
 * @return 0 if ok, <0 if error
 */
static inline int ptp_clock_set(const struct device *dev,
				struct net_ptp_time *tm)
{
	const struct ptp_clock_driver_api *api =
		(const struct ptp_clock_driver_api *)dev->api;

	return api->set(dev, tm);
}

/**
 * @brief Get the time of the PTP clock.
 *
 * @param dev PTP clock device
 * @param tm Where to store the current time.
 *
 * @return 0 if ok, <0 if error
 */
__syscall int ptp_clock_get(const struct device *dev, struct net_ptp_time *tm);

static inline int z_impl_ptp_clock_get(const struct device *dev,
				       struct net_ptp_time *tm)
{
	const struct ptp_clock_driver_api *api =
		(const struct ptp_clock_driver_api *)dev->api;

	return api->get(dev, tm);
}

/**
 * @brief Adjust the PTP clock time.
 *
 * @param dev PTP clock device
 * @param increment Increment of the clock in nanoseconds
 *
 * @return 0 if ok, <0 if error
 */
static inline int ptp_clock_adjust(const struct device *dev, int increment)
{
	const struct ptp_clock_driver_api *api =
		(const struct ptp_clock_driver_api *)dev->api;

	return api->adjust(dev, increment);
}

/**
 * @brief Adjust the PTP clock rate ratio based on its nominal frequency
 *
 * @param dev PTP clock device
 * @param rate Rate ratio based on its nominal frequency
 *
 * @return 0 if ok, <0 if error
 */
static inline int ptp_clock_rate_adjust(const struct device *dev, double rate)
{
	const struct ptp_clock_driver_api *api =
		(const struct ptp_clock_driver_api *)dev->api;

	return api->rate_adjust(dev, rate);
}

/**
 * @brief Program ingress and egress timestamp latency compensation.
 *
 * @param dev PTP clock device
 * @param latency PHY ingress and egress latency values in nanoseconds
 *
 * @return 0 if ok, `-ENOSYS` if the driver does not implement this API, <0 if error
 */
static inline int ptp_clock_set_latency(const struct device *dev, const struct phy_latency *latency)
{
	const struct ptp_clock_driver_api *api = (const struct ptp_clock_driver_api *)dev->api;

	if (api->set_latency == NULL) {
		return -ENOSYS;
	}

	return api->set_latency(dev, latency);
}

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/ptp_clock.h>

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_PTP_CLOCK_H_ */
