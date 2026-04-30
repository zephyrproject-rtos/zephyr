/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief eBPF helper function IDs and dispatch.
 * @ingroup ebpf
 */

#ifndef ZEPHYR_INCLUDE_EBPF_HELPERS_H_
#define ZEPHYR_INCLUDE_EBPF_HELPERS_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Helper function IDs used by the CALL instruction's imm field. */
enum ebpf_helper_id {
	/** Look up an element in a map. */
	EBPF_HELPER_MAP_LOOKUP_ELEM      = 1,
	/** Create or update an element in a map. */
	EBPF_HELPER_MAP_UPDATE_ELEM      = 2,
	/** Delete an element from a map. */
	EBPF_HELPER_MAP_DELETE_ELEM      = 3,
	/** Return a monotonic kernel timestamp in nanoseconds. */
	EBPF_HELPER_KTIME_GET_NS         = 5,
	/** Emit a tracing message for debugging. */
	EBPF_HELPER_TRACE_PRINTK         = 6,
	/** Return the current CPU index. */
	EBPF_HELPER_GET_SMP_PROCESSOR_ID = 8,
	/** Return the current thread and process identifier pair. */
	EBPF_HELPER_GET_CURRENT_PID_TGID = 14,
	/** Copy the current thread name into a caller-provided buffer. */
	EBPF_HELPER_GET_CURRENT_COMM     = 16,
	/** Append a record to a ring buffer map. */
	EBPF_HELPER_RINGBUF_OUTPUT       = 130,
	/** Number of supported helper IDs. */
	EBPF_HELPER_MAX
};

/** @cond INTERNAL_HIDDEN */

/**
 * @brief Type for helper function implementations.
 *
 * Takes up to 5 arguments from R1-R5, returns value to R0.
 */
typedef uint64_t (*ebpf_helper_fn)(uint64_t r1, uint64_t r2, uint64_t r3,
				   uint64_t r4, uint64_t r5);

/**
 * @brief Get helper function by ID.
 *
 * @param id Helper function ID
 * @return Function pointer, or NULL if invalid
 */
ebpf_helper_fn ebpf_get_helper(uint32_t id);

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_EBPF_HELPERS_H_ */
