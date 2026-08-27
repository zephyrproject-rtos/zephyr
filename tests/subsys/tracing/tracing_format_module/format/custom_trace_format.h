/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Minimal out-of-tree tracing format.
 *
 * This header is selected by CONFIG_TRACING_FORMAT_HEADER and pulled in from
 * <zephyr/tracing/tracing.h> with a computed #include - no edits to the Zephyr
 * core tree are required. A real downstream format would serialize events to its
 * own backend here; this example only counts a couple of hooks so a test can
 * observe that the format is wired in.
 *
 * Note the closing #include: it backfills a no-op for every sys_port_trace_*
 * hook this header does not define, so a format only has to implement the hooks
 * it cares about.
 */

#ifndef CUSTOM_TRACE_FORMAT_H
#define CUSTOM_TRACE_FORMAT_H

/* Storage lives in the application/module; see src/main.c. */
extern volatile unsigned int custom_trace_sem_give_count;
extern volatile unsigned int custom_trace_thread_create_count;

#define sys_port_trace_k_sem_give_enter(sem) (custom_trace_sem_give_count++)
#define sys_port_trace_k_thread_create(new_thread) (custom_trace_thread_create_count++)

/* This format does not instrument the scheduler/idle/init/named layer; pull in
 * the default no-op implementations for it.
 */
#include <zephyr/tracing/tracing_default_hooks.h>

/* Backfill a no-op for every sys_port_trace_* hook not defined above. */
#include <zephyr/tracing/tracing_hooks.h>

#endif /* CUSTOM_TRACE_FORMAT_H */
