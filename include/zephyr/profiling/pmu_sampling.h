/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief PMU overflow PC sampling (profiling subsystem)
 */

#ifndef ZEPHYR_INCLUDE_PROFILING_PMU_SAMPLING_H_
#define ZEPHYR_INCLUDE_PROFILING_PMU_SAMPLING_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup pmu_sampling PMU sampling
 * @ingroup os_services
 * @{
 */

/** One sample captured on PMU overflow (24-byte wire format, little-endian on target). */
struct pmu_sample_record {
	/** Timestamp (cycles) when the sample was taken. */
	uint64_t tstamp;
	/** Interrupted program counter (ELR_EL1). */
	uint64_t pc;
	/** Thread ID of the interrupted thread. */
	uint32_t tid;
	/** CPU index that captured the sample. */
	uint16_t cpu;
	/** PMU event code being sampled. */
	uint16_t event;
};

/** Optional arguments for @ref pmu_sampling_start_cfg(). */
struct pmu_sampling_start_cfg {
	/** PMU_EVT_* code to sample. */
	uint32_t event;
	/** Counts between overflows. */
	uint32_t period_events;
	/** Auto-stop after this many seconds; ignored if <= 0. */
	int auto_stop_s;
};

/** Aggregate statistics (best-effort under concurrent overflow). */
struct pmu_sampling_stats {
	/** Total samples stored across all per-CPU rings. */
	size_t stored_samples;
	/** Total samples dropped because a ring was full. */
	size_t lost_samples;
	/** Approximate worst-CPU ring occupancy vs capacity, 0–100. */
	uint32_t buffer_high_water_pct;
};

/**
 * Initialize PMU sampling (connect overflow IRQ, allocate buffer).
 *
 * @retval 0 Success
 * @retval -ENODEV No IRQ configured or PMU unavailable
 */
int pmu_sampling_init(void);

/**
 * Start periodic sampling using programmable counter 0.
 *
 * Programs counter 0 for @a event, arms overflow every @a period_events
 * counts, and enables PMINTEN for that counter. Requires @a pmu_init() and
 * @ref pmu_sampling_init() first.
 *
 * @param event PMU_EVT_* code (e.g. PMU_EVT_CPU_CYCLES)
 * @param period_events Counts between overflows (1 .. UINT32_MAX/2)
 *
 * @retval 0 Success
 * @retval negative errno
 */
int pmu_sampling_start(uint32_t event, uint32_t period_events);

/**
 * Start sampling with optional auto-stop timer.
 *
 * @param cfg Event, period in PMU counts, and optional @a auto_stop_s (>0 to schedule stop).
 *
 * @retval 0 Success
 * @retval negative errno
 */
int pmu_sampling_start_cfg(const struct pmu_sampling_start_cfg *cfg);

/**
 * Derive a PMU overflow period (event counts) from a target sample rate in Hz.
 *
 * Uses arch_pmu_cpu_freq_mhz() for the current CPU. Fails if the frequency is
 * unknown (0) or the computed period is out of range.
 *
 * @param sample_hz Desired samples per second (must be > 0)
 * @param period_events Out: counts between overflows
 *
 * @retval 0 Success
 * @retval -EINVAL @a sample_hz is 0
 * @retval -ENOTSUP CPU MHz not available
 * @retval -ERANGE Period does not fit in counter range
 */
int pmu_sampling_period_from_hz(unsigned int sample_hz, uint32_t *period_events);

/** @return Whether overflow sampling is currently armed. */
bool pmu_sampling_is_active(void);

/** Stop sampling: mask PMU IRQ path, disable counter0 overflow interrupt. */
void pmu_sampling_stop(void);

/**
 * Discard all samples in the ring buffers.
 *
 * Call after @ref pmu_sampling_stop() if overflows might still be in flight.
 */
void pmu_sampling_clear(void);

/** @return Number of samples currently stored (sum of per-CPU pending samples). */
size_t pmu_sampling_count(void);

/**
 * Retrieve aggregate sampling statistics.
 *
 * @param stats Out: stored/lost sample counts and buffer high-water mark.
 */
void pmu_sampling_get_stats(struct pmu_sampling_stats *stats);

/**
 * Copy stored samples without consuming them.
 *
 * @param buf Output array
 * @param max Maximum records to copy
 * @return Number of records copied (CPU 0 .. N-1 order; within each CPU,
 *         oldest first; not globally time-ordered across CPUs)
 */
size_t pmu_sampling_copy(struct pmu_sample_record *buf, size_t max);

/**
 * Size of a zperf v1 blob for the current buffered samples (header + samples).
 */
size_t pmu_sampling_export_zperf_size(void);

/**
 * Encode a **zperf v1** binary blob (magic ``ZPERFV01``, little-endian header,
 * then @ref pmu_sample_record array). Suitable for UART capture and host decode.
 *
 * @param out Output buffer
 * @param out_len Capacity of @a out
 * @return Bytes written on success, or negative errno (-ENOSPC if @a out_len too small)
 */
int pmu_sampling_export_zperf(void *out, size_t out_len);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_PROFILING_PMU_SAMPLING_H_ */
