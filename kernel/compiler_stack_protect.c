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
 * CONFIG_STACK_CANARIES=y.
 *
 * When this feature is enabled, the compiler generated code refers to
 * function __stack_chk_fail and global variable __stack_chk_guard.
 */

#include <toolchain.h> /* compiler specific configurations */

#include <kernel_structs.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <kernel.h>
#include <app_memory/app_memdomain.h>

/**
 *
 * @brief Stack canary error handler
 *
 * This function is invoked when a stack canary error is detected.
 *
 * @return Does not return
 */
void _StackCheckHandler(void)
{
	/* Stack canary error is a software fatal condition; treat it as such.
	 */
	z_except_reason(K_ERR_STACK_CHK_FAIL);
	CODE_UNREACHABLE; /* LCOV_EXCL_LINE */
}

/* Global variable */

/*
 * Symbol referenced by GCC compiler generated code for canary value.
 * The canary value gets initialized in z_cstart().
 */
#ifdef CONFIG_USERSPACE
K_APP_DMEM(z_libc_partition) uintptr_t __stack_chk_guard;
#else
__noinit uintptr_t __stack_chk_guard;
#endif

/**
 *
 * @brief Referenced by GCC compiler generated code
 *
 * This routine is invoked when a stack canary error is detected, indicating
 * a buffer overflow or stack corruption problem.
 */
FUNC_ALIAS(_StackCheckHandler, __stack_chk_fail, void);
