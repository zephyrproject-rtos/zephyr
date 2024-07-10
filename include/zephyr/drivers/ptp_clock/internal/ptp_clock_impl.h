/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_PTP_CLOCK_H_
#error "Should only be included by zephyr/drivers/ptp_clock.h"
#endif

#ifndef ZEPHYR_INCLUDE_DRIVERS_PTP_CLOCK_INTERNAL_PTP_CLOCK_IMPL_H_
#define ZEPHYR_INCLUDE_DRIVERS_PTP_CLOCK_INTERNAL_PTP_CLOCK_IMPL_H_

#include <zephyr/kernel.h>
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/net/ptp_time.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int z_impl_ptp_clock_get(const struct device *dev,
				       struct net_ptp_time *tm)
{
	const struct ptp_clock_driver_api *api =
		(const struct ptp_clock_driver_api *)dev->api;

	return api->get(dev, tm);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_PTP_CLOCK_INTERNAL_PTP_CLOCK_IMPL_H_ */
