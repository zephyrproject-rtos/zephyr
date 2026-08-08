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
 * @since 1.13
 * @version 1.0.0
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/kernel.h>
#include <errno.h>
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/net/ptp_time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Name of the PTP clock driver */
#if !defined(PTP_CLOCK_NAME)
#define PTP_CLOCK_NAME "PTP_CLOCK"
#endif

/**
 * @def_driverbackendgroup{PTP Clock,ptp_clock_interface}
 * @{
 */

/** @brief PTP clock capability flags. */
enum ptp_clock_caps_flags {
	/** The clock time can be read with ptp_clock_get(). */
	PTP_CLOCK_CAP_READ = BIT(0),
	/** The clock time can be set with ptp_clock_set(). */
	PTP_CLOCK_CAP_SET = BIT(1),
	/** The clock supports phase adjustment with ptp_clock_adjust(). */
	PTP_CLOCK_CAP_ADJUST = BIT(2),
	/** The clock supports rate adjustment with ptp_clock_rate_adjust(). */
	PTP_CLOCK_CAP_RATE_ADJUST = BIT(3),
};

/** @brief PTP clock capabilities and adjustment limits. */
struct ptp_clock_caps {
	/** Combination of @ref ptp_clock_caps_flags. */
	uint32_t flags;
	/** Smallest representable clock increment in nanoseconds. */
	uint32_t resolution_ns;
	/** Largest supported absolute phase adjustment in nanoseconds. */
	int32_t max_adjust_ns;
	/** Minimum supported rate adjustment in parts per billion. */
	int32_t min_rate_ppb;
	/** Maximum supported rate adjustment in parts per billion. */
	int32_t max_rate_ppb;
};

/**
 * @brief Set the time of the PTP clock.
 * See ptp_clock_set() for argument description.
 */
typedef int (*ptp_clock_api_set_t)(const struct device *dev, struct net_ptp_time *tm);

/**
 * @brief Get the time of the PTP clock.
 * See ptp_clock_get() for argument description.
 */
typedef int (*ptp_clock_api_get_t)(const struct device *dev, struct net_ptp_time *tm);

/**
 * @brief Adjust the PTP clock time.
 * See ptp_clock_adjust() for argument description.
 */
typedef int (*ptp_clock_api_adjust_t)(const struct device *dev, int increment);

/**
 * @brief Adjust the PTP clock rate ratio based on its nominal frequency.
 * See ptp_clock_rate_adjust() for argument description.
 */
typedef int (*ptp_clock_api_rate_adjust_t)(const struct device *dev, double ratio);

/**
 * @brief Query PTP clock capabilities and limits.
 * See ptp_clock_get_caps() for argument description.
 */
typedef int (*ptp_clock_api_get_caps_t)(const struct device *dev, struct ptp_clock_caps *caps);

/**
 * @driver_ops{PTP Clock}
 */
__subsystem struct ptp_clock_driver_api {
	/**
	 * @driver_ops_mandatory @copybrief ptp_clock_set
	 */
	ptp_clock_api_set_t set;
	/**
	 * @driver_ops_mandatory @copybrief ptp_clock_get
	 */
	ptp_clock_api_get_t get;
	/**
	 * @driver_ops_mandatory @copybrief ptp_clock_adjust
	 */
	ptp_clock_api_adjust_t adjust;
	/**
	 * @driver_ops_mandatory @copybrief ptp_clock_rate_adjust
	 */
	ptp_clock_api_rate_adjust_t rate_adjust;
	/**
	 * @driver_ops_optional @copybrief ptp_clock_get_caps
	 */
	ptp_clock_api_get_caps_t get_caps;
};

/** @} */

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
	return DEVICE_API_GET(ptp_clock, dev)->set(dev, tm);
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
	return DEVICE_API_GET(ptp_clock, dev)->get(dev, tm);
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
	return DEVICE_API_GET(ptp_clock, dev)->adjust(dev, increment);
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
	return DEVICE_API_GET(ptp_clock, dev)->rate_adjust(dev, rate);
}

/**
 * @brief Query PTP clock capabilities and limits.
 *
 * @param dev PTP clock device
 * @param caps Where to store capabilities
 *
 * @return 0 if ok, -ENOTSUP if the driver does not report capabilities,
 *	   <0 if error
 */
static inline int ptp_clock_get_caps(const struct device *dev, struct ptp_clock_caps *caps)
{
	const struct ptp_clock_driver_api *api;

	if (dev == NULL || caps == NULL) {
		return -EINVAL;
	}

	api = DEVICE_API_GET(ptp_clock, dev);
	if (api == NULL || api->get_caps == NULL) {
		return -ENOTSUP;
	}

	return api->get_caps(dev, caps);
}

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/ptp_clock.h>

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_PTP_CLOCK_H_ */
