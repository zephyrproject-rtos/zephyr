/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_TIMING_TIMING_H_
#define ZEPHYR_INCLUDE_TIMING_TIMING_H_

#include <zephyr/sys/arch_interface.h>
#include <zephyr/timing/types.h>

#ifdef __cplusplus
extern "C" {
#endif

void soc_timing_init(void);
void soc_timing_start(void);
void soc_timing_stop(void);
timing_t soc_timing_counter_get(void);
uint64_t soc_timing_cycles_get(volatile timing_t *const start,
			       volatile timing_t *const end);
uint64_t soc_timing_freq_get(void);
uint64_t soc_timing_cycles_to_ns(uint64_t cycles);
uint64_t soc_timing_cycles_to_ns_avg(uint64_t cycles, uint32_t count);
uint32_t soc_timing_freq_get_mhz(void);

void board_timing_init(void);
void board_timing_start(void);
void board_timing_stop(void);
timing_t board_timing_counter_get(void);
uint64_t board_timing_cycles_get(volatile timing_t *const start,
				 volatile timing_t *const end);
uint64_t board_timing_freq_get(void);
uint64_t board_timing_cycles_to_ns(uint64_t cycles);
uint64_t board_timing_cycles_to_ns_avg(uint64_t cycles, uint32_t count);
uint32_t board_timing_freq_get_mhz(void);


/**
 * @brief Timing Measurement APIs
 * @defgroup timing_api Timing APIs
 * @{
 */

#ifdef CONFIG_TIMING_FUNCTIONS

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
static inline timing_t timing_counter_get(void)
{
#if defined(CONFIG_BOARD_HAS_TIMING_FUNCTIONS)
	return board_timing_counter_get();
#elif defined(CONFIG_SOC_HAS_TIMING_FUNCTIONS)
	return soc_timing_counter_get();
#else
	return arch_timing_counter_get();
#endif
}

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
static inline uint64_t timing_cycles_get(volatile timing_t *const start,
					 volatile timing_t *const end)
{
#if defined(CONFIG_BOARD_HAS_TIMING_FUNCTIONS)
	return board_timing_cycles_get(start, end);
#elif defined(CONFIG_SOC_HAS_TIMING_FUNCTIONS)
	return soc_timing_cycles_get(start, end);
#else
	return arch_timing_cycles_get(start, end);
#endif
}

/**
 * @brief Get frequency of counter used (in Hz).
 *
 * @return Frequency of counter used for timing in Hz.
 */
static inline uint64_t timing_freq_get(void)
{
#if defined(CONFIG_BOARD_HAS_TIMING_FUNCTIONS)
	return board_timing_freq_get();
#elif defined(CONFIG_SOC_HAS_TIMING_FUNCTIONS)
	return soc_timing_freq_get();
#else
	return arch_timing_freq_get();
#endif
}

/**
 * @brief Convert number of @p cycles into nanoseconds.
 *
 * @param cycles Number of cycles
 * @return Converted time value
 */
static inline uint64_t timing_cycles_to_ns(uint64_t cycles)
{
#if defined(CONFIG_BOARD_HAS_TIMING_FUNCTIONS)
	return board_timing_cycles_to_ns(cycles);
#elif defined(CONFIG_SOC_HAS_TIMING_FUNCTIONS)
	return soc_timing_cycles_to_ns(cycles);
#else
	return arch_timing_cycles_to_ns(cycles);
#endif
}

/**
 * @brief Convert number of @p cycles into nanoseconds with averaging.
 *
 * @param cycles Number of cycles
 * @param count Times of accumulated cycles to average over
 * @return Converted time value
 */
static inline uint64_t timing_cycles_to_ns_avg(uint64_t cycles, uint32_t count)
{
#if defined(CONFIG_BOARD_HAS_TIMING_FUNCTIONS)
	return board_timing_cycles_to_ns_avg(cycles, count);
#elif defined(CONFIG_SOC_HAS_TIMING_FUNCTIONS)
	return soc_timing_cycles_to_ns_avg(cycles, count);
#else
	return arch_timing_cycles_to_ns_avg(cycles, count);
#endif
}

/**
 * @brief Get frequency of counter used (in MHz).
 *
 * @return Frequency of counter used for timing in MHz.
 */
static inline uint32_t timing_freq_get_mhz(void)
{
#if defined(CONFIG_BOARD_HAS_TIMING_FUNCTIONS)
	return board_timing_freq_get_mhz();
#elif defined(CONFIG_SOC_HAS_TIMING_FUNCTIONS)
	return soc_timing_freq_get_mhz();
#else
	return arch_timing_freq_get_mhz();
#endif
}

#endif /* CONFIG_TIMING_FUNCTIONS */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_TIMING_TIMING_H_ */
