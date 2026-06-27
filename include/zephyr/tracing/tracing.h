/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup subsys_tracing
 * @ingroup subsys_tracing_apis
 * @brief Main header file for tracing subsystem API.
 */

#ifndef ZEPHYR_INCLUDE_TRACING_TRACING_H_
#define ZEPHYR_INCLUDE_TRACING_TRACING_H_

#include "tracking.h"

#if defined CONFIG_SEGGER_SYSTEMVIEW
#include "tracing_sysview.h"
#elif defined CONFIG_TRACING_CTF
#include "tracing_ctf.h"
#elif defined CONFIG_TRACING_TEST
#include "tracing_test.h"
#elif defined CONFIG_TRACING_USER
#include "tracing_user.h"
#elif defined CONFIG_TRACING_CUSTOM
#include "zephyr_custom_tracing.h"
/*
 * Fill any hook the custom header did not define with a canonical no-op. The
 * per-macro #ifndef guards in tracing_hooks.h make the custom header's own
 * definitions win; only gaps are filled.
 */
#include "tracing_hooks.h"
#else
/**
 * @brief Interfaces for the tracing subsystem.
 *
 * The tracing subsystem permits you to collect data from your application and allows tools
 * running on a host to visualize the inner-working of the kernel and various other subsystems.
 *
 * @defgroup subsys_tracing Tracing
 * @ingroup os_services
 * @{
 */

/**
 * @defgroup subsys_tracing_apis Tracing hooks
 * @ingroup subsys_tracing
 * @brief Hook points used by tracing backends.
 *
 * Macros invoked across kernel and subsystem code to mark entry, blocking, exit, and various
 * lifecycle events.
 * @{
 */


/*
 * The full set of sys_port_trace_* hook macros. Kept in a dedicated header so it
 * is the single source of truth, shared with every backend (see below).
 */
#include "tracing_hooks.h"


#if defined(CONFIG_PERCEPIO_TRACERECORDER)
#include "tracing_tracerecorder.h"

/**
 * @brief Called when the cpu exits the idle state
 */
void sys_trace_idle_exit(void);

#else
/**
 * @brief Called when entering an ISR
 */
void sys_trace_isr_enter(void);

/**
 * @brief Called when exiting an ISR
 */
void sys_trace_isr_exit(void);

/**
 * @brief Called when exiting an ISR and switching to scheduler
 */
void sys_trace_isr_exit_to_scheduler(void);

/**
 * @brief Called when the cpu enters the idle state
 */
void sys_trace_idle(void);

/**
 * @brief Called when the cpu exits the idle state
 */
void sys_trace_idle_exit(void);

#endif /* CONFIG_PERCEPIO_TRACERECORDER */

/**
 * @brief Called when entering an init function
 */
#define sys_trace_sys_init_enter(entry, level)

/**
 * @brief Called when exiting an init function
 */
#define sys_trace_sys_init_exit(entry, level, result)

/**
 * @brief Tracing hooks for user-defined named events
 * @defgroup subsys_tracing_apis_named User-defined event
 * @{
 */

/**
 * @brief Called by user to generate named events
 *
 * @param name name of event. Tracing subsystems may place a limit on
 * the length of this string
 * @param arg0 arbitrary user-provided data for this event
 * @param arg1 arbitrary user-provided data for this event
 */
#define sys_trace_named_event(name, arg0, arg1)

/** @} */ /* end of subsys_tracing_apis_named */

/** @} */ /* end of subsys_tracing_apis */

/** @} */ /* end of subsys_tracing */

#endif

#endif /* ZEPHYR_INCLUDE_TRACING_TRACING_H_ */
