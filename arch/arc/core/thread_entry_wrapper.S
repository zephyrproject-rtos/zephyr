/*
 * Copyright (c) 2014-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Wrapper for z_thread_entry
 *
 * Wrapper for z_thread_entry routine when called from the initial context.
 */

#include <toolchain.h>
#include <linker/sections.h>
#include <v2/irq.h>

GTEXT(z_thread_entry_wrapper)
GTEXT(z_thread_entry_wrapper1)

/*
 * @brief Wrapper for z_thread_entry
 *
 * The routine pops parameters for the z_thread_entry from stack frame, prepared
 * by the z_arch_new_thread() routine.
 *
 * @return N/A
 */

SECTION_FUNC(TEXT, z_thread_entry_wrapper)
	seti _ARC_V2_INIT_IRQ_LOCK_KEY
z_thread_entry_wrapper1:
	pop_s r3
	pop_s r2
	pop_s r1
	pop_s r0
	j z_thread_entry
	nop
