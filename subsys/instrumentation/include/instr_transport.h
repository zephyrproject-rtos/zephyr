/*
 * Copyright (c) 2026 Antmicro
 * Copyright (c) 2026 Analog Devices
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INSTRUMENTATION_INSTR_TRANSPORT_H_
#define ZEPHYR_INSTRUMENTATION_INSTR_TRANSPORT_H_

#include <zephyr/kernel.h>
#include <zephyr/instrumentation/instrumentation.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_INSTRUMENTATION_MODE_STATISTICAL)
/*
 * Entry for discovered functions.
 */
struct disco_func_entry {
	timing_t entry_timestamp; /* Timestamp at function entry */
	uint64_t delta_t;         /* Accumulated (per function) delta time */
	void *addr;               /* Function address/ID */
	uint16_t call_depth;      /* Call depth */
};
#endif

/**
 * @brief Initialize the active transport backend.
 */
void instr_transport_init(void);

/**
 * @brief Push a callgraph record to the active transport.
 *
 * @param record Pointer to the generated instrumentation record.
 */
void instr_transport_push_record(struct instr_record *record);

/**
 * @brief Command the active transport to dump the callgraph trace.
 */
void instr_transport_cmd_dump_trace(void);

/**
 * @brief Command the active transport to dump the profiling statistics.
 */
void instr_transport_cmd_dump_profile(void);

#if defined(CONFIG_INSTRUMENTATION_MODE_STATISTICAL)
/**
 * @brief Send the accumulated profiling statistics via the active transport.
 *
 * @param stats Pointer to the array of discovered function entries.
 *
 * @param count Number of valid entries in the array.
 */
void instr_transport_send_stats(struct disco_func_entry *stats, int count);
#endif

#ifdef __cplusplus
}
#endif

#endif
