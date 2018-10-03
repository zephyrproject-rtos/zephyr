/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_PTP_CLOCK_H_
#define ZEPHYR_INCLUDE_PTP_CLOCK_H_

#include <stdint.h>
#include <device.h>
#include <misc/util.h>
#include <net/ptp_time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Name of the PTP clock driver */
#if !defined(PTP_CLOCK_NAME)
#define PTP_CLOCK_NAME "PTP_CLOCK"
#endif

struct ptp_clock_driver_api {
	int (*set)(struct device *dev, struct net_ptp_time *tm);
	int (*get)(struct device *dev, struct net_ptp_time *tm);
	int (*adjust)(struct device *dev, int increment);
	int (*rate_adjust)(struct device *dev, float ratio);
};

/**
 * @brief Set the time of the PTP clock.
 *
 * @param dev PTP clock device
 * @param tm Time to set
 *
 * @return 0 if ok, <0 if error
 */
static inline int ptp_clock_set(struct device *dev, struct net_ptp_time *tm)
{
	const struct ptp_clock_driver_api *api = dev->driver_api;

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
static inline int ptp_clock_get(struct device *dev, struct net_ptp_time *tm)
{
	const struct ptp_clock_driver_api *api = dev->driver_api;

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
static inline int ptp_clock_adjust(struct device *dev, int increment)
{
	const struct ptp_clock_driver_api *api = dev->driver_api;

	return api->adjust(dev, increment);
}

/**
 * @brief Adjust the PTP clock time change rate when compared to its neighbor.
 *
 * @param dev PTP clock device
 * @param rate Rate of the clock time change
 *
 * @return 0 if ok, <0 if error
 */
static inline int ptp_clock_rate_adjust(struct device *dev, float rate)
{
	const struct ptp_clock_driver_api *api = dev->driver_api;

	return api->rate_adjust(dev, rate);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_PTP_CLOCK_H_ */
