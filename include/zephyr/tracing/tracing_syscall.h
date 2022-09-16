/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_TRACING_SYSCALL_H_
#define ZEPHYR_INCLUDE_TRACING_SYSCALL_H_

#if defined CONFIG_SEGGER_SYSTEMVIEW
#include "tracing_sysview_syscall.h"
#elif defined CONFIG_TRACING_TEST
#include "tracing_test_syscall.h"
#else

/**
 * @brief Syscall Tracing APIs
 * @defgroup subsys_tracing_apis_syscall Syscall Tracing APIs
 * @ingroup subsys_tracing_apis
 * @{
 */

/**
 * @brief Trace syscall entry
 * @param id Syscall ID (as defined in the generated syscall_list.h)
 * @param name Syscall name as a token (ex: k_thread_create)
 * @param ... Other parameters passed to the syscall
 */
#define sys_port_trace_syscall_enter(id, name, ...)

/**
 * @brief Trace syscall exit
 * @param id Syscall ID (as defined in the generated syscall_list.h)
 * @param name Syscall name as a token (ex: k_thread_create)
 * @param ... Other parameters passed to the syscall, if the syscall has a
 *            return, the return value is the last parameter in the list
 */
#define sys_port_trace_syscall_exit(id, name, ...)

/** @} */ /* end of subsys_tracing_syscall_apis */

#endif

#endif /* ZEPHYR_INCLUDE_TRACING_SYSCALL_H_ */
