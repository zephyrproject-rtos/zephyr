/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Compiler stack protection (kernel part)
 *
 * This module provides functions to support compiler stack protection
 * using canaries.  This feature is enabled with configuration
 * CONFIG_STACK_CANARIES=y or CONFIG_STACK_CANARIES_STRONG=y or
 * CONFIG_STACK_CANARIES_ALL=y or CONFIG_STACK_CANARIES_EXPLICIT=y.
 *
 * When this feature is enabled, the compiler generated code refers to
 * function __stack_chk_fail and global variable __stack_chk_guard.
 */

#include <zephyr/toolchain.h> /* compiler specific configurations */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/kernel.h>
#include <zephyr/app_memory/app_memdomain.h>

/**
 *
 * @brief Stack canary error handler
 *
 * This function is invoked when a stack canary error is detected.
 * Corruption of a function's stack frame guard value is treated as a
 * fatal software error (K_ERR_STACK_CHK_FAIL).
 *
 * @note This function does not return.
 *
 * @satisfies ZEP-SRS-8-12
 * @satisfies ZEP-SRS-8-25
 */
void _StackCheckHandler(void)
{
	/* Stack canary error is a software fatal condition; treat it as such.
	 */
	z_except_reason(K_ERR_STACK_CHK_FAIL);
	CODE_UNREACHABLE; /* LCOV_EXCL_LINE */
}

/* Global variable */

/**
 * @brief Stack canary guard value.
 *
 * Symbol referenced by GCC compiler generated code for canary value.
 * The canary value gets initialized in z_cstart(). When thread-local
 * stack canaries (CONFIG_STACK_CANARIES_TLS) are enabled the guard is
 * thread-local storage, giving each thread its own canary value.
 *
 * @satisfies ZEP-SRS-8-26
 */
#ifdef CONFIG_STACK_CANARIES_TLS
#ifdef CONFIG_STACK_CANARIES_TLS_PREPEND
__attribute__((section(".stack_chk.guard"))) Z_THREAD_LOCAL volatile uintptr_t __stack_chk_guard;
#else
Z_THREAD_LOCAL volatile uintptr_t __stack_chk_guard;
#endif
#elif CONFIG_USERSPACE
K_APP_DMEM(z_libc_partition) volatile uintptr_t __stack_chk_guard;
#else
__noinit volatile uintptr_t __stack_chk_guard;
#endif

/**
 *
 * @brief Referenced by GCC compiler generated code
 *
 * This routine is invoked when a stack canary error is detected, indicating
 * a buffer overflow or stack corruption problem.
 */
FUNC_ALIAS(_StackCheckHandler, __stack_chk_fail, void);
