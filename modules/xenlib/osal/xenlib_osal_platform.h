/*
 * Copyright (c) 2026 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Xenlib Operating System Abstraction Layer (OSAL) platform header.
 * Provides static part of the OSAL interface definition, including
 * function-like macros, constants, and type definitions.
 */

#ifndef XENLIB_OSAL_PLATFORM_H
#define XENLIB_OSAL_PLATFORM_H

#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
/* compiler compatibility header */
#include <zephyr/toolchain.h>

#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/dlist.h>
#include <zephyr/sys/slist.h>
#include <zephyr/shell/shell.h>

/**
 * Shell text color constants.
 */
#define XENLIB_OSAL_SHELL_NORMAL SHELL_NORMAL
#define XENLIB_OSAL_SHELL_INFO SHELL_INFO
#define XENLIB_OSAL_SHELL_OPTION SHELL_OPTION
#define XENLIB_OSAL_SHELL_WARNING SHELL_WARNING
#define XENLIB_OSAL_SHELL_ERROR SHELL_ERROR

/**
 * Shell output macro functions.
 */
#define xenlib_osal_shell_info(shell, ...) \
	shell_info(shell, __VA_ARGS__)

#define xenlib_osal_shell_warn(shell, ...) \
	shell_warn(shell, __VA_ARGS__)

#define xenlib_osal_shell_error(shell, ...) \
	shell_error(shell, __VA_ARGS__)

#define xenlib_osal_shell_print(shell, ...) \
	shell_print(shell, __VA_ARGS__)

#define xenlib_osal_shell_fprintf(shell, color, ...) \
	shell_fprintf(shell, color, __VA_ARGS__)

/**
 * Shell commands and arguments definition macro functions
 */
#define XENLIB_OSAL_SHELL_STATIC_SUBCMD_SET_CREATE(name, ...) \
	SHELL_STATIC_SUBCMD_SET_CREATE(name, __VA_ARGS__)

#define XENLIB_OSAL_SHELL_CMD_ARG(syntax, subcmd, help, handler, mand, opt) \
	SHELL_CMD_ARG(syntax, subcmd, help, handler, mand, opt)

#define XENLIB_OSAL_SHELL_CMD_ARG_REGISTER(syntax, subcmd, help, handler, \
	mandatory, optional) \
	SHELL_CMD_ARG_REGISTER(syntax, subcmd, help, handler, mandatory, optional)

#define XENLIB_OSAL_SHELL_SUBCMD_SET_END SHELL_SUBCMD_SET_END

/**
 * Doubly linked list operations
 */
#define XENLIB_OSAL_DLIST_STATIC_INIT(list) \
	SYS_DLIST_STATIC_INIT(list)

#define XENLIB_OSAL_DLIST_FOR_EACH_CONTAINER(list, container, member) \
	SYS_DLIST_FOR_EACH_CONTAINER(list, container, member)

#define XENLIB_OSAL_DLIST_FOR_EACH_CONTAINER_SAFE(list, container, next, member) \
	SYS_DLIST_FOR_EACH_CONTAINER_SAFE(list, container, next, member)

#define XENLIB_OSAL_DLIST_PEEK_HEAD_CONTAINER(list, container, member) \
	SYS_DLIST_PEEK_HEAD_CONTAINER(list, container, member)

/**
 * Single linked list operations
 */
#define XENLIB_OSAL_SLIST_STATIC_INIT(list) \
	SYS_SLIST_STATIC_INIT(list)

#define XENLIB_OSAL_SLIST_FOR_EACH_CONTAINER(list, container, member) \
	SYS_SLIST_FOR_EACH_CONTAINER(list, container, member)

#define XENLIB_OSAL_SLIST_FOR_EACH_CONTAINER_SAFE(list, container, next, member) \
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(list, container, next, member)

#define XENLIB_OSAL_SLIST_PEEK_HEAD_CONTAINER(list, container, member) \
	SYS_SLIST_PEEK_HEAD_CONTAINER(list, container, member)

/*
 * Register the Xenlib logging module.
 * Call once in one source file per logging module.
 */
#define XENLIB_OSAL_LOG_MODULE_REGISTER(name) \
	LOG_MODULE_REGISTER(name)

/**
 * Logging functions (by level)
 */
#define XENLIB_OSAL_LOG_ERR(...) \
	LOG_ERR(__VA_ARGS__)

#define XENLIB_OSAL_LOG_WRN(...) \
	LOG_WRN(__VA_ARGS__)

#define XENLIB_OSAL_LOG_INF(...) \
	LOG_INF(__VA_ARGS__)

#define XENLIB_OSAL_LOG_DBG(...) \
	LOG_DBG(__VA_ARGS__)

/*
 * Assert macro functions
 */
#define XENLIB_OSAL_ASSERT(cond, ...)	__ASSERT(cond, __VA_ARGS__)
#define XENLIB_OSAL_ASSERT_NO_MSG(cond)	__ASSERT_NO_MSG(cond)
#define XENLIB_OSAL_BUILD_ASSERT(cond)	BUILD_ASSERT(cond)

/*
 * Macro function for defining module entry-point
 */
#define XENLIB_OSAL_INIT(init_fn, level, prio) \
	SYS_INIT(init_fn, level, prio)

/*
 * Static mutex define
 */
#define XENLIB_OSAL_MUTEX_DEFINE(name) K_MUTEX_DEFINE(name)

/*
 * Timeout constants
 */
#define XENLIB_OSAL_WAIT_FOREVER  K_FOREVER
#define XENLIB_OSAL_NO_WAIT       K_NO_WAIT

/*
 * Static thread stack definition functions
 */
#define XENLIB_OSAL_THREAD_STACK_ARRAY_DEFINE(name, count, size) \
	K_THREAD_STACK_ARRAY_DEFINE(name, count, size)

#define XENLIB_OSAL_THREAD_STACK_DEFINE(name, size) \
	K_THREAD_STACK_DEFINE(name, size)

/*
 * OSAL types definition
 */
typedef struct shell xenlib_osal_shell_t;
typedef shell_bypass_cb_t xenlib_osal_shell_bypass_cb_t;

typedef struct k_thread xenlib_osal_thread_t;
typedef k_thread_stack_t xenlib_osal_thread_stack_t;
typedef k_thread_entry_t xenlib_osal_thread_entry_t;
typedef k_tid_t xenlib_osal_tid_t;

typedef struct k_sem xenlib_osal_semaphore_t;
typedef struct k_mutex xenlib_osal_mutex_t;

typedef k_timeout_t xenlib_osal_timeout_t;

typedef atomic_t xenlib_osal_atomic_t;
typedef atomic_val_t xenlib_osal_atomic_val_t;

typedef sys_dlist_t xenlib_osal_dlist_t;
typedef sys_dnode_t xenlib_osal_dnode_t;
typedef sys_slist_t xenlib_osal_slist_t;
typedef sys_snode_t xenlib_osal_snode_t;

typedef k_timepoint_t xenlib_osal_timepoint_t;

#endif /* XENLIB_OSAL_PLATFORM_H */
