/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup subsys_tracing_apis
 * @brief Default implementations of the low-level sys_trace_* hook layer.
 *
 * Besides the sys_port_trace_* macro hooks (see tracing_hooks.h), the kernel
 * calls a small set of lower-level @c sys_trace_* hooks for the scheduler/ISR,
 * idle, system init and user-named events. A tracing format that does not
 * instrument these can include this header to get safe defaults:
 *
 * - @c sys_trace_isr_* / @c sys_trace_idle* resolve to the weak no-op (or CPU
 *   load accounting) implementations in tracing_none.c, which a format may
 *   override with a real definition.
 * - @c sys_trace_named_event and @c sys_trace_sys_init_* become no-op macros,
 *   guarded by @c \#ifndef so a format that defines its own wins.
 *
 * In-tree formats (CTF, SystemView, ...) provide this layer themselves and do
 * not include this header; it exists for out-of-tree formats selected via
 * @kconfig{CONFIG_TRACING_FORMAT_HEADER} or @kconfig{CONFIG_TRACING_CUSTOM}.
 */

#ifndef ZEPHYR_INCLUDE_TRACING_TRACING_DEFAULT_HOOKS_H_
#define ZEPHYR_INCLUDE_TRACING_TRACING_DEFAULT_HOOKS_H_

/** @brief Trace an interrupt service routine being entered. */
void sys_trace_isr_enter(void);
/** @brief Trace an interrupt service routine being exited. */
void sys_trace_isr_exit(void);
/** @brief Trace an interrupt service routine exiting into the scheduler. */
void sys_trace_isr_exit_to_scheduler(void);
/** @brief Trace the CPU entering the idle state. */
void sys_trace_idle(void);
/** @brief Trace the CPU leaving the idle state. */
void sys_trace_idle_exit(void);

/**
 * @brief Trace a user-defined named event.
 * @param name Event name
 * @param arg0 First user argument
 * @param arg1 Second user argument
 */
#ifndef sys_trace_named_event
#define sys_trace_named_event(name, arg0, arg1)
#endif

/**
 * @brief Trace a system init level entry being entered.
 * @param entry Init entry being run
 * @param level Init level the entry belongs to
 */
#ifndef sys_trace_sys_init_enter
#define sys_trace_sys_init_enter(entry, level)
#endif

/**
 * @brief Trace a system init level entry being exited.
 * @param entry Init entry that was run
 * @param level Init level the entry belongs to
 * @param result Return code of the init entry
 */
#ifndef sys_trace_sys_init_exit
#define sys_trace_sys_init_exit(entry, level, result)
#endif

#endif /* ZEPHYR_INCLUDE_TRACING_TRACING_DEFAULT_HOOKS_H_ */
