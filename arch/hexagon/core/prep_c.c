/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <kernel_internal.h>

/**
 * z_prep_c - Architecture-specific C entry point.
 *
 * Called from the reset handler in hvm_event_vectors.S after BSS has
 * been cleared and the MMU page table configured.  Performs any
 * remaining arch-level setup and hands off to the kernel via z_cstart().
 */
FUNC_NORETURN void z_prep_c(void)
{
	z_cstart();
	CODE_UNREACHABLE;
}
