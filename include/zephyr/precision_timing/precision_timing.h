/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Philipp Steiner
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Protocol-neutral precision timing APIs.
 *
 * Convenience header that pulls in the complete precision timing API. Include
 * the individual headers directly when only a part of the API is needed.
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_PRECISION_TIMING_PRECISION_TIMING_H_
#define ZEPHYR_INCLUDE_ZEPHYR_PRECISION_TIMING_PRECISION_TIMING_H_

/**
 * @brief Precision Timing APIs
 * @defgroup precision_timing Precision Timing
 * @since 4.5
 * @version 0.1.0
 * @ingroup os_services
 * @{
 */

/** @} */

#include <zephyr/precision_timing/precision_clock.h>
#include <zephyr/precision_timing/precision_deadline.h>
#include <zephyr/precision_timing/precision_mapping.h>
#include <zephyr/precision_timing/precision_pi.h>
#include <zephyr/precision_timing/precision_time.h>

#endif /* ZEPHYR_INCLUDE_ZEPHYR_PRECISION_TIMING_PRECISION_TIMING_H_ */
