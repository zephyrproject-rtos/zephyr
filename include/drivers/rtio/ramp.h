/*
 * Copyright (c) 2019 Tom Burdick <tom.burdick@electromatic.us>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_RTIO_RAMP_H
#define ZEPHYR_RTIO_RAMP_H

#include <zephyr/types.h>

/**
 * @brief RTIO Ramp Signal generator configuration
 */
struct rtio_ramp_config {
	/**
	 * @brief Sample rate in Hz that the ramp to sample at
	 */
	u32_t sample_rate;

	/**
	 * @brief Value ramp generator should reset at.
	 */
	u32_t max_value;
};


#endif /* ZEPHYR_RTIO_RAMP_H */
