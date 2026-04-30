/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>

#include <zephyr/ebpf/ebpf_helpers.h>
#include <zephyr/ebpf/ebpf_map.h>

LOG_MODULE_DECLARE(ebpf_vm, CONFIG_EBPF_LOG_LEVEL);

/**
 * Resolve a map lookup helper call and return the value pointer in R0.
 */
static uint64_t ebpf_helper_map_lookup_elem(uint64_t r1, uint64_t r2, uint64_t r3,
					   uint64_t r4, uint64_t r5)
{
	struct ebpf_map *map = ebpf_map_from_id((uint32_t)r1);
	const void *key = (const void *)(uintptr_t)r2;

	if (map == NULL) {
		return 0;
	}

	void *val = ebpf_map_lookup_elem(map, key);

	return (uint64_t)(uintptr_t)val;
}

/**
 * Resolve a map update helper call and return a Zephyr-style status code.
 */
static uint64_t ebpf_helper_map_update_elem(uint64_t r1, uint64_t r2, uint64_t r3,
					   uint64_t r4, uint64_t r5)
{
	struct ebpf_map *map = ebpf_map_from_id((uint32_t)r1);
	const void *key = (const void *)(uintptr_t)r2;
	const void *value = (const void *)(uintptr_t)r3;

	if (map == NULL) {
		return (uint64_t)(int64_t)-ENOENT;
	}

	return (uint64_t)(int64_t)ebpf_map_update_elem(map, key, value, r4);
}

/**
 * Resolve a map delete helper call and return a Zephyr-style status code.
 */
static uint64_t ebpf_helper_map_delete_elem(uint64_t r1, uint64_t r2, uint64_t r3,
					   uint64_t r4, uint64_t r5)
{
	struct ebpf_map *map = ebpf_map_from_id((uint32_t)r1);
	const void *key = (const void *)(uintptr_t)r2;

	if (map == NULL) {
		return (uint64_t)(int64_t)-ENOENT;
	}

	return (uint64_t)(int64_t)ebpf_map_delete_elem(map, key);
}

/**
 * Return a monotonic timestamp in nanoseconds with hardware-cycle precision.
 *
 * Uses k_cycle_get_32() for sub-tick accuracy.  The underlying 32-bit cycle
 * counter wraps approximately every 2^32 / freq seconds (e.g. ~28 s at 150 MHz),
 * which is far longer than any single eBPF measurement interval.
 */
static uint64_t ebpf_helper_ktime_get_ns(uint64_t r1, uint64_t r2, uint64_t r3,
					 uint64_t r4, uint64_t r5)
{
	return k_cyc_to_ns_floor64(k_cycle_get_32());
}

/**
 * Emit a simplified trace log entry for debug-oriented helper calls.
 */
static uint64_t ebpf_helper_trace_printk(uint64_t r1, uint64_t r2, uint64_t r3,
					 uint64_t r4, uint64_t r5)
{
	/* Simplified: just log the format string address and first arg */
	LOG_INF("ebpf_trace: arg1=0x%llx arg2=0x%llx arg3=0x%llx", r3, r4, r5);
	return 0;
}

/**
 * Return the current thread pointer as a lightweight task identifier.
 */
static uint64_t ebpf_helper_get_current_pid_tgid(uint64_t r1, uint64_t r2, uint64_t r3,
						 uint64_t r4, uint64_t r5)
{
	return (uint64_t)(uintptr_t)k_current_get();
}

/**
 * Copy the current thread name into the caller-provided buffer.
 */
static uint64_t ebpf_helper_get_current_comm(uint64_t r1, uint64_t r2, uint64_t r3,
					     uint64_t r4, uint64_t r5)
{
	char *buf = (char *)(uintptr_t)r1;
	uint32_t buf_size = (uint32_t)r2;

	const char *name = k_thread_name_get(k_current_get());

	if (name != NULL && buf_size > 0) {
		strncpy(buf, name, buf_size - 1);
		buf[buf_size - 1] = '\0';
	} else if (buf_size > 0) {
		buf[0] = '\0';
	} else {
	}

	return 0;
}

/**
 * Return the current CPU index when SMP is enabled.
 */
static uint64_t ebpf_helper_get_smp_processor_id(uint64_t r1, uint64_t r2, uint64_t r3,
						 uint64_t r4, uint64_t r5)
{
#ifdef CONFIG_SMP
	return (uint64_t)_current_cpu->id;
#else
	return 0;
#endif
}

#ifdef CONFIG_EBPF_MAP_RINGBUF
/**
 * Publish a payload into an eBPF ring buffer map.
 */
static uint64_t ebpf_helper_ringbuf_output(uint64_t r1, uint64_t r2, uint64_t r3,
					  uint64_t r4, uint64_t r5)
{
	struct ebpf_map *map = ebpf_map_from_id((uint32_t)r1);
	const void *data = (const void *)(uintptr_t)r2;

	if (map == NULL) {
		return (uint64_t)(int64_t)-ENOENT;
	}

	return (uint64_t)(int64_t)ebpf_ringbuf_output(map, data, (uint32_t)r3, r4);
}
#endif

/**
 * Resolve a helper ID to its implementation entry point.
 */
ebpf_helper_fn ebpf_get_helper(uint32_t id)
{
	switch (id) {
	case EBPF_HELPER_MAP_LOOKUP_ELEM: return &ebpf_helper_map_lookup_elem;
	case EBPF_HELPER_MAP_UPDATE_ELEM: return &ebpf_helper_map_update_elem;
	case EBPF_HELPER_MAP_DELETE_ELEM: return &ebpf_helper_map_delete_elem;
	case EBPF_HELPER_KTIME_GET_NS: return &ebpf_helper_ktime_get_ns;
	case EBPF_HELPER_TRACE_PRINTK: return &ebpf_helper_trace_printk;
	case EBPF_HELPER_GET_CURRENT_PID_TGID: return &ebpf_helper_get_current_pid_tgid;
	case EBPF_HELPER_GET_CURRENT_COMM: return &ebpf_helper_get_current_comm;
	case EBPF_HELPER_GET_SMP_PROCESSOR_ID: return &ebpf_helper_get_smp_processor_id;
#ifdef CONFIG_EBPF_MAP_RINGBUF
	case EBPF_HELPER_RINGBUF_OUTPUT: return ebpf_helper_ringbuf_output;
#endif
	default:
		return NULL;
	}
}
