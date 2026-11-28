/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 * Copyright 2026 Arm Limited and/or its affiliates <open-source-office@arm.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Portable CPU performance monitoring (PMU) counter API
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_ARCH_PERF_COUNTER_H_
#define ZEPHYR_INCLUDE_ZEPHYR_ARCH_PERF_COUNTER_H_

#include <zephyr/autoconf.h>
#include <stdint.h>
#include <stdbool.h>

#if defined(CONFIG_ARM64)
#include <zephyr/arch/arm64/pmuv3.h>
#else
/**
 * @brief Architectural / backend event selector when no arch supplement defines
 *        @c perf_counter_evt_t.
 */
typedef uint32_t perf_counter_evt_t;

/** @brief Placeholder @c perf_counter_info_t when no arch supplement is present. */
typedef struct {
	/** PMU implementer identification value when reported by the backend. */
	uint32_t implementer;
	/** PMU part number / ID code when reported by the backend. */
	uint32_t idcode;
} perf_counter_info_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup perf_counter Performance counters
 * @ingroup io_interfaces
 * @version 0.1.0
 *
 * @note This API is experimental and may change in future releases.
 *
 * @{
 */

/** Performance counter configuration entry */
struct perf_counter_config {
	/** Event (e.g. @c PMU_EVT_* enumerators when @c CONFIG_ARM64). */
	perf_counter_evt_t event;
	/** Enable this counter when configured */
	bool enabled;
};

/** Information about a named event supported by the current CPU PMU. */
struct perf_counter_event_info {
	/** Architecture-specific selector accepted by perf_counter_config(). */
	perf_counter_evt_t event;
	/** Architecture-defined event name, with static lifetime. */
	const char *name;
};

/**
 * @brief Initialize the PMU for the current logical CPU.
 *
 * Architecture-neutral interface for hardware performance counters.
 * Backends implement these functions for each supported architecture
 * (for example ARMv8-A PMUv3 on AArch64, future ARMv8-R / Cortex-R52,
 * RISC-V HPM CSRs, etc.).
 *
 * Architectural event codes are defined by the architecture supplement
 * (for example @c perf_counter_evt_t in <zephyr/arch/arm64/pmuv3.h> when @c CONFIG_ARM64
 * is enabled).
 *
 * PMU is not initialized during kernel startup. Call @ref perf_counter_init() before
 * other @c perf_counter_*() functions on this CPU, except the event discovery functions.
 *
 * Call once per logical CPU that will use counters, before any other
 * counter API on that CPU. Idempotent on success: later calls return
 * @c 0 if this CPU already initialized successfully, or @c -ENOTSUP if this
 * CPU recorded that no PMU is available.
 *
 * PMU control and counters are per-CPU. On SMP, use the PMU only while running
 * on the intended CPU — for example pin the thread with @c k_thread_cpu_pin()
 * (requires @c CONFIG_SCHED_CPU_MASK), or constrain its CPU affinity with
 * @c k_thread_cpu_mask_enable() / @c k_thread_cpu_mask_disable(), so it does
 * not migrate during a measurement.
 *
 * @warning Callers must serialize PMU operations and provide exclusive PMU
 * ownership for measurements. Do not modify the PMU while a perf session or
 * another PMU user is measuring on the same CPU. Read-only event discovery may
 * be used during a measurement.
 *
 * There is no @c SYS_INIT hook; callers choose when to probe
 * (for example at the start of a benchmark).
 *
 * @retval 0 Success
 * @retval -ENOTSUP No PMU or not usable at this exception level on this CPU
 */
int perf_counter_init(void);

/** @return Number of event counters on this CPU, or 0 if PMU unavailable */
uint32_t perf_counter_num_counters(void);

/**
 * @brief Return the number of named events supported by the current CPU PMU.
 *
 * Available when a PMU backend is enabled. Event discovery may be used
 * before perf_counter_init() and does not change PMU state.
 * Callers must have access to the PMU registers and remain on the same CPU
 * throughout enumeration. Catalogs may differ between CPUs.
 *
 * @return Number of event records, or zero if the PMU is unavailable or has
 *         no supported named events.
 */
uint32_t perf_counter_num_events(void);

/**
 * @brief Get information about a named event supported by the current CPU PMU.
 *
 * Available when a PMU backend is enabled. Indexes range from 0 to
 * perf_counter_num_events() - 1 and identify positions in the supported event list.
 * The hardware event ID is returned in @c info->event.
 *
 * Names have static lifetime. This function has the same CPU affinity and
 * register-access requirements as perf_counter_num_events(), may be used before
 * perf_counter_init(), and does not change PMU state.
 *
 * @param index Position in the supported event list, in the range [0, perf_counter_num_events()).
 * @param info Caller-owned output storage. Must not be NULL.
 *
 * @retval 0 Success
 * @retval -EINVAL Index outside the current CPU's supported event list or NULL output storage
 */
int perf_counter_event_get(uint32_t index, struct perf_counter_event_info *info);

/**
 * @brief Fill @a info with PMU identification for the current logical CPU.
 *
 * @a info must not be NULL. Fields are only meaningful after a successful
 * @ref perf_counter_init on this CPU; contents are backend-defined
 * (see arch @c perf_counter_info_t).
 */
void perf_counter_get_info(perf_counter_info_t *info);

/**
 * @brief Map hardware event counter @a counter to architectural event @a event.
 *
 * @param counter Event counter index in the range [0, perf_counter_num_counters()).
 * @param event Event selector (@c perf_counter_evt_t from the architecture supplement
 *              when present).
 *
 * @return 0 on success, negative errno on failure
 */
int perf_counter_config(uint32_t counter, perf_counter_evt_t event);

/**
 * @brief Enable counting on hardware event counter @a counter.
 *
 * @param counter Event counter index [0, perf_counter_num_counters()).
 */
void perf_counter_enable(uint32_t counter);

/**
 * @brief Disable counting on hardware event counter @a counter.
 *
 * @param counter Event counter index [0, perf_counter_num_counters()).
 */
void perf_counter_disable(uint32_t counter);

/**
 * @brief Enable all implemented event counters (and the cycle counter where applicable).
 */
void perf_counter_enable_all(void);

/**
 * @brief Disable all event counters (and the cycle counter where applicable).
 */
void perf_counter_disable_all(void);

/**
 * @brief Read the current value of hardware event counter @a counter.
 *
 * @param counter Event counter index [0, perf_counter_num_counters()).
 *
 * @return Counter value, or 0 if PMU is unavailable or @a counter is out of range
 */
uint64_t perf_counter_read(uint32_t counter);

/**
 * @brief Reset hardware event counter @a counter to zero.
 *
 * @param counter Event counter index [0, perf_counter_num_counters()).
 */
void perf_counter_reset(uint32_t counter);

/**
 * @brief Reset all hardware event counters to zero (implementation-defined;
 *        may include cycle counter policies).
 */
void perf_counter_reset_all(void);

/**
 * @brief Read the architectural cycle count register (if implemented).
 *
 * @return Cycle count, or 0 if unavailable
 */
uint64_t perf_counter_cycle_count(void);

/**
 * @brief Reset the cycle count register to zero (implementation-defined).
 */
void perf_counter_cycle_reset(void);

/**
 * @brief Start counting: enable the PMU globally (implementation-defined).
 */
void perf_counter_start(void);

/**
 * @brief Stop counting: disable the PMU globally (implementation-defined).
 */
void perf_counter_stop(void);

/**
 * @brief Return whether hardware event counter @a counter has overflowed
 *        since the flag was cleared.
 *
 * @param counter Event counter index [0, perf_counter_num_counters()).
 *
 * @return true if overflow is pending, false otherwise or if PMU is unavailable
 */
bool perf_counter_overflow(uint32_t counter);

/**
 * @brief Clear the overflow flag for hardware event counter @a counter.
 *
 * @param counter Event counter index [0, perf_counter_num_counters()).
 */
void perf_counter_clear_overflow(uint32_t counter);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZEPHYR_ARCH_PERF_COUNTER_H_ */
