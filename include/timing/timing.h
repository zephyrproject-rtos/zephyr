/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_TIMING_TIMING_H_
#define ZEPHYR_INCLUDE_TIMING_TIMING_H_

#include <kernel.h>


/**
 * @brief Timing Measurement APIs
 * @defgroup timing_api Timing APIs
 * @{
 */


typedef uint64_t timing_t;

/**
 * @brief Initialize the timing subsystem.
 *
 * Perform the necessary steps to initialize the timing subsystem.
 */
void timing_init(void);

/**
 * @brief Signal the start of the timing information gathering.
 *
 * Signal to the timing subsystem that timing information
 * will be gathered from this point forward.
 */
void timing_start(void);

/**
 * @brief Signal the end of the timing information gathering.
 *
 * Signal to the timing subsystem that timing information
 * is no longer being gathered from this point forward.
 */
void timing_stop(void);

/**
 * @brief Return timing counter.
 *
 * @return Timing counter.
 */
timing_t timing_counter_get(void);

/**
 * @brief Get number of cycles between @p start and @p end.
 *
 * For some architectures or SoCs, the raw numbers from counter
 * need to be scaled to obtain actual number of cycles.
 *
 * @param start Pointer to counter at start of a measured execution.
 * @param end Pointer to counter at stop of a measured execution.
 * @return Number of cycles between start and end.
 */
uint64_t timing_cycles_get(volatile timing_t *const start,
					 volatile timing_t *const end);

/**
 * @brief Get frequency of counter used (in Hz).
 *
 * @return Frequency of counter used for timing in Hz.
 */
uint64_t timing_freq_get(void);

/**
 * @brief Convert number of @p cycles into nanoseconds.
 *
 * @param cycles Number of cycles
 * @return Converted time value
 */
uint64_t timing_cycles_to_ns(uint64_t cycles);

uint64_t timing_cycles_to_ns_avg(uint64_t cycles, uint32_t count);

/**
 * @brief Get frequency of counter used (in MHz).
 *
 * @return Frequency of counter used for timing in MHz.
 */
uint32_t timing_freq_get_mhz(void);

/**
 * @}
 */
#endif /* ZEPHYR_INCLUDE_TIMING_TIMING_H_ */
